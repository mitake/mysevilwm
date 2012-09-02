#!/usr/bin/env gosh

(use util.match)
(use srfi-1)

(define marker-mod "EVENT_FUNCTION")

(define init-src '("#include \"events.h\""
                   "#include \"ev_handler.h\""
                   "static struct ev_handler evhandler_array[] = {"))
(define tail-src '("};"
                   "void * evhandler_array_start = evhandler_array;"))

(define-macro (int->charcode integer)
  `(+ ,integer (char->integer #\0)))

(define-macro (first-of-string str)
  `(string-ref ,str 0))

(define-macro (some->charcode some)
  (if (integer? some)
      `(int->charcode ,some)
      `(char->integer (first-of-string (symbol->string ',some)))))

(define (list-cut n lis)
  (if (= n 0)
      lis
      (list-cut (- n 1) (cdr lis))))

(define-macro (char-range from to)
  `(map integer->char (list-cut (some->charcode ,from)
                                (iota (+ 1 (some->charcode ,to))))))

(define *line-to-proc* ())

(define delimiters '(#\newline #\space))

(define (skip-delim)
  (define (skip-delim-impl skipped)
    (if (null? skipped)
        ()
        (if (member (car skipped) delimiters)
            (skip-delim-impl (cdr skipped))
            skipped))
    (set! *line-to-proc* (skip-delim-impl *line-to-proc*))))

(define (skip-preproc-line)
  (define (skip-preproc-line-impl skipped)
    (match (car skipped)
           (#\\ (skip-preproc-line-impl (cddr skipped)))
           (#\newline (cdr skipped))
           (else (skip-preproc-line-impl (cdr skipped)))))
  (set! *line-to-proc* (skip-preproc-line-impl (cdr *line-to-proc*))))

(define (skip-comment-singleline skipped)
  (if (char=? (car skipped) #\newline)
      (cdr skipped)
      (skip-comment-singleline (cdr skipped))))

(define (skip-comment-range skipped)
  (if (char=? (car skipped) #\*)
      (if (char=? (cadr skipped) #\/)
          (cddr skipped)
          (skip-comment-range (cdr skipped)))
      (skip-comment-range (cdr skipped))))

(define (skip-comment)
  (match (cadr *line-to-proc*)
         (#\/ (set! *line-to-proc*
                    (skip-comment-singleline (cddr *line-to-proc*))))
         (#\* (set! *line-to-proc* (skip-comment-range (cddr *line-to-proc*))))
         (else (set! *line-to-proc* (cdr *line-to-proc*)))))

(define (skip-string)
  (define (skip-string-impl skipped)
    (match (car skipped)
           (#\\ (skip-string-impl (cddr skipped)))
           (#\" (cdr skipped))
           (else (skip-string-impl (cdr skipped)))))
  (set! *line-to-proc* (skip-string-impl (cdr *line-to-proc*))))

(define first-of-id (append (char-range a z) (char-range A Z) (list #\_)))
(define rest-of-id (append first-of-id (char-range 0 9)))

(define (get-identifier-impl res lis)
  (if (null? lis)
      (begin (set! *line-to-proc* lis) (apply string res))
      (if (member (car lis) rest-of-id)
          (get-identifier-impl (append res (list (car lis))) (cdr lis))
          (begin (set! *line-to-proc* lis) (apply string res)))))

(define (get-identifier)
  (if (not (member (car *line-to-proc*) first-of-id))
      (begin (set! *line-to-proc* (cdr *line-to-proc*)) (get-token))
      (get-identifier-impl (list (car *line-to-proc*)) (cdr *line-to-proc*))))

(define (get-token)
  (skip-delim)
  (if (null? *line-to-proc*)
      ""
      (match (car *line-to-proc*)
             (#\# (skip-preproc-line) (get-token))
             (#\/ (skip-comment) (get-token))
             (#\" (skip-string) (get-token))
             (else (get-identifier)))))

(define (port->charline port)
  (define (stdin->charline-impl port res)
    (let ((next (read-char port)))
      (if (eof-object? next)
          res
          (stdin->charline-impl port (append res (list next))))))
  (stdin->charline-impl port ()))

(define (make-token-list)
  (define (make-token-list-impl res)
    (let ((new-token (get-token)))
      (if (string=? new-token "")
          res
          (make-token-list-impl (append res (list new-token))))))
  (make-token-list-impl ()))

(define (find-handler lis)
  (define (find-handler-impl lis res)
    (if (null? lis)
        res
        (if (string=? marker-mod (car lis))
            (find-handler-impl (cddr lis) (append res (list (cadr lis))))
            (find-handler-impl (cdr lis) res))))
  (find-handler-impl lis ()))

(define (name->member name)
  (string-append " { .handler = " name ", .name = \"" name "\" },"))

(define (evhandler-num n)
  (list (string-append "int evhandler_num = " (format #f "~a" n) ";")))

(define (proc-handlers handlers)
  (let ((handler-num (length handlers)))
    (map print
         (append
          init-src
          (map name->member handlers)
          tail-src
          (evhandler-num handler-num)))))

(define (main args)
  (set! *line-to-proc* (port->charline (current-input-port)))
  (proc-handlers (find-handler (make-token-list)))
  0)

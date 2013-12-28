#CFLAGS = -g -DSTDIO -DCLICK_FOCUS -DDEBUG
CFLAGS = -Os -DSTDIO -DCLICK_FOCUS
LDFLAGS = -L/usr/X11R6/lib -lX11
OBJS = newclient.o screen.o keys.o mark.o events.o sevilwm.o ignore.o client.o str.o parser.o wins.o ev_handler.o config_key.o config_win.o config_ign.o
EXES = sevilwm

all: $(EXES)

sevilwm: $(OBJS)
	$(CC) -o sevilwm $(OBJS) $(LDFLAGS) $(CFLAGS)

ev_handler.c: events.h events.c ev_handler.scm
	gosh ev_handler.scm < events.h > ev_handler.tmp.c
	mv ev_handler.tmp.c ev_handler.c

.def.c:
	echo "const char * "`echo $< | sed s/\\\\./_/g`" = \"\\" > $@.tmp
	cat $< | sed -e 's/\\/\\\\/g; s/"/\\"/g' | awk '{print $$0"\\n\\"}' \
	>> $@.tmp
	echo "\";" >> $@.tmp
	mv $@.tmp $@

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) -f $(OBJS) $(EXES) config_*.def*.c ev_handler.tmp.c ev_handler.c

cscope:
	find . -name "*\.[ch]" > cscope.files
	find . -name "*\.def" >> cscope.files
	cscope -b -q

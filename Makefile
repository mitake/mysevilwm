#CFLAGS = -g -DSTDIO -DCLICK_FOCUS -DDEBUG
CFLAGS = -Os -DSTDIO -DCLICK_FOCUS
LDFLAGS = -L/usr/X11R6/lib -lX11
OBJS = newclient.o screen.o keys.o mark.o events.o sevilwm.o client.o str.o parser.o
EXES = sevilwm

all: $(EXES)

sevilwm: $(OBJS)
	$(CC) -o sevilwm $(OBJS) $(LDFLAGS) $(CFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) -f $(OBJS) $(EXES) config_*.def*.c ev_handler.tmp.c ev_handler.c

cscope:
	find . -name "*\.[ch]" > cscope.files
	find . -name "*\.def" >> cscope.files
	cscope -b -q

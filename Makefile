CFLAGS = -Os
LDFLAGS = -L/usr/X11R6/lib -lX11
OBJS = screen.o keys.o events.o sevilwm.o client.o
EXES = sevilwm

all: $(EXES)

sevilwm: $(OBJS)
	$(CC) -o sevilwm $(OBJS) $(LDFLAGS) $(CFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	$(RM) -f $(OBJS) $(EXES)

cscope:
	find . -name "*\.[ch]" > cscope.files
	find . -name "*\.def" >> cscope.files
	cscope -b -q

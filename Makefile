
CC	= gcc
CFLAGS	= `pkg-config --cflags gtk+-2.0`
LD	= gcc
LDFLAGS	=
LIBS	= `pkg-config --libs gtk+-2.0`
BINS	= tray_reboot

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<


all: $(BINS)

tray_reboot: reboot.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

clean:
	rm -f core *.o *~

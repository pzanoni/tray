
BINDIR	= /usr/bin
ICONDIR	= /usr/share/icons


CC	= gcc
CFLAGS	= `pkg-config --cflags gtk+-2.0` -DICON_DIR=\"$(ICONDIR)\"
LD	= gcc
LDFLAGS	=
LIBS	= `pkg-config --libs gtk+-2.0`
INSTALL	= install
BINS	= tray_reboot
INSTS	= $(BINS:=-install)

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<


all: $(BINS)

install: $(INSTS)

tray_reboot: reboot.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_reboot-install: tray_reboot
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m644 icons/reboot/*.png $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m755 tray_reboot $(DESTDIR)$(BINDIR)
	

clean:
	rm -f core *.o *~

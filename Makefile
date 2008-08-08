
BINDIR	= /usr/bin
ICONDIR	= /usr/share/icons
LOCALEDIR = /usr/share/locale


CC	= gcc
CFLAGS	= -Wall -O2 -DICON_DIR=\"$(ICONDIR)\" -DLOCALE_DIR=\"$(LOCALEDIR)\" \
	  `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gdk-2.0` 
LD	= gcc
LDFLAGS	=
LIBS	= `pkg-config --libs gtk+-2.0` `pkg-config --libs gdk-2.0`
INSTALL	= install
MSGFMT	= msgfmt -vv
BINS	= tray_reboot tray_keyleds
INSTS	= $(BINS:=-install)
LOCS	= $(BINS)
LANGS	= pt_BR

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<

%.mo: %.po
	$(MSGFMT) -o $*.mo $<

all: $(BINS)

install: $(INSTS)

locale:
	@for i in $(LOCS); do \
		for j in $(LANGS); do \
			$(MAKE) intl/$$i/$$j.mo; \
		done \
	done

tray_reboot: reboot.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_keyleds: keyleds.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_reboot-install: tray_reboot
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m644 icons/tray_reboot/*.png $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m755 -s tray_reboot $(DESTDIR)$(BINDIR)
	for i in $(LANGS); do \
		$(INSTALL) -m644 -D intl/tray_reboot/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/tray_reboot.mo; \
	done
	
clean:
	rm -f core *.o *~ intl/*/*.mo


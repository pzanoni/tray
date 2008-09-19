
BINDIR	= /usr/bin
ICONDIR	= /usr/share/icons
LOCALEDIR = /usr/share/locale


CC	= gcc -g
CFLAGS	= -Wall -O2 -DICON_DIR=\"$(ICONDIR)\" -DLOCALE_DIR=\"$(LOCALEDIR)\" \
	  `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gdk-2.0` 
LD	= gcc
LDFLAGS	=
LIBS	= `pkg-config --libs gtk+-2.0` `pkg-config --libs gdk-2.0`
INSTALL	= install
MSGFMT	= msgfmt -vv
BINS	= tray_reboot tray_keyleds tray_mixer
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

tray_mixer: mixer.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS) -lasound

tray_reboot-install: tray_reboot
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m644 icons/tray_reboot/*.png $(DESTDIR)$(ICONDIR)/tray_reboot
	$(INSTALL) -m755 -s tray_reboot $(DESTDIR)$(BINDIR)
	for i in $(LANGS); do \
		$(INSTALL) -m644 -D intl/tray_reboot/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/tray_reboot.mo; \
	done

tray_keyleds-install: tray_keyleds
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/tray_keyleds
	$(INSTALL) -m644 icons/tray_keyleds/*.png $(DESTDIR)$(ICONDIR)/tray_keyleds
	$(INSTALL) -m755 -s tray_keyleds $(DESTDIR)$(BINDIR)
	for i in $(LANGS); do \
		$(INSTALL) -m644 -D intl/tray_keyleds/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/tray_keyleds.mo; \
	done
	
tray_mixer-install: tray_mixer
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/tray_mixer
	$(INSTALL) -m644 icons/tray_mixer/*.png $(DESTDIR)$(ICONDIR)/tray_mixer
	$(INSTALL) -m755 -s tray_mixer $(DESTDIR)$(BINDIR)
	for i in $(LANGS); do \
		$(INSTALL) -m644 -D intl/tray_mixer/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/tray_mixer.mo; \
	done
	
clean:
	rm -f core *.o *~ intl/*/*.mo


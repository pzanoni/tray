
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
BINS	= tray_reboot tray_keyleds tray_mixer tray_eject
INSTS	= $(BINS:=-install)
LOCS	= $(BINS)
LANGS	= pt_BR

.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<

%.mo: %.po
	$(MSGFMT) -o $*.mo $<

all: $(BINS)

install: locale $(INSTS)

locale:
	@for i in $(LOCS); do \
		for j in $(LANGS); do \
			$(MAKE) intl/$$i/$$j.mo; \
		done \
	done

eject.o: eject.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags hal` -o $*.o $<

tray_reboot: reboot.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_keyleds: keyleds.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_mixer: mixer.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS) -lasound

tray_eject: eject.o
	$(LD) $(LDFLAGS) -o $@ $+ `pkg-config --libs hal` `pkg-config --libs dbus-glib-1` $(LIBS)


%-install: %
	@$(MAKE) applet-install APPLET="$+"

applet-install:
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/$(APPLET)
	$(INSTALL) -m644 icons/$(APPLET)/*.png $(DESTDIR)$(ICONDIR)/$(APPLET)
	$(INSTALL) -m755 -s $(APPLET) $(DESTDIR)$(BINDIR)
	for i in $(LANGS); do \
		$(INSTALL) -m644 -D intl/$(APPLET)/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/$(APPLET).mo; \
	done

clean:
	rm -f core *.o *~ intl/*/*.mo


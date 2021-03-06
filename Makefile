
PREFIX    = /usr
BINDIR    = $(PREFIX)/bin
SBINDIR   = $(PREFIX)/sbin
ICONDIR   = $(PREFIX)/share/icons
LOCALEDIR = $(PREFIX)/share/locale


CC	= gcc -g
CFLAGS	= -Wall -O2 -DICON_DIR=\"$(ICONDIR)\" -DLOCALE_DIR=\"$(LOCALEDIR)\" \
	  -DSBIN_DIR=\"$(SBINDIR)\" \
	  `pkg-config --cflags gtk+-2.0` `pkg-config --cflags gdk-2.0`
LD	= gcc
LDFLAGS	=
LIBS	= `pkg-config --libs gtk+-2.0` `pkg-config --libs gdk-2.0`
INSTALL	= install
MSGFMT	= msgfmt -vv
T_BINS = tray_buttons tray_eject tray_keyleds tray_mixer tray_randr tray_reboot
S_BINS	= vold
BINS	= $(T_BINS) $(S_BINS)
INSTS	= $(BINS:=-install)
LANGS	= pt_BR fr_FR de_DE es_ES it_IT nl_NL pl_PL pt_PT zh_CN
INTL_PROGS = buttons eject keyleds mixer randr reboot


.c.o:
	$(CC) -c $(CFLAGS) -o $*.o $<

%.mo: %.po
	$(MSGFMT) -o $*.mo $<

all: $(BINS)

install: locale $(INSTS) logout-sh

update-po:
	@for prog in $(INTL_PROGS); do \
		xgettext -k_ $$prog.c -o intl/tray_$$prog/$$prog.pot; \
		for lang in $(LANGS); do \
			msgmerge -U --backup=off intl/tray_$$prog/$$lang.po \
				 intl/tray_$$prog/$$prog.pot; \
		done \
	done

locale:
	@for i in $(T_BINS); do \
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

tray_randr: randr.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

tray_buttons: buttons.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)

vold: volume.o
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS) -lasound


%-install: %
	@$(MAKE) applet-install APPLET="$+"

applet-install:
	$(INSTALL) -m755 -d $(DESTDIR)$(ICONDIR)/$(APPLET)
	$(INSTALL) -m644 icons/$(APPLET)/* $(DESTDIR)$(ICONDIR)/$(APPLET)
	$(INSTALL) -m755 -s -D $(APPLET) $(DESTDIR)$(BINDIR)/$(APPLET)
	for i in $(LANGS); do \
		if test -f intl/$(APPLET)/$$i.mo; then \
			$(INSTALL) -m644 -D intl/$(APPLET)/$$i.mo \
			$(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/$(APPLET).mo; \
		fi \
	done

logout-sh:
	$(INSTALL) -m755 -D tray_logout.sh $(DESTDIR)$(SBINDIR)/tray_logout.sh

clean:
	rm -f core *.o *~ intl/*/*.mo

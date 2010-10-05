
#ifndef __TRAY_H
#define __TRAY_H

#include <libintl.h>

#define _(x) gettext(x)
#define N_(x) (x)

#ifndef ICON_DIR
#define ICON_DIR "/usr/share/icons"
#endif

#ifndef LOCALE_DIR
#define LOCALE_DIR "/usr/share/locale"
#endif

#ifndef SBIN_DIR
#define SBIN_DIR "/usr/sbin"
#endif

#endif

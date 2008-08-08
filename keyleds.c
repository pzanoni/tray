#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_keyleds/"
#define POPUP_MENU

struct indicator {
	GtkStatusIcon *icon;
	char name[2][NAME_MAX];
	int mask;
	int val;
	int oldval;
};

static Display *display;
static Window rootwin;
#ifdef POPUP_MENU
GtkWidget *menu, *item;
#endif

static int cancel;

#ifdef POPUP_MENU
static void popup()
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
					gtk_get_current_event_time());
}

static void quit()
{
	//gtk_main_quit();
	cancel = 1;
}
#endif

static void init_ind(struct indicator *ind, char *i0, char *i1, int keycode, int mask)
{
	strncpy((char *)&ind->name[0], i0, NAME_MAX);
	strncpy((char *)&ind->name[1], i1, NAME_MAX);
	ind->mask = mask;
	ind->oldval = -1;
	ind->val = 1;

	ind->icon = (GtkStatusIcon *)gtk_status_icon_new_from_file(ind->name[0]);

	XGrabKey(display, keycode, AnyModifier, rootwin, True,
					GrabModeAsync, GrabModeAsync);
}

static void check_status(struct indicator *ind)
{
	unsigned int state;
	
	XkbGetIndicatorState(display, XkbUseCoreKbd, &state);
	ind->val = !!(state & ind->mask);

	if (ind->oldval != ind->val) {
		gtk_status_icon_set_from_file(ind->icon, ind->name[ind->val]);
		ind->oldval = ind->val;
	}
}

int main(int argc, char **argv)
{
	struct indicator caps, num;

	bindtextdomain("tray_keyleds", LOCALE_DIR);
	textdomain("tray_keyleds");

	gtk_init(&argc, &argv);

	display = GDK_DISPLAY();
	rootwin = GDK_ROOT_WINDOW();
	cancel = 0;

	init_ind(&caps, ICON_PATH "caps0.png", ICON_PATH "caps1.png", 66, 0x01);
	init_ind(&num, ICON_PATH "num0.png", ICON_PATH "num1.png", 77, 0x02);

#ifdef POPUP_MENU
	item = gtk_menu_item_new_with_label(N_("Quit"));
	menu = gtk_menu_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(caps.icon), "popup-menu",
						G_CALLBACK(popup), NULL);
	g_signal_connect(G_OBJECT(num.icon), "popup-menu",
						G_CALLBACK(popup), NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(quit), NULL);
#endif

	do {
		check_status(&caps);
		check_status(&num);
		gtk_main_iteration();
	} while (!cancel);

	return 0;
}

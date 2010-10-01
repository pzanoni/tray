#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <getopt.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_reboot/"

GtkWidget *window;
GtkWidget *button1;
GtkWidget *button2;
GtkWidget *button3;
GtkWidget *button4;
GtkWidget *button5;
GtkWidget *button6;
GtkWidget *vbox;
GtkWidget *hbox1, *hbox2, *hbox3;
GtkWidget *menu, *item;
int susp = 1, direct = 0, screensaver = 0, logout = 0;

enum {
	OPT_REBOOT,
	OPT_LOGOUT,
	OPT_SHUTDOWN,
	OPT_SUSPEND,
	OPT_HIBERNATE,
	OPT_CANCEL
};

static void option(GtkButton *button, int n)
{
	gtk_widget_hide_all(window);

	switch (n) {
	case OPT_REBOOT:
		system("reboot");
		break;
	case OPT_LOGOUT:
		system("killall minilauncher");
		break;
	case OPT_SHUTDOWN:
		system("halt");
		break;
	case OPT_SUSPEND:
		system("pm-suspend");
		break;
	case OPT_HIBERNATE:
		system("pm-hibernate");
		break;
	case OPT_CANCEL:
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		if (direct)
			exit(0);
		break;
	}
}
	 
static void click()
{
	gtk_widget_show_all(window);
	gdk_pointer_grab(window->window, TRUE, 0, NULL, NULL, GDK_CURRENT_TIME);
	gdk_keyboard_grab(window->window, TRUE, GDK_CURRENT_TIME);
	gdk_window_raise(window->window);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
}

static void lock()
{
	system("xscreensaver -lock");
}

static void popup()
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
					gtk_get_current_event_time());
}

static void quit()
{
	gtk_main_quit();
}

static void keypress(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Escape) {
		if (direct) {
			quit();
		} else {
			gtk_widget_hide_all(window);
			gdk_keyboard_ungrab(GDK_CURRENT_TIME);
			gdk_pointer_ungrab(GDK_CURRENT_TIME);
		}
	}
}

static void set_button(GtkWidget **b, char *l, int p, char *i)
{
	*b = gtk_button_new_with_label(_(l));
	g_signal_connect(G_OBJECT(*b), "clicked", G_CALLBACK(option),
							(gpointer)(p));
	gtk_button_set_image(GTK_BUTTON(*b), gtk_image_new_from_file(i));
	gtk_button_set_image_position(GTK_BUTTON(*b), GTK_POS_TOP);
}

int main(int argc, char **argv)
{
	struct GtkStatusIcon *icon1 = NULL;
	struct GtkStatusIcon *icon2 = NULL;
	GtkSettings *settings;
	int o;

	bindtextdomain("tray_reboot", LOCALE_DIR);
	textdomain("tray_reboot");

	while ((o = getopt(argc, argv, "lSsd")) >= 0) {
		switch (o) {
		case 'l':
			logout = 1;
			break;
		case 'S':
			screensaver = 1;
			break;
		case 's':
			susp = 0;
			break;
		case 'd':
			direct = 1;
			break;
		}
	}

	gtk_init(&argc, &argv);

	if (!direct) {
		icon1 = (struct GtkStatusIcon *)
			gtk_status_icon_new_from_file(ICON_PATH "exit.png");
		g_signal_connect(G_OBJECT(icon1), "activate",
						G_CALLBACK(click), NULL);

		item = gtk_menu_item_new_with_label(_("Quit"));
		menu = gtk_menu_new();
		gtk_widget_show(item);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		g_signal_connect(G_OBJECT(icon1), "popup-menu",
						G_CALLBACK(popup), NULL);
		g_signal_connect(G_OBJECT(item), "activate",
						G_CALLBACK(quit), NULL);

		if (screensaver) {
			icon2 = (struct GtkStatusIcon *)
				gtk_status_icon_new_from_file(ICON_PATH "lock.png");
			g_signal_connect(G_OBJECT(icon2), "activate",
						G_CALLBACK(lock), NULL);
			g_signal_connect(G_OBJECT(icon2), "popup-menu",
						G_CALLBACK(popup), NULL);
		}
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 15);
	vbox = gtk_vbox_new(TRUE, TRUE);
	hbox1 = gtk_hbox_new(TRUE, TRUE);
	hbox2 = gtk_hbox_new(TRUE, TRUE);
	hbox3 = gtk_hbox_new(TRUE, TRUE);
	g_signal_connect(G_OBJECT(window), "key-press-event",
						G_CALLBACK(keypress), NULL);

	set_button(&button1, N_("Reboot"), OPT_REBOOT, ICON_PATH "reboot.png");

	set_button(&button2, N_("Turn off"), OPT_SHUTDOWN, ICON_PATH "shutdown.png");
	set_button(&button3, N_("Suspend"), OPT_SUSPEND, ICON_PATH "suspend.png");
	set_button(&button4, N_("Hibernate"), OPT_HIBERNATE, ICON_PATH "hibernate.png");
	set_button(&button5, N_("Cancel"), OPT_CANCEL, ICON_PATH "cancel.png");

	set_button(&button6, N_("Logout"), OPT_LOGOUT, ICON_PATH "logout.png");

	settings = gtk_settings_get_default();
	g_object_set (settings, "gtk-button-images", TRUE, NULL);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), hbox1);
	gtk_container_add(GTK_CONTAINER(hbox1), button1);
	gtk_container_add(GTK_CONTAINER(hbox1), button2);

	if (susp) {
		gtk_container_add(GTK_CONTAINER(vbox), hbox2);
		gtk_container_add(GTK_CONTAINER(hbox2), button3);
		gtk_container_add(GTK_CONTAINER(hbox2), button4);
	}

	if (logout) {
		gtk_container_add(GTK_CONTAINER(vbox), hbox3);
		gtk_container_add(GTK_CONTAINER(hbox3), button6);
		gtk_container_add(GTK_CONTAINER(hbox3), button5);
	} else {
		gtk_container_add(GTK_CONTAINER(vbox), button5);
	}

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window), N_("Quit session"));

	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_new_from_file("/usr/share/icons/gnome/scalable/actions/gtk-quit.svg", NULL);
	gtk_window_set_icon(GTK_WINDOW(window), pixbuf);

	if (direct) {
		click();
	} else {
		gtk_status_icon_set_visible(GTK_STATUS_ICON(icon1), TRUE);
		if (screensaver)
			gtk_status_icon_set_visible(GTK_STATUS_ICON(icon2), TRUE);
	}

	gtk_main();

	return 0;
}

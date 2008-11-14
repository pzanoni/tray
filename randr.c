
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_randr/"


GtkWidget *menu, *sep;
GtkWidget *item_swext, *item_swint, *item_clone;
GtkWidget *item_ext1024, *item_ext800, *item_ext640;
GtkStatusIcon *icon;
int count;

struct mdev {
	GtkWidget *item;
	GtkWidget *item_content;
	char *mountpoint;
};

static void randr(GtkWidget *widget, gpointer data)
{
}

void newitem(GtkWidget **item, char *txt)
{
	*item = gtk_menu_item_new_with_label(txt);
	gtk_widget_show(*item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), *item);
	g_signal_connect(G_OBJECT(*item), "activate", G_CALLBACK(randr), NULL);
}

static void popup()
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
						gtk_get_current_event_time());
}

int main(int argc, char **argv)
{
	bindtextdomain("tray_randr", LOCALE_DIR);
        textdomain("tray_randr");

	gtk_init(&argc, &argv);

	icon = (GtkStatusIcon *)
                        gtk_status_icon_new_from_file(ICON_PATH "randr.png");

	menu = gtk_menu_new();

	newitem(&item_swext,   N_("Switch to external display"));
	newitem(&item_swint,   N_("Switch to built-in display"));
	newitem(&item_clone,   N_("Use both displays"));

	sep = gtk_separator_menu_item_new();
	gtk_widget_show(sep);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

	newitem(&item_ext1024, N_("Set external resolution to 1024x768"));
	newitem(&item_ext800,  N_("Set external resolution to 800x600"));
	newitem(&item_ext640,  N_("Set external resolution to 640x480"));

	g_signal_connect(G_OBJECT(icon), "popup-menu",
						G_CALLBACK(popup), NULL);

	gtk_main();

	return 0;
}

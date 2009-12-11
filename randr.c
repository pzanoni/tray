
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_randr/"

#define CMD_INT_ON  "xrandr --output LVDS --pos 0x0 --auto"
#define CMD_INT_OFF "xrandr --output LVDS --off"
#define CMD_EXT_ON  "xrandr --output VGA --pos 0x0 --auto"
#define CMD_EXT_OFF "xrandr --output VGA --off"

#define CMD_SWEXT CMD_INT_OFF ";" CMD_EXT_ON
#define CMD_SWINT CMD_EXT_OFF ";" CMD_INT_ON
#define CMD_CLONE CMD_INT_ON ";" CMD_EXT_ON
#define CMD_1024 "xrandr --output VGA --mode 1024x768"
#define CMD_800  "xrandr --output VGA --mode 800x600"
#define CMD_640  "xrandr --output VGA --mode 640x480"


GtkWidget *menu, *sep, *item_quit;
GtkWidget *item_swext, *item_swint, *item_clone;
GtkWidget *item_ext1024, *item_ext800, *item_ext640;
GtkStatusIcon *icon;
int count;

struct mdev {
	GtkWidget *item;
	GtkWidget *item_content;
	char *mountpoint;
};

static void quit()
{
	gtk_main_quit();
}

static void randr(GtkWidget *widget, gpointer data)
{
	char *cmd = (char *)data;
	system(cmd);
}

static void newitem(GtkWidget **item, char *txt, char *cmd)
{
	*item = gtk_menu_item_new_with_label(txt);
	gtk_widget_show(*item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), *item);
	g_signal_connect(G_OBJECT(*item), "activate", G_CALLBACK(randr), cmd);
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

	newitem(&item_swext,   _("Switch to external display"), CMD_SWEXT);
	newitem(&item_swint,   _("Switch to built-in display"), CMD_SWINT);
	newitem(&item_clone,   _("Use both displays"), CMD_CLONE);

	sep = gtk_separator_menu_item_new();
	gtk_widget_show(sep);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

	newitem(&item_ext1024, _("Set ext. resolution to 1024x768"), CMD_1024);
	newitem(&item_ext800,  _("Set ext. resolution to 800x600"), CMD_800);
	newitem(&item_ext640,  _("Set ext. resolution to 640x480"), CMD_640);

	g_signal_connect(G_OBJECT(icon), "popup-menu",
						G_CALLBACK(popup), NULL);

	sep = gtk_separator_menu_item_new();
	gtk_widget_show(sep);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

	item_quit = gtk_menu_item_new_with_label(_("Quit"));
	gtk_widget_show(item_quit);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_quit);
	g_signal_connect(G_OBJECT(item_quit), "activate", G_CALLBACK(quit), NULL);
	
	gtk_main();

	return 0;
}

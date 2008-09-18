#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <getopt.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_mixer/"

struct channel {
	GtkWidget *vbox;
	GtkWidget *vscale;
	guint vscale_handler;
	GtkWidget *mute;
};

GtkWidget *window;
GtkWidget *hbox;
GtkWidget *menu, *item;
struct channel ch[1];

	 

static int mixer_read(struct channel *c)
{
	return 0;
}


static int vol_change(struct channel *c)
{
	return 0;
}

static int scale_scroll(struct channel *c)
{
	return 0;
}

static void click()
{
	gtk_widget_show_all(window);
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

int main(int argc, char **argv)
{
	struct GtkStatusIcon *icon;

	bindtextdomain("tray_mixer", LOCALE_DIR);
	textdomain("tray_mixer");

	gtk_init(&argc, &argv);
	icon = (struct GtkStatusIcon *)
			gtk_status_icon_new_from_file(ICON_PATH "speaker.png");
	g_signal_connect(G_OBJECT(icon), "activate", G_CALLBACK(click), NULL);

	item = gtk_menu_item_new_with_label(N_("Quit"));
	menu = gtk_menu_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	g_signal_connect(G_OBJECT(icon), "popup-menu", G_CALLBACK(popup), NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(quit), NULL);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 30);
	hbox = gtk_hbox_new(TRUE, TRUE);

	ch[0].vbox = gtk_vbox_new(TRUE, TRUE);
	ch[0].vscale = gtk_vscale_new(GTK_ADJUSTMENT(
		gtk_adjustment_new(mixer_read(&ch[0]), 0, 100, 0, 0, 0)));
	gtk_scale_set_draw_value(GTK_SCALE(ch[0].vscale), FALSE);
	gtk_range_set_inverted(GTK_RANGE(ch[0].vscale), TRUE);

	ch[0].vscale_handler = g_signal_connect((gpointer)ch[0].vscale,
			"value_changed", G_CALLBACK(vol_change), &ch[0]);
	g_signal_connect(ch[0].vscale,
			"scroll-event", G_CALLBACK(scale_scroll), &ch[0]);

	gtk_status_icon_set_visible(GTK_STATUS_ICON(icon), TRUE);

	gtk_main();

	return 0;
}

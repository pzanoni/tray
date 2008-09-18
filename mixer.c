#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>


#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_mixer/"

struct channel {
	GtkWidget *vbox;
	GtkWidget *vscale;
	guint vscale_handler;
	GtkWidget *mute;
	long min, max;
};

GtkWidget *window;
GtkWidget *hbox;
GtkWidget *menu, *item;
struct channel ch[1];
snd_mixer_t *mixer;
snd_mixer_elem_t *elem;
snd_mixer_selem_id_t *sid;

	 

static int mixer_init()
{
	snd_mixer_elem_t *e;

	snd_mixer_selem_id_alloca(&sid);
	if (snd_mixer_open(&mixer, 0) < 0) {
		fprintf(stderr, "can't open mixer\n");
		exit(1);
	}
	snd_mixer_attach(mixer, "default");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);
	
	snd_mixer_selem_id_set_name(sid, "PCM");
	elem = snd_mixer_find_selem(mixer, sid);
	if (!elem) {
		fprintf(stderr, "can't find PCM element\n");
		exit(1);
	}

	snd_mixer_selem_get_playback_volume_range(elem, &ch[0].min, &ch[0].max);

	return 0;
}

static int mixer_get(struct channel *c)
{
	long l = snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_LEFT, &l);
	long r = snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_RIGHT, &r);

	printf("%ld, %ld\n", l, r);
	return (l + r) / 2;
}

static void mixer_set(struct channel *c, int vol)
{
	snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, vol);
	snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, vol);
mixer_get(c);
}

static void vol_change(GtkRange *range, struct channel *c)
{
	int vol = c->max * gtk_range_get_value(range) / 100;
	mixer_set(c, vol);
}

static int scale_scroll(GtkScale* scale, GdkEventScroll *e, struct channel *c)
{
	int val = gtk_range_get_value((GtkRange*)scale);

	val += e->direction == GDK_SCROLL_UP ? 2 : -2;
	gtk_range_set_value((GtkRange*)scale, CLAMP(val, 0, 100));

	return 0;
}

static void click()
{
	static int show = 0;

	if (show) {
		gtk_widget_hide_all(window);
	} else {
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
		gtk_widget_show_all(window);
	}

	show ^= 1;
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

	mixer_init();

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
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(window),
					GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size(GTK_WINDOW(window), 60, 140);

	hbox = gtk_hbox_new(TRUE, TRUE);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	ch[0].vbox = gtk_vbox_new(TRUE, TRUE);
	ch[0].vscale = gtk_vscale_new(GTK_ADJUSTMENT(
		gtk_adjustment_new(mixer_get(&ch[0]), 0, 100, 0, 0, 0)));
	gtk_scale_set_draw_value(GTK_SCALE(ch[0].vscale), FALSE);
	gtk_range_set_inverted(GTK_RANGE(ch[0].vscale), TRUE);
	gtk_container_add(GTK_CONTAINER(hbox), ch[0].vbox);
	gtk_container_add(GTK_CONTAINER(ch[0].vbox), ch[0].vscale);

	ch[0].vscale_handler = g_signal_connect((gpointer)ch[0].vscale,
			"value_changed", G_CALLBACK(vol_change), &ch[0]);
	g_signal_connect(ch[0].vscale,
			"scroll-event", G_CALLBACK(scale_scroll), &ch[0]);

	gtk_status_icon_set_visible(GTK_STATUS_ICON(icon), TRUE);

	gtk_main();

	return 0;
}

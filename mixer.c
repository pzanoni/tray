#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <getopt.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_mixer/"

struct channel {
	GtkWidget *vbox;
	GtkWidget *vscale;
	GtkWidget *mute;
	int muteval;
	char *name;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	int playback;
};

GtkStatusIcon *icon;
GtkWidget *window;
GtkWidget *hbox;
GtkWidget *menu, *item;
struct channel ch[2];
snd_mixer_t *mixer;

static gboolean on_mixer_event(GIOChannel* channel, GIOCondition cond, void *ud);
static void update_gui(struct channel *c);


static void get_element(char *name, int playback, struct channel *c)
{
	c->name = name;
	c->playback = playback;

	snd_mixer_selem_id_alloca(&c->sid);
	snd_mixer_selem_id_set_name(c->sid, c->name);
	c->elem = snd_mixer_find_selem(mixer, c->sid);
	if (!c->elem) {
		fprintf(stderr, "can't find %s element\n", c->name);
		exit(1);
	}

	if (c->playback)
		snd_mixer_selem_set_playback_volume_range(c->elem, 0, 100);
	else
		snd_mixer_selem_set_capture_volume_range(c->elem, 0, 100);
}

static int mixer_init(char *name, char *iname)
{
	int n, i;
	struct pollfd *fds;

	if (snd_mixer_open(&mixer, 0) < 0) {
		fprintf(stderr, "can't open mixer\n");
		exit(1);
	}
	snd_mixer_attach(mixer, "default");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);

	get_element(name, 1, &ch[0]);
	get_element(iname, 0, &ch[1]);
	
	n = snd_mixer_poll_descriptors_count(mixer);
	fds = calloc(n, sizeof(struct pollfd));
	snd_mixer_poll_descriptors(mixer, fds, n);

	for (i = 0; i < n; i++) {
		GIOChannel* channel = g_io_channel_unix_new( fds[i].fd );
		g_io_add_watch(channel, G_IO_IN|G_IO_HUP, on_mixer_event, NULL);
		g_io_channel_unref(channel);
	}
	return 0;
}

/*** wrappers for mixer api calls ***/

static void _get_volume(snd_mixer_elem_t *elem, int playback, int n, long *val)
{
	if (playback)
		snd_mixer_selem_get_playback_volume(elem, n, val);
	else
		snd_mixer_selem_get_capture_volume(elem, n, val);
}

static void _set_volume(snd_mixer_elem_t *elem, int playback, int n, long val)
{
	if (playback)
		snd_mixer_selem_set_playback_volume(elem, n, val);
	else
		snd_mixer_selem_set_capture_volume(elem, n, val);
}

static void _get_switch(snd_mixer_elem_t *elem, int playback, int n, int *val)
{
	if (playback) {
		snd_mixer_selem_get_playback_switch(elem, n, val);
	} else {
		snd_mixer_selem_get_capture_switch(elem, n, val);
		*val = !*val;
	}
}

static void _set_switch(snd_mixer_elem_t *elem, int playback, int n, int val)
{
	if (playback)
		snd_mixer_selem_set_playback_switch(elem, n, val);
	else
		snd_mixer_selem_set_capture_switch(elem, n, !val);
}

static int _has_switch(snd_mixer_elem_t *elem, int playback)
{
	return playback ?
		snd_mixer_selem_has_playback_switch(elem) :
		snd_mixer_selem_has_capture_switch(elem);
}

/*** end of wrappers for mixer api calls ***/


static int mixer_get(struct channel *c)
{
	long l, r;

	_get_volume(c->elem, c->playback, SND_MIXER_SCHN_FRONT_LEFT, &l);
	_get_volume(c->elem, c->playback, SND_MIXER_SCHN_FRONT_RIGHT, &r);

	return (l + r) / 2;
}

static void mixer_set(struct channel *c, int vol)
{
	_set_volume(c->elem, c->playback, SND_MIXER_SCHN_FRONT_LEFT, vol);
	_set_volume(c->elem, c->playback, SND_MIXER_SCHN_FRONT_RIGHT, vol);
}

static void mixer_getmute(struct channel *c)
{
	if (c->mute) {
		_get_switch(c->elem, c->playback, SND_MIXER_SCHN_FRONT_LEFT,
								&c->muteval);
	} else {
		c->muteval = 1;
	}
}

gboolean mixer_evt_idle;

static gboolean reset_mixer_evt_idle()
{
    mixer_evt_idle = 0;
    return FALSE;
}

static gboolean on_mixer_event(GIOChannel* channel, GIOCondition cond, void *ud)
{
	if (mixer_evt_idle == 0) {
		mixer_evt_idle = g_idle_add_full(G_PRIORITY_DEFAULT,
						 (GSourceFunc) reset_mixer_evt_idle,
						 NULL, NULL);
		snd_mixer_handle_events (mixer);
	}

	if (cond & G_IO_IN) {
		/* update mixer status */
		update_gui(&ch[0]);
		update_gui(&ch[1]);
	}

	if (cond & G_IO_HUP) {
		/* FIXME: This means there're some problems with alsa. */
		return FALSE;
	}

	return TRUE;
}

static void update_icon(struct channel *c)
{
	if (c->playback)
		gtk_status_icon_set_from_file(icon, c->muteval ?
				ICON_PATH "speaker.png" : ICON_PATH "mute.png");
}

static void update_gui(struct channel *c)
{
	mixer_getmute(c);
	update_icon(c);

	if (c->mute) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->mute),
							!c->muteval);
	} else {
		c->muteval = 1;
	}

	gtk_range_set_value(GTK_RANGE(c->vscale), mixer_get(c));
}

static void vol_change(GtkRange *range, struct channel *c)
{
	mixer_set(c, gtk_range_get_value(range));
}

static void mute(GtkWidget *widget, struct channel *c)
{
	int val, i;

	val = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++)
		_set_switch(c->elem, c->playback, i, !val);

	mixer_getmute(c);
	update_icon(c);
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

static void add_channel(struct channel *c)
{
	c->vbox = gtk_vbox_new(FALSE, 5);
	c->vscale = gtk_vscale_new(GTK_ADJUSTMENT(gtk_adjustment_new(
					mixer_get(c), 0, 100, 0, 0, 0)));
	gtk_scale_set_draw_value(GTK_SCALE(c->vscale), FALSE);
	gtk_range_set_inverted(GTK_RANGE(c->vscale), TRUE);
	gtk_container_add(GTK_CONTAINER(hbox), c->vbox);
	gtk_box_pack_start(GTK_BOX(c->vbox), c->vscale, TRUE, TRUE, 0);

	g_signal_connect((gpointer)c->vscale,
			"value_changed", G_CALLBACK(vol_change), c);
	g_signal_connect(c->vscale,
			"scroll-event", G_CALLBACK(scale_scroll), c);

	if (_has_switch(c->elem, c->playback)) {
		c->mute = gtk_check_button_new_with_label(
				c->playback ? "Mute" : "Capture");
		g_signal_connect((gpointer)c->mute,
				"toggled", G_CALLBACK(mute), c);
		gtk_box_pack_end(GTK_BOX(c->vbox), c->mute, FALSE, FALSE, 0);
	}
	
	update_gui(c);
}

int main(int argc, char **argv)
{
	int o;
	char *name = "Master";
	char *iname = "Capture";
	int showinput = 0;

	bindtextdomain("tray_mixer", LOCALE_DIR);
	textdomain("tray_mixer");

	while ((o = getopt(argc, argv, "e:Ii:")) >= 0) {
		switch (o) {
		case 'e':
			name = optarg;
			break;
		case 'i':
			iname = optarg;
			/* fall through */
		case 'I':
			showinput = 1;
			break;
		}
	}

	gtk_init(&argc, &argv);

	mixer_init(name, iname);

	icon = gtk_status_icon_new_from_file(ICON_PATH "speaker.png");
	g_signal_connect(G_OBJECT(icon), "activate", G_CALLBACK(click), NULL);

	item = gtk_menu_item_new_with_label(_("Quit"));
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

	hbox = gtk_hbox_new(TRUE, 5);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	add_channel(&ch[0]);

	if (showinput)
		add_channel(&ch[1]);

	gtk_status_icon_set_visible(GTK_STATUS_ICON(icon), TRUE);

	gtk_main();

	return 0;
}

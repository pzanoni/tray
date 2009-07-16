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
	GtkWidget *hbox;
	GtkWidget *hscale;
	GtkWidget *mute;
	GtkWidget *image;
	int muteval;
};

Display *display;
Window rootwin;
//GtkStatusIcon *icon;
GtkWidget *window;
GtkWidget *hbox;
//GtkWidget *menu, *item;
struct channel ch[1];
snd_mixer_t *mixer;
snd_mixer_elem_t *elem;
snd_mixer_selem_id_t *sid;

KeyCode raise_vol_kc, lower_vol_kc, mute_kc;

static gboolean on_mixer_event(GIOChannel* channel, GIOCondition cond, void *ud);
static void update_gui(struct channel *c);

static void grab_audio_keys () {

    KeySym raise_vol_ks, lower_vol_ks, mute_ks;

    raise_vol_ks = XStringToKeysym("XF86AudioRaiseVolume");
    lower_vol_ks = XStringToKeysym("XF86AudioLowerVolume");
    mute_ks = XStringToKeysym("XF86AudioMute");

    raise_vol_kc = XKeysymToKeycode(GDK_DISPLAY(), raise_vol_ks);
    lower_vol_kc = XKeysymToKeycode(GDK_DISPLAY(), lower_vol_ks);
    mute_kc = XKeysymToKeycode(GDK_DISPLAY(), mute_ks);

    printf("raise_vol_kc: %d\n", raise_vol_kc);
    printf("lower_vol_kc: %d\n", lower_vol_kc);
    printf("mute_kc: %d\n", mute_kc);

    XGrabKey(GDK_DISPLAY(),
             raise_vol_kc,
             AnyModifier,
             GDK_ROOT_WINDOW(),
             False,
             GrabModeAsync,
             GrabModeAsync);

    XGrabKey(GDK_DISPLAY(),
             lower_vol_kc,
             AnyModifier,
             GDK_ROOT_WINDOW(),
             False,
             GrabModeAsync,
             GrabModeAsync);

    XGrabKey(GDK_DISPLAY(),
             mute_kc,
             AnyModifier,
             GDK_ROOT_WINDOW(),
             False,
             GrabModeAsync,
             GrabModeAsync);
}

static int mixer_init(char *name)
{
	int n, i;
	struct pollfd *fds;

	snd_mixer_selem_id_alloca(&sid);
	if (snd_mixer_open(&mixer, 0) < 0) {
		fprintf(stderr, "can't open mixer\n");
		exit(1);
	}
	snd_mixer_attach(mixer, "default");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);
	
	snd_mixer_selem_id_set_name(sid, name);
	elem = snd_mixer_find_selem(mixer, sid);
	if (!elem) {
		fprintf(stderr, "can't find PCM element\n");
		exit(1);
	}

	snd_mixer_selem_set_playback_volume_range(elem, 0, 100);

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

static int mixer_get(struct channel *c)
{
	long l, r;

	snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_LEFT, &l);
	snd_mixer_selem_get_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_RIGHT, &r);

	return (l + r) / 2;
}

static void mixer_set(struct channel *c, int vol)
{
	snd_mixer_selem_set_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_LEFT, vol);
	snd_mixer_selem_set_playback_volume(elem,
				SND_MIXER_SCHN_FRONT_RIGHT, vol);
}

static void mixer_getmute(struct channel *c)
{
	if (c->mute) {
		snd_mixer_selem_get_playback_switch(elem,
				SND_MIXER_SCHN_FRONT_LEFT, &c->muteval);
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
		update_gui (&ch[0]);
	}

	if (cond & G_IO_HUP) {
		/* FIXME: This means there're some problems with alsa. */
		return FALSE;
	}

	return TRUE;
}

#if 0
static void update_icon(struct channel *c)
{
        gtk_status_icon_set_from_file(icon, c->muteval ?
			ICON_PATH "speaker.png" : ICON_PATH "mute.png");
}
#endif

static void update_gui(struct channel *c)
{
	mixer_getmute(c);
	//update_icon(c);

	if (c->mute) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(c->mute),
					!c->muteval);
	} else {
		c->muteval = 1;
	}

	gtk_range_set_value(GTK_RANGE(c->hscale), mixer_get(c));
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
		snd_mixer_selem_set_playback_switch(elem, i, !val);

	mixer_getmute(c);
	//update_icon(c);
}

static int scale_scroll(GtkScale* scale, GdkEventScroll *e, struct channel *c)
{
	int val = gtk_range_get_value((GtkRange*)scale);

	val += e->direction == GDK_SCROLL_UP ? 2 : -2;
	gtk_range_set_value((GtkRange*)scale, CLAMP(val, 0, 100));

	return 0;
}

#if 0
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
#endif

GdkFilterReturn event_callback(GdkXEvent *gdk_xev,
			       GdkEvent *gdk_ev,
			       gpointer data) {

	XEvent *xev = (XEvent *) gdk_xev;
	XKeyEvent *xke;

	switch (xev->type) {
		printf("event: %d\n", xev->type);
		case Expose:
			break;

		case KeyPress:
			xke = (XKeyEvent *)xev;
			if (xke->keycode == raise_vol_kc)
				printf("volume++\n");
			else if (xke->keycode == lower_vol_kc)
				printf("volume--\n");
			else if (xke->keycode == mute_kc) {
				printf("{,un}mute\n");
			}
		case KeyRelease:
			printf("key release\n");

		default:
			break;
	}

	return GDK_FILTER_CONTINUE;
}



int main(int argc, char **argv)
{
	int o;
	char *name = "Master";

	bindtextdomain("tray_mixer", LOCALE_DIR);
	textdomain("tray_mixer");

	while ((o = getopt(argc, argv, "e:")) >= 0) {
		switch (o) {
		case 'e':
			name = optarg;
			break;
		}
	}

	gtk_init(&argc, &argv);

	mixer_init(name);

	//icon = gtk_status_icon_new_from_file(ICON_PATH "speaker.png");
	//g_signal_connect(G_OBJECT(icon), "activate", G_CALLBACK(click), NULL);

	//item = gtk_menu_item_new_with_label(N_("Quit"));
	//menu = gtk_menu_new();
	//gtk_widget_show(item);
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	//g_signal_connect(G_OBJECT(icon), "popup-menu", G_CALLBACK(popup), NULL);
	//g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(quit), NULL);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(window),
					GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 40);

	hbox = gtk_hbox_new(TRUE, 5);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	ch[0].hbox = gtk_hbox_new(FALSE, 5);
	ch[0].image = gtk_image_new_from_file (ICON_PATH "speaker.png");
	ch[0].hscale = gtk_hscale_new(GTK_ADJUSTMENT(
		gtk_adjustment_new(mixer_get(&ch[0]), 0, 100, 0, 0, 0)));
	gtk_scale_set_draw_value(GTK_SCALE(ch[0].hscale), FALSE);
	gtk_range_set_inverted(GTK_RANGE(ch[0].hscale), TRUE);
	gtk_container_add(GTK_CONTAINER(hbox), ch[0].hbox);
	gtk_box_pack_start(GTK_BOX(ch[0].hbox), ch[0].image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ch[0].hbox), ch[0].hscale, TRUE, TRUE, 0);

	g_signal_connect((gpointer)ch[0].hscale,
			"value_changed", G_CALLBACK(vol_change), &ch[0]);
	g_signal_connect(ch[0].hscale,
			"scroll-event", G_CALLBACK(scale_scroll), &ch[0]);

#if 0
	if (snd_mixer_selem_has_playback_switch(elem)) {
		ch[0].mute = gtk_check_button_new_with_label("Mute");
		g_signal_connect((gpointer)ch[0].mute,
				"toggled", G_CALLBACK(mute), &ch[0]);
		gtk_box_pack_end(GTK_BOX(ch[0].hbox), ch[0].mute, FALSE, FALSE, 0);
	}
#endif
	
	update_gui(&ch[0]);

	//gtk_status_icon_set_visible(GTK_STATUS_ICON(icon), TRUE);

	grab_audio_keys();

	gtk_widget_show_all(window);
	gdk_window_raise(window->window);
	gdk_window_add_filter(NULL, event_callback, NULL);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

	gtk_main();

	return 0;
}

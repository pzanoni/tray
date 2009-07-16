#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <getopt.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/vold/"
#define STEP 5

struct channel {
	GtkWidget *hbox;
	GtkWidget *pbar;
	GtkWidget *mute;
	GtkWidget *image;
	int muteval;
};

Display *display;
Window rootwin;
GtkWidget *window;
GtkWidget *hbox;
struct channel ch;
snd_mixer_t *mixer;
snd_mixer_elem_t *elem;
snd_mixer_selem_id_t *sid;

KeyCode raise_vol_kc, lower_vol_kc, mute_kc;

static gboolean on_mixer_event(GIOChannel* channel, GIOCondition cond, void *ud);
static void update_gui(struct channel *c);


static void grab_audio_keys()
{
	KeySym raise_vol_ks, lower_vol_ks, mute_ks;

#if 0
	raise_vol_ks = XStringToKeysym("XF86AudioRaiseVolume");
	lower_vol_ks = XStringToKeysym("XF86AudioLowerVolume");
	mute_ks = XStringToKeysym("XF86AudioMute");
#else
	raise_vol_ks = XStringToKeysym("Right");
	lower_vol_ks = XStringToKeysym("Left");
	mute_ks = XStringToKeysym("Down");
#endif

	raise_vol_kc = XKeysymToKeycode(GDK_DISPLAY(), raise_vol_ks);
	lower_vol_kc = XKeysymToKeycode(GDK_DISPLAY(), lower_vol_ks);
	mute_kc = XKeysymToKeycode(GDK_DISPLAY(), mute_ks);

	XGrabKey(GDK_DISPLAY(), raise_vol_kc, AnyModifier, GDK_ROOT_WINDOW(),
					False, GrabModeAsync, GrabModeAsync);
	XGrabKey(GDK_DISPLAY(), lower_vol_kc, AnyModifier, GDK_ROOT_WINDOW(),
					False, GrabModeAsync, GrabModeAsync);
	XGrabKey(GDK_DISPLAY(), mute_kc, AnyModifier, GDK_ROOT_WINDOW(),
					False, GrabModeAsync, GrabModeAsync);
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

static int mixer_getmute(struct channel *c)
{
	int val;

	snd_mixer_selem_get_playback_switch(elem,
				SND_MIXER_SCHN_FRONT_LEFT, &val);

	return val;
}

static void mixer_setmute(struct channel *c, int val)
{
	int i;

	for (i = 0; i <= SND_MIXER_SCHN_LAST; i++)
		snd_mixer_selem_set_playback_switch(elem, i, val);
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
				 (GSourceFunc)reset_mixer_evt_idle, NULL, NULL);
		snd_mixer_handle_events (mixer);
	}

	if (cond & G_IO_IN) {
		/* update mixer status */
		update_gui(&ch);
	}

	if (cond & G_IO_HUP) {
		/* FIXME: This means there're some problems with alsa. */
		return FALSE;
	}

	return TRUE;
}


static void update_gui(struct channel *c)
{
	gtk_image_set_from_file(GTK_IMAGE(c->image), mixer_getmute(c) ?
			ICON_PATH "speaker.png" : ICON_PATH "mute.png");

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(c->pbar),
						(gdouble)mixer_get(c) / 100);
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
#endif


GdkFilterReturn event_callback(GdkXEvent *gdk_xev, GdkEvent *gdk_ev, gpointer data)
{
	XEvent *xev = (XEvent *) gdk_xev;
	XKeyEvent *xke;
	gdouble val;

	val = 100.0 * gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(ch.pbar));

	switch (xev->type) {
		printf("event: %d\n", xev->type);
		case Expose:
			break;

		case KeyPress:
			xke = (XKeyEvent *)xev;
			if (xke->keycode == raise_vol_kc) {
				if (val < 100 - STEP)
					val += STEP;		
				else
					val = 100;
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ch.pbar), val / 100);
				mixer_set(&ch, val);
			} else if (xke->keycode == lower_vol_kc) {
				if (val > STEP)
					val -= STEP;
				else
					val = 0;
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ch.pbar), val / 100);
				mixer_set(&ch, val);
			} else if (xke->keycode == mute_kc) {
				int val = mixer_getmute(&ch);
				mixer_setmute(&ch, !val);
				update_gui(&ch);
			}
		/*case KeyRelease:
			printf("key release\n");*/

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

	ch.hbox = gtk_hbox_new(FALSE, 5);
	ch.image = gtk_image_new_from_file(ICON_PATH "speaker.png");
	ch.pbar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), ch.hbox);
	gtk_box_pack_start(GTK_BOX(ch.hbox), ch.image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ch.hbox), ch.pbar, TRUE, TRUE, 0);

	update_gui(&ch);

	grab_audio_keys();

	gtk_widget_show_all(window);
	gdk_window_raise(window->window);
	gdk_window_add_filter(NULL, event_callback, NULL);
	g_object_set(G_OBJECT(window), "accept-focus", FALSE, NULL);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

	gtk_main();

	return 0;
}

/*
 * This is a silly panel application that executes arbitrary commands
 * when the buttons are pressed.
 *
 * Config file line format is: <command>^<icon path>
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <getopt.h>

#include "tray.h"

#define ICON_PATH ICON_DIR "/tray_buttons/"
#define MAX_BUTTONS 16

struct button_data {
	GtkWidget *gtk_button;
	char *command;
};

struct button_data buttons[MAX_BUTTONS];
GtkWidget *window;
GtkWidget *box;
GtkWidget *menu, *item;
int num_buttons;
int vertical;
int direct;
int center;
char *config = "/etc/buttons.conf";
int debug = 0;
gboolean can_use_alpha = FALSE;
double background_alpha = 1.0;

static gboolean window_exposed(GtkWidget *widget, GdkEventExpose *event,
			       gpointer data)
{
	cairo_t *cr = gdk_cairo_create(widget->window);

	if (can_use_alpha)
	    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, background_alpha);
	else
	    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_destroy(cr);
	return FALSE;
}

static void screen_changed(GtkWidget *widget, GdkScreen *old_screen,
			   gpointer data)
{
	GdkScreen *screen = gtk_widget_get_screen(widget);
	GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

	if (!colormap) {
	    fprintf(stderr, "Your screen does not support alpha.\n");
	    colormap = gdk_screen_get_rgb_colormap(screen);
	    can_use_alpha = FALSE;
	} else {
	    can_use_alpha = TRUE;
	}
	gtk_widget_set_colormap(widget, colormap);
}

static void option(GtkButton *button, gpointer data)
{
	struct button_data *bdata = (struct button_data*)data;
	//printf("button %d/%d, execute command: \"%s\"\n", n, num_buttons,
	//       bdata->command);
	system(bdata->command);
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

static void click()
{
	if (GTK_WIDGET_VISIBLE(window)) {
		gtk_widget_hide_all(window);
	} else {
		gtk_widget_show_all(window);
		//gdk_pointer_grab(window->window, TRUE, 0, NULL, NULL, GDK_CURRENT_TIME);
		//gdk_keyboard_grab(window->window, TRUE, GDK_CURRENT_TIME);
		gdk_window_raise(window->window);
		gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	}
}

static void set_button(struct button_data *bdata, char *i, char *tooltip)
{
	bdata->gtk_button = gtk_button_new();
	g_signal_connect(G_OBJECT(bdata->gtk_button), "clicked",
			 G_CALLBACK(option), (gpointer)(bdata));
	gtk_button_set_image(GTK_BUTTON(bdata->gtk_button),
			     gtk_image_new_from_file(i));
	gtk_button_set_image_position(GTK_BUTTON(bdata->gtk_button),
				      GTK_POS_TOP);
	if (tooltip)
		gtk_widget_set_tooltip_text(bdata->gtk_button, tooltip);
	gtk_container_add(GTK_CONTAINER(box), bdata->gtk_button);
}

#define LINE_SIZE 256

static int read_config(char *conf)
{
	char line[LINE_SIZE];
	FILE *f;
	int i;

	f = fopen(conf, "r");
	if (f == NULL)
		return -1;

	for (i = 0; i < MAX_BUTTONS; i++) {
		char *cmd, *icon, *tooltip;

		fgets(line, LINE_SIZE, f);
		if (feof(f))
			break;

		cmd = strtok(line, "^");
		icon = strtok(NULL, "^\n");
		tooltip = strtok(NULL, "\n");
		if (!cmd || !icon) {
			fprintf(stderr, "Error parsing config file line %d\n",
				i+1);
			return -1;
		}

		if (debug)
			printf("cmd=\"%s\", icon=\"%s\", tooltip=\"%s\"\n",
			       cmd, icon, tooltip);

		buttons[i].command = strdup(cmd);
		set_button(&buttons[i], icon, tooltip);
	}

	num_buttons = i;
	fclose(f);

	return 0;
}

int main(int argc, char **argv)
{
	struct GtkStatusIcon *icon1 = NULL;
	GtkSettings *settings;
	GError *error = NULL;
	GOptionContext *context;
	static GOptionEntry entries[] = {
		{ "debug", 'D', 0, G_OPTION_ARG_NONE, &debug,
				"Print debug information", NULL },
		{ "direct", 'd', 0, G_OPTION_ARG_NONE, &direct,
				"Don't show tray icon", NULL },
		{ "center", 'c', 0, G_OPTION_ARG_NONE, &center,
				"Center button panel", NULL },
		{ "config", 'f', 0, G_OPTION_ARG_FILENAME, &config,
				"Use this configuration file", "name" },
		{ "vertical", 'v', 0, G_OPTION_ARG_NONE, &vertical,
				"Use vertical icon bar", NULL },
		{ "alpha", 'a', 0, G_OPTION_ARG_DOUBLE, &background_alpha,
				"Background window alpha", NULL },
		{ NULL }
	};

	bindtextdomain("tray_buttons", LOCALE_DIR);
	textdomain("tray_buttons");

	direct = 0;		/* display window without tray icon */
	vertical = 0;		/* horizontal or vertical bar */
	num_buttons = 0;	/* number of buttons in bar */
	center = 0;		/* center button panel */

	context = g_option_context_new("- panel with configurable buttons");
	g_option_context_add_main_entries(context, entries, "tray_buttons");
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_parse(context, &argc, &argv, &error);

	gtk_init(&argc, &argv);

	/* tray icon stuff */
	if (!direct) {
		icon1 = (struct GtkStatusIcon *)
			gtk_status_icon_new_from_file(ICON_PATH "key.png");
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
	}
	/* end of tray icon stuff */

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* next 4 functions are needed so we can set the transparency */
	gtk_widget_set_app_paintable(window, TRUE);
	g_signal_connect(G_OBJECT(window), "expose-event",
			 G_CALLBACK(window_exposed), NULL);
	g_signal_connect(G_OBJECT(window), "screen-changed",
			 G_CALLBACK(screen_changed), NULL);
	screen_changed(window, NULL, NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	box = vertical ? gtk_vbox_new(TRUE, TRUE) : gtk_hbox_new(TRUE, TRUE);

	gtk_container_add(GTK_CONTAINER(window), box);

	if (read_config(config) < 0) {
		fprintf(stderr, "can't read config file %s\n", config);
		exit(1);
	}

	if (num_buttons == 0) {
		fprintf(stderr, "no buttons configured\n");
		exit(1);
	}

	settings = gtk_settings_get_default();
	g_object_set (settings, "gtk-button-images", TRUE, NULL);

	if (center) {
		gtk_window_set_position(GTK_WINDOW(window),
					GTK_WIN_POS_CENTER_ALWAYS);
	}

	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window), _("Buttons"));

	if (direct) {
		click();
	} else {
		gtk_status_icon_set_visible(GTK_STATUS_ICON(icon1), TRUE);
	}

	gtk_main();

	return 0;
}

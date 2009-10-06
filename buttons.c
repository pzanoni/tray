#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <getopt.h>

#include "tray.h"

#define MAX_BUTTONS 16

GtkWidget *window;
GtkWidget *button[MAX_BUTTONS];
GtkWidget *box;
GtkWidget *menu, *item;
char *command[MAX_BUTTONS];
int num_buttons;
int direction;
char *config = "/etc/buttons.conf";
int debug = 0;


static void option(GtkButton *button, int n)
{
	//printf("button %d/%d, execute command: \"%s\"\n", n, num_buttons, command[n]);
	if (n < num_buttons)
		system(command[n]);
}
	 
static void click()
{
	gtk_widget_show_all(window);
	//gdk_pointer_grab(window->window, TRUE, 0, NULL, NULL, GDK_CURRENT_TIME);
	//gdk_keyboard_grab(window->window, TRUE, GDK_CURRENT_TIME);
	gdk_window_raise(window->window);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
}

static void set_button(GtkWidget **b, int p, char *i)
{
	*b = gtk_button_new();
	g_signal_connect(G_OBJECT(*b), "clicked", G_CALLBACK(option),
							(gpointer)(p));
	gtk_button_set_image(GTK_BUTTON(*b), gtk_image_new_from_file(i));
	gtk_button_set_image_position(GTK_BUTTON(*b), GTK_POS_TOP);
	gtk_container_add(GTK_CONTAINER(box), *b);
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
		char *cmd, *icon;

		fgets(line, LINE_SIZE, f);
		if (feof(f))
			break;

		cmd = strtok(line, "^");
		icon = strtok(NULL, "\n");

		if (debug)
			printf("cmd=\"%s\", icon=\"%s\"\n", cmd, icon);

		command[i] = strdup(cmd);
		set_button(&button[i], i, icon);
	}

	num_buttons = i;
	fclose(f);

	return 0;
}

int main(int argc, char **argv)
{
	GtkSettings *settings;
	int o;

	bindtextdomain("tray_buttons", LOCALE_DIR);
	textdomain("tray_buttons");

	direction = 0;
	num_buttons = 0;

	while ((o = getopt(argc, argv, "df:v")) >= 0) {
		switch (o) {
		case 'd':
			debug++;
			break;
		case 'f':
			config = optarg;
			break;
		case 'v':
			direction = 1;
			break;
		}
	}

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	box = direction ? gtk_vbox_new(TRUE, TRUE) : gtk_hbox_new(TRUE, TRUE);

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

	//gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_title(GTK_WINDOW(window), N_("Quit session"));

	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_new_from_file("/usr/share/icons/gnome/scalable/actions/gtk-quit.svg", NULL);
	gtk_window_set_icon(GTK_WINDOW(window), pixbuf);

	click();

	gtk_main();

	return 0;
}

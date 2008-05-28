#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/signal.h>

#define ICON_PATH "/usr/share/icons/tray_reboot/"
//#define ICON_PATH "icons/"

GtkWidget *window;
GtkWidget *button1;
GtkWidget *button2;
GtkWidget *button3;
GtkWidget *button4;
GtkWidget *button5;
GtkWidget *vbox;
GtkWidget *hbox1, *hbox2;

enum {
	OPT_REBOOT,
	OPT_SHUTDOWN,
	OPT_SUSPEND,
	OPT_HIBERNATE,
	OPT_CANCEL
};

void option(GtkButton *button, int n)
{
	gtk_widget_hide_all(window);

	switch (n) {
	case OPT_REBOOT:
		system("shutdown_helper 1");
		break;
	case OPT_SHUTDOWN:
		system("shutdown_helper 2");
		break;
	case OPT_SUSPEND:
		system("pm-suspend");
		break;
	case OPT_HIBERNATE:
		system("pm-hibernate");
		break;
	case OPT_CANCEL:
		break;
	}
}
	 
void click()
{
	gtk_widget_show_all(window);
}

#ifdef USE_SCREENSAVER
void lock()
{
	system("xscreensaver -lock");
}
#endif

#define set_button(b,l,p,i) do { \
  (b) = gtk_button_new_with_label(l); \
  g_signal_connect(G_OBJECT(b), "clicked", G_CALLBACK(option), (gpointer)(p)); \
  gtk_button_set_image(GTK_BUTTON(b), gtk_image_new_from_file(ICON_PATH i)); \
  gtk_button_set_image_position(GTK_BUTTON(b), GTK_POS_TOP); \
} while (0)

int main(int argc, char **argv)
{
	struct GtkStatusIcon *icon1;
#ifdef USE_SCREENSAVER
	struct GtkStatusIcon *icon2;
#endif

	gtk_init(&argc, &argv);
	icon1 = (struct GtkStatusIcon *)gtk_status_icon_new_from_file(ICON_PATH "exit.png");
	g_signal_connect(G_OBJECT(icon1), "activate", G_CALLBACK(click), NULL);

#ifdef USE_SCREENSAVER
	icon2 = (struct GtkStatusIcon *)gtk_status_icon_new_from_file(ICON_PATH "lock.png");
	g_signal_connect(G_OBJECT(icon2), "activate", G_CALLBACK(lock), NULL);
#endif

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 30);
	vbox = gtk_vbox_new(TRUE, TRUE);
	hbox1 = gtk_hbox_new(TRUE, TRUE);
	hbox2 = gtk_hbox_new(TRUE, TRUE);

	set_button(button1, "Reboot", OPT_REBOOT, "reboot.png");
	set_button(button2, "Shutdown", OPT_SHUTDOWN, "shutdown.png");
	set_button(button3, "Suspend", OPT_SUSPEND, "suspend.png");
	set_button(button4, "Hibernate", OPT_HIBERNATE, "hibernate.png");
	set_button(button5, "Cancel", OPT_CANCEL, "cancel.png");

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), hbox1);
	gtk_container_add(GTK_CONTAINER(vbox), hbox2);

	gtk_container_add(GTK_CONTAINER(hbox1), button1);
	gtk_container_add(GTK_CONTAINER(hbox1), button2);
	gtk_container_add(GTK_CONTAINER(hbox2), button3);
	gtk_container_add(GTK_CONTAINER(hbox2), button4);
	gtk_container_add(GTK_CONTAINER(vbox), button5);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

	gtk_status_icon_set_visible(GTK_STATUS_ICON(icon1), TRUE);
#ifdef USE_SCREENSAVER
	gtk_status_icon_set_visible(GTK_STATUS_ICON(icon2), TRUE);
#endif

	gtk_main();
}

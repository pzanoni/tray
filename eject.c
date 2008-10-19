
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#define ICON_PATH ICON_DIR "/tray_eject/"
#define MEDIA_DIR "/media/"
#define CMD_SIZE 256

/* Originally based on wmvolman by Sir Raorn */


GHashTable *dev;
GtkWidget *menu, *item, *sep;
GtkStatusIcon *icon;
int count;

struct mdev {
	GtkWidget *item;
	char *mountpoint;
};


void update_status()
{
	if (count) {
		gtk_widget_show(sep);
		gtk_widget_set_sensitive(item, TRUE);
		gtk_status_icon_set_from_file(icon, ICON_PATH "dev1.png");
	} else {
		gtk_widget_hide(sep);
		gtk_widget_set_sensitive(item, FALSE);
		gtk_status_icon_set_from_file(icon, ICON_PATH "dev0.png");
	}
}

void key_destroy(gpointer data)
{
	free(data);
}

void value_destroy(gpointer data)
{
	struct mdev *m = (struct mdev *)data;

	gtk_widget_destroy(m->item);
	free(m);
	count--;
}


void add_mount(const char *udi, char *mountpoint)
{
	struct mdev *m;
	char txt[80];

	if ((m = malloc(sizeof(*m))) == NULL) {
		perror("malloc");
		return;
	}

	if ((m->mountpoint = strdup(mountpoint)) == NULL)
		return;

	snprintf(txt, 80, "%s %s", "Remove", mountpoint + strlen(MEDIA_DIR));
	m->item = gtk_menu_item_new_with_label(txt);
        gtk_widget_show(m->item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m->item);
	count++;

	g_hash_table_insert(dev, strdup(udi), m);
}

void remove_mount(const char *udi)
{
	g_hash_table_remove(dev, udi);
}


static void popup()
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
					gtk_get_current_event_time());
}

int eject(char *device, char *mountpoint)
{
	char line[CMD_SIZE];

	snprintf(line, CMD_SIZE, "eject %s", device);
	return system(line);
}

static void hal_property_modified(LibHalContext *ctx, const char *udi,
	  const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{

	if (!strcmp(key, "volume.is_mounted")) {
		if (libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", NULL)) {
			char *mountpoint = libhal_device_get_property_string(
					ctx, udi, "volume.mount_point", NULL);
			
			if (!strncmp(MEDIA_DIR, mountpoint, strlen(MEDIA_DIR))) {
				add_mount(udi, mountpoint);
			}

			if (mountpoint)
				libhal_free_string(mountpoint);
		} else {
			remove_mount(udi);
		}

		update_status();
	}
}

int init_hal(void)
{
	LibHalContext *ctx;
	DBusError error;
	DBusConnection *dbus_connection;
	char **devices, **volumes;
	int i, num;

	if (!(ctx = libhal_ctx_new())) {
		fprintf(stderr, "can't initialize\n");
		return -1;
	}

	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set(&error)) {
		fprintf(stderr, "%s\n", error.message);
		dbus_error_free(&error);
		return -1;
	}

	dbus_connection_setup_with_g_main(dbus_connection, NULL);
	libhal_ctx_set_dbus_connection(ctx, dbus_connection);
	libhal_ctx_set_device_property_modified(ctx, hal_property_modified);

	if (!libhal_device_property_watch_all(ctx, &error)) {
		fprintf(stderr, "%s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return -1;
	}

	if (!libhal_ctx_init(ctx, &error)) {
		fprintf(stderr, "%s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return -1;
	}

	if (!(devices = libhal_get_all_devices(ctx, &num, &error))) {
		fprintf(stderr, "%s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return -1;
	}

	libhal_free_string_array(devices);

	volumes = libhal_find_device_by_capability(ctx, "volume", &num, &error);
	if (dbus_error_is_set(&error)) {
		printf("can't find volume devices: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return -1;
	}

	for (i = 0; i < num; i++) {
		char *udi = volumes[i];

		if (libhal_device_property_exists(ctx, udi, "volume.is_mounted",
			 NULL) && libhal_device_get_property_bool(ctx, udi,
			"volume.is_mounted", NULL))
		{
			hal_property_modified(ctx, udi, "volume.is_mounted",
					      FALSE, FALSE);
			hal_property_modified(ctx, udi, "volume.mount_point",
					      FALSE, FALSE);
		}
	}

	libhal_free_string_array(volumes);

	return 0;
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	count = 0;
	dev = g_hash_table_new_full(g_str_hash, g_str_equal, key_destroy,
							value_destroy);

	icon = (GtkStatusIcon *)
                        gtk_status_icon_new_from_file(ICON_PATH "dev0.png");
	item = gtk_menu_item_new_with_label("Remove all");
	sep = gtk_separator_menu_item_new();
	menu = gtk_menu_new();
	gtk_widget_show(item);
	gtk_widget_show(sep);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
	g_signal_connect(G_OBJECT(icon), "popup-menu",
						G_CALLBACK(popup), NULL);

	if (init_hal() < 0)
		return 1;

	gtk_main();

	return 0;
}

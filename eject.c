
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#define ICON_PATH ICON_DIR "/tray_reboot/"
#define CMD_SIZE 256

GHashTable *dev;
GtkWidget *menu, *item;
struct GtkStatusIcon *icon;

struct mdev {
	GtkWidget *item;
	char *mountpoint;
};


void key_destroy(gpointer data)
{
	free(data);
}

void value_destroy(gpointer data)
{
	struct mdev *m = (struct mdev *)data;

	gtk_widget_destroy(m->item);
	free(m);
}


void add_mount(const char *udi, char *mountpoint)
{
	struct mdev *m;
	printf("udi = %s, mountpoint = %s\n", udi, mountpoint);

	if ((m = malloc(sizeof(*m))) == NULL) {
		perror("malloc");
		return;
	}

	if ((m->mountpoint = strdup(mountpoint)) == NULL) {
		return;
	}

	m->item = gtk_menu_item_new_with_label(mountpoint);
        gtk_widget_show(m->item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), m->item);

	g_hash_table_insert(dev, strdup(udi), m);
}

void remove_mount(const char *udi)
{
	g_hash_table_remove(dev, udi);
#if 0
	struct mdev *m;

	show_table();
	if ((m = g_hash_table_lookup(dev, udi))) {
		printf("mp = %s\n", m->mountpoint);
	}
#endif
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
				  const char *key, dbus_bool_t is_removed,
				  dbus_bool_t is_added)
{

	if (!strcmp(key, "volume.is_mounted")) {
		if (libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", NULL)) {
			char *mountpoint = libhal_device_get_property_string(
					ctx, udi, "volume.mount_point", NULL);
			
			if (!strncmp("/media", mountpoint, strlen("/media"))) {
				add_mount(udi, mountpoint);
			}

			if (mountpoint)
				libhal_free_string(mountpoint);
		} else {
			remove_mount(udi);
		}
	}
}

gboolean hal_init(void)
{
	LibHalContext *ctx;
	DBusError error;
	DBusConnection *dbus_connection;
	char **devices;
	char **volumes;
	char *udi;
	int i;
	int nr;

	if (!(ctx = libhal_ctx_new())) {
		printf("failed to initialize HAL!\n");
		return FALSE;
	}

	dbus_error_init(&error);

	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set(&error)) {
		printf("hal_initialize failed: %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	dbus_connection_setup_with_g_main(dbus_connection, NULL);
	libhal_ctx_set_dbus_connection(ctx, dbus_connection);

	libhal_ctx_set_device_property_modified(ctx, hal_property_modified);

	if (!libhal_device_property_watch_all(ctx, &error)) {
		printf("failed to watch all HAL properties!: %s\n",
		     error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return FALSE;
	}

	if (!libhal_ctx_init(ctx, &error)) {
		printf("hal_initialize failed: %s\n", error.message);
		dbus_error_free(&error);
		libhal_ctx_free(ctx);
		return FALSE;
	}

	/*
	 * Do something to ping the HAL daemon - the above functions will
	 * succeed even if hald is not running, so long as DBUS is.  But we
	 * want to exit silently if hald is not running, to behave on
	 * pre-2.6 systems.
	 */
	devices = libhal_get_all_devices(ctx, &nr, &error);
	if (!devices) {
		printf("seems that HAL is not running: %s\n", error.message);
		dbus_error_free(&error);

		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return FALSE;
	}
	libhal_free_string_array(devices);

	volumes = libhal_find_device_by_capability(ctx, "volume", &nr, &error);
	if (dbus_error_is_set(&error)) {
		printf("could not find volume devices: %s\n", error.message);
		dbus_error_free(&error);

		libhal_ctx_shutdown(ctx, NULL);
		libhal_ctx_free(ctx);
		return FALSE;
	}

	for (i = 0; i < nr; i++) {
		udi = volumes[i];

		if (libhal_device_property_exists(ctx, udi, "volume.is_mounted", NULL) && libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", NULL)) {
			hal_property_modified(ctx, udi, "volume.is_mounted",
					      FALSE, FALSE);
			hal_property_modified(ctx, udi, "volume.mount_point",
					      FALSE, FALSE);
		}
	}

	libhal_free_string_array(volumes);

	return TRUE;
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);

	dev = g_hash_table_new_full(g_str_hash, g_str_equal, key_destroy, value_destroy);


	icon = (struct GtkStatusIcon *)
                        gtk_status_icon_new_from_file(ICON_PATH "exit.png");
	item = gtk_menu_item_new_with_label("Bla");
	menu = gtk_menu_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(icon), "popup-menu",
						G_CALLBACK(popup), NULL);


	hal_init();
	gtk_main();

	return 0;
}

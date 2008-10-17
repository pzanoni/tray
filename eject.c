
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#define CMD_SIZE 256

//GHashtable *dev;


int eject(char *device, char *mountpoint)
{
	char line[CMD_SIZE];

	snprintf(line, CMD_SIZE, "eject %s", device);
	return system(line);
}

static dbus_bool_t hal_mainloop_integration(LibHalContext * ctx,
					    DBusError * error)
{
	DBusConnection *dbus_connection;
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, error);

	if (dbus_error_is_set(error))
		return FALSE;

	dbus_connection_setup_with_g_main(dbus_connection, NULL);
	libhal_ctx_set_dbus_connection(ctx, dbus_connection);

	return TRUE;
}

static void hal_device_removed(LibHalContext * ctx, const char *udi)
{
	printf("remove_volume: %s\n", udi);
}

static void hal_property_modified(LibHalContext * ctx, const char *udi,
				  const char *key, dbus_bool_t is_removed,
				  dbus_bool_t is_added)
{
	if (!strcmp(key, "volume.is_mounted")) {
		char *mountpoint = libhal_device_get_property_string(ctx, udi,
					  "volume.mount_point", NULL);
		printf(":%s: %s\n", mountpoint, udi);
		if (mountpoint)
			libhal_free_string(mountpoint);
	}
}

gboolean do_hal_init(void)
{
	LibHalContext *ctx;
	DBusError error;
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
	if (!hal_mainloop_integration(ctx, &error)) {
		printf("hal_initialize failed: %s\n", error.message);
		dbus_error_free(&error);
		return FALSE;
	}

	//libhal_ctx_set_device_added(ctx, hal_device_added);
	libhal_ctx_set_device_removed(ctx, hal_device_removed);
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

int main()
{
	static GMainLoop *loop;

	//dev = g_hash_table_new(g_str_hash, g_str_equal);

	do_hal_init();
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	return 0;
}

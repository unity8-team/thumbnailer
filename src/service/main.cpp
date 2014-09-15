#include <cstdio>
#include <memory>
#include <stdexcept>
#include <glib.h>
#include <gio/gio.h>

#include "dbusinterface.h"

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";

static std::unique_ptr<GMainLoop,void(*)(GMainLoop*)> main_loop(
    g_main_loop_new(nullptr, FALSE), g_main_loop_unref);

static void nameLost(GDBusConnection *, const char *, void *) {
    fprintf(stderr, "Could no acquire D-Bus name %s.  Quitting.\n", BUS_NAME);
    g_main_loop_quit(main_loop.get());
}

int main(int /*argc*/, char **/*argv*/) {

    GError *error = nullptr;
    std::unique_ptr<GDBusConnection, void(*)(void*)> bus(
        g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error),
        g_object_unref);
    if (error != nullptr) {
        fprintf(stderr, "Failed to connect to session bus: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    DBusInterface dbus(bus.get(), BUS_PATH);

    unsigned int name_id = g_bus_own_name_on_connection(
        bus.get(), BUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE,
        nullptr, &nameLost, nullptr, nullptr);

    g_main_loop_run(main_loop.get());

    if (name_id != 0) {
        g_bus_unown_name(name_id);
    }

    return 0;
}

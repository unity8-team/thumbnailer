/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * Authors:
 *    James Henstridge <james.henstridge@canonical.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dbusinterface.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
#include <thread>
#include <fcntl.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <thumbnailer.h>

#include <internal/gobj_memory.h>
#include "dbus-generated.h"

using namespace std;

static const char ART_ERROR[] = "com.canonical.MediaScanner2.Error.Failed";

struct DBusInterfacePrivate {
    const std::string bus_path;
    unique_gobj<GDBusConnection> bus;
    unique_gobj<TNThumbnailer> iface;
    std::vector<gulong> handler_id;
    std::shared_ptr<Thumbnailer> thumbnailer;

    DBusInterfacePrivate(GDBusConnection *g_bus, const std::string& bus_path)
        : bus_path(bus_path),
          bus(static_cast<GDBusConnection*>(g_object_ref(g_bus))),
          iface(tn_thumbnailer_skeleton_new()),
          thumbnailer(std::make_shared<Thumbnailer>()) {
        handler_id = {
            g_signal_connect(
                    iface.get(), "handle-get-album-art",
                    G_CALLBACK(&DBusInterfacePrivate::handleGetAlbumArt), this),
            g_signal_connect(
                    iface.get(), "handle-get-artist-art",
                    G_CALLBACK(&DBusInterfacePrivate::handleGetArtistArt), this)
        };

        GError *error = nullptr;
        if (!g_dbus_interface_skeleton_export(
                G_DBUS_INTERFACE_SKELETON(iface.get()), bus.get(),
                bus_path.c_str(), &error)) {
            string errortxt(error->message);
            g_error_free(error);

            string msg = "Failed to export interface: ";
            msg += errortxt;
            throw runtime_error(msg);
        }
    }

    ~DBusInterfacePrivate() {
        g_dbus_interface_skeleton_unexport(
            G_DBUS_INTERFACE_SKELETON(iface.get()));
        for (auto const id: handler_id) {
            g_signal_handler_disconnect(iface.get(), id);
        }
    }

    static ThumbnailSize desiredSizeFromString(const char *size)
    {
        if (!strcmp(size, "small")) {
            return TN_SIZE_SMALL;
        } else if (!strcmp(size, "large")) {
            return TN_SIZE_LARGE;
        } else if (!strcmp(size, "xlarge")) {
            return TN_SIZE_XLARGE;
        } else if (!strcmp(size, "original")) {
            return TN_SIZE_ORIGINAL;
        }
        std::string error("Unknown size: ");
        error += size;
        throw std::logic_error(error);
    }

    static gboolean handleGetAlbumArt(TNThumbnailer *iface, GDBusMethodInvocation *invocation, GUnixFDList *, const char *artist, const char *album, const char *size, void *user_data) {
        auto p = static_cast<DBusInterfacePrivate*>(user_data);
        fprintf(stderr, "Look up cover art for %s/%s at size %s\n", artist, album, size);

        ThumbnailSize desiredSize;
        try {
            desiredSize = desiredSizeFromString(size);
        } catch (const std::logic_error& error) {
            g_dbus_method_invocation_return_dbus_error(
                    invocation, ART_ERROR, error.what());
            return TRUE;
        }

        try {
            std::thread t(&getAlbumArt,
                          unique_gobj<TNThumbnailer>(static_cast<TNThumbnailer*>(g_object_ref(iface))),
                          unique_gobj<GDBusMethodInvocation>(static_cast<GDBusMethodInvocation*>(g_object_ref(invocation))),
                          p->thumbnailer,
                          std::string(artist), std::string(album), desiredSize);
            t.detach();
        } catch (const std::exception &e) {
            g_dbus_method_invocation_return_dbus_error(
                invocation, ART_ERROR, e.what());
        }
        return TRUE;
    }

    static gboolean handleGetArtistArt(TNThumbnailer *iface, GDBusMethodInvocation *invocation, GUnixFDList *, const char *artist, const char *album,
            const char *size, void *user_data) {
        auto p = static_cast<DBusInterfacePrivate*>(user_data);
        fprintf(stderr, "Look up artist art for %s/%s at size %s\n", artist, album, size);

        ThumbnailSize desiredSize;
        try {
            desiredSize = desiredSizeFromString(size);
        } catch (const std::logic_error& error) {
            g_dbus_method_invocation_return_dbus_error(
                    invocation, ART_ERROR, error.what());
            return TRUE;
        }

        try {
            std::thread t(&getArtistArt,
                          unique_gobj<TNThumbnailer>(static_cast<TNThumbnailer*>(g_object_ref(iface))),
                          unique_gobj<GDBusMethodInvocation>(static_cast<GDBusMethodInvocation*>(g_object_ref(invocation))),
                          p->thumbnailer, std::string(artist), std::string(album), desiredSize);
            t.detach();
        } catch (const std::exception &e) {
            g_dbus_method_invocation_return_dbus_error(
                invocation, ART_ERROR, e.what());
        }
        return TRUE;
    }

    static void getAlbumArt(unique_gobj<TNThumbnailer> iface,
                            unique_gobj<GDBusMethodInvocation> invocation,
                            std::shared_ptr<Thumbnailer> thumbnailer,
                            const std::string artist, const std::string album,
                            ThumbnailSize desiredSize) {
        std::string art;
        try {
            art = thumbnailer->get_album_art(
                artist, album, desiredSize, TN_REMOTE);
        } catch (const std::exception &e) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, e.what());
            return;
        }

        if (art.empty()) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, "Could not get thumbnail");
            return;
        }
        int fd = open(art.c_str(), O_RDONLY);
        if (fd < 0) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, strerror(errno));
            return;
        }

        unique_gobj<GUnixFDList> fd_list(g_unix_fd_list_new());
        GError *error = nullptr;
        g_unix_fd_list_append(fd_list.get(), fd, &error);
        close(fd);
        if (error != nullptr) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, error->message);
            g_error_free(error);
            return;
        }

        tn_thumbnailer_complete_get_album_art(
            iface.get(), invocation.get(), fd_list.get(), g_variant_new_handle(0));
    }

    static void getArtistArt(unique_gobj<TNThumbnailer> iface,
                            unique_gobj<GDBusMethodInvocation> invocation,
                            std::shared_ptr<Thumbnailer> thumbnailer,
                            const std::string artist, const std::string album,
                            ThumbnailSize desiredSize) {
        std::string art;
        try {
            art = thumbnailer->get_artist_art(
                artist, album, desiredSize, TN_REMOTE);
        } catch (const std::exception &e) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, e.what());
            return;
        }

        if (art.empty()) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, "Could not get thumbnail");
            return;
        }
        int fd = open(art.c_str(), O_RDONLY);
        if (fd < 0) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, strerror(errno));
            return;
        }

        unique_gobj<GUnixFDList> fd_list(g_unix_fd_list_new());
        GError *error = nullptr;
        g_unix_fd_list_append(fd_list.get(), fd, &error);
        close(fd);
        if (error != nullptr) {
            g_dbus_method_invocation_return_dbus_error(
                invocation.get(), ART_ERROR, error->message);
            g_error_free(error);
            return;
        }

        tn_thumbnailer_complete_get_artist_art(
            iface.get(), invocation.get(), fd_list.get(), g_variant_new_handle(0));
    }
};

DBusInterface::DBusInterface(GDBusConnection *bus, const std::string& bus_path)
    : p(new DBusInterfacePrivate(bus, bus_path)) {
}

DBusInterface::~DBusInterface() {
    delete p;
}

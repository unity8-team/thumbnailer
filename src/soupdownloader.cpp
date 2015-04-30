/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#include<internal/soupdownloader.h>
#include<libsoup/soup.h>
#pragma GCC diagnostic pop
#include<internal/gobj_memory.h>
#include<stdexcept>

SoupDownloader::SoupDownloader() {
    session = soup_session_sync_new();
    if(!session) {
        throw std::runtime_error("Could not create Soup session.");
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
SoupDownloader::~SoupDownloader() {
    g_object_unref(G_OBJECT(session));
}
#pragma GCC diagnostic pop

std::string SoupDownloader::download(const std::string &url) {
    unique_gobj<SoupMessage> msg(soup_message_new("GET", url.c_str()));
    guint status;
    status = soup_session_send_message(session, msg.get());
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Determination failed.\n");
        return "";
    }
    std::string result(msg->response_body->data, msg->response_body->length);
    return result;
}


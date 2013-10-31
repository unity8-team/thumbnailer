/*
 * Copyright (C) 2013 Canonical Ltd.
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

#include<string>
#include<cstdio>
#include<libsoup/soup.h>

using namespace std;

void getImage() {
    const char *lastfmTemplate = "http://ws.audioscrobbler.com/1.0/album/%s/%s/info.xml";
    string artist = "The Prodigy";
    string album = "Music for the Jilted Generation";
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, lastfmTemplate, artist.c_str(), album.c_str());
    SoupSession *s = soup_session_sync_new();
    SoupMessage *msg = soup_message_new("GET", buf);
    guint status;
    status = soup_session_send_message(s, msg);
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Fail\n");
        return;
    }
    printf("%s\n", msg->response_body->data);
    g_object_unref(msg);
    g_object_unref(s);
}

int main(int /*argc*/, char **/*argv*/) {
    getImage();
    return 0;
}

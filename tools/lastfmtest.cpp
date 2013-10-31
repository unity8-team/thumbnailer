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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>

using namespace std;

string parseXML(const string &xml) {
    xmlDocPtr doc;
    xmlXPathContextPtr xpath_ctx;
    xmlXPathObjectPtr xpath_res;
    string node = "/album/coverart/large";
    doc = xmlReadMemory(xml.c_str(), xmlStrlen ((xmlChar*) xml.c_str()), NULL, NULL,
                          XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
     if (!doc) {
       return NULL;
     }
     xpath_ctx = xmlXPathNewContext (doc);
     xpath_res = xmlXPathEvalExpression ((xmlChar *) node.c_str(), xpath_ctx);
     if (!xpath_res) {
       xmlXPathFreeContext (xpath_ctx);
       xmlFreeDoc (doc);
       return "";
     }

     char *imageurl;
     if (xpath_res->nodesetval->nodeTab) {
       imageurl = (char *) xmlNodeListGetString (doc,
               xpath_res->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
     }
     string url(imageurl);
     g_free(imageurl);
     xmlXPathFreeObject (xpath_res);
     xmlXPathFreeContext (xpath_ctx);
     xmlFreeDoc (doc);
     return url;
}

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
    string xml(msg->response_body->data);
    string parsed = parseXML(xml);
    if(parsed.empty() ||
       parsed == "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png") {
        fprintf(stderr, "Could not find album art.\n");
        return;
    }
    printf("Result: %s\n", parsed.c_str());
    g_object_unref(msg);
    g_object_unref(s);
}

int main(int /*argc*/, char **/*argv*/) {
    getImage();
    return 0;
}

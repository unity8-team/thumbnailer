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
#include<memory>

using namespace std;

string parseXML(const string &xml) {
    string node = "/album/coverart/large";
    unique_ptr<xmlDoc, void(*)(xmlDoc*)> doc(
            xmlReadMemory(xml.c_str(), xmlStrlen ((xmlChar*) xml.c_str()), NULL, NULL,
                          XML_PARSE_RECOVER | XML_PARSE_NOBLANKS), xmlFreeDoc);
     if (!doc) {
       return "";
     }
     unique_ptr<xmlXPathContext, void(*)(xmlXPathContext*)>xpath_ctx(
             xmlXPathNewContext(doc.get()), xmlXPathFreeContext);

     unique_ptr<xmlXPathObject, void(*)(xmlXPathObject *)> xpath_res(
             xmlXPathEvalExpression((xmlChar *) node.c_str(), xpath_ctx.get()),
             xmlXPathFreeObject);
     if (!xpath_res) {
       return "";
     }

     char *imageurl;
     if (xpath_res->nodesetval->nodeTab) {
       imageurl = (char *) xmlNodeListGetString (doc.get(),
               xpath_res->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
     }
     string url(imageurl);
     g_free(imageurl);
     return url;
}

void getImage() {
    const char *lastfmTemplate = "http://ws.audioscrobbler.com/1.0/album/%s/%s/info.xml";
    string artist = "The Prodigy";
    string album = "Music for the Jilted Generation";
    string outputFile = "image.png";
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, lastfmTemplate, artist.c_str(), album.c_str());
    SoupSession *s = soup_session_sync_new();
    SoupMessage *msg = soup_message_new("GET", buf);
    guint status;
    status = soup_session_send_message(s, msg);
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Determination failed.\n");
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

    msg = soup_message_new("GET", parsed.c_str());
    status = soup_session_send_message(s, msg);
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Image download failed.\n");
        return;
    }
    FILE *f = fopen(outputFile.c_str(), "w");
    fwrite(msg->response_body->data, 1, msg->response_body->length, f);
    fclose(f);
    g_object_unref(msg);
    g_object_unref(s);
}

int main(int /*argc*/, char **/*argv*/) {
    getImage();
    return 0;
}

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

#include<internal/lastfmdownloader.h>

#include<internal/gobj_memory.h>
#include<memory>
#include<libsoup/soup.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>

const char *NOTFOUND_IMAGE = "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png";

using namespace std;

LastFMDownloader::LastFMDownloader() {

}

LastFMDownloader::~LastFMDownloader() {

}


string LastFMDownloader::parseXML(const string &xml) {
    string node = "/album/coverart/large";
    unique_ptr<xmlDoc, void(*)(xmlDoc*)> doc(
            xmlReadMemory(xml.c_str(), xmlStrlen ((xmlChar*) xml.c_str()), NULL, NULL,
                          XML_PARSE_RECOVER | XML_PARSE_NOBLANKS), xmlFreeDoc);
     if (!doc) {
       return "";
     }
     unique_ptr<xmlXPathContext, void(*)(xmlXPathContext*)> cntx(
             xmlXPathNewContext(doc.get()), xmlXPathFreeContext);

     unique_ptr<xmlXPathObject, void(*)(xmlXPathObject *)> path(
             xmlXPathEvalExpression((xmlChar *) node.c_str(),  cntx.get()),
             xmlXPathFreeObject);
     if (!path) {
       return "";
     }

     char *imageurl;
     if (path->nodesetval->nodeTab) {
       imageurl = (char *) xmlNodeListGetString(doc.get(),
               path->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
     }
     string url(imageurl);
     xmlFree(imageurl);
     return url;
}

bool LastFMDownloader::download(const std::string &artist,
        const std::string &album, const std::string &fname) {
    const char *lastfmTemplate = "http://ws.audioscrobbler.com/1.0/album/%s/%s/info.xml";
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, lastfmTemplate, artist.c_str(), album.c_str());
    unique_gobj<SoupSession> s(soup_session_sync_new());
    unique_gobj<SoupMessage> msg(soup_message_new("GET", buf));
    guint status;
    status = soup_session_send_message(s.get(), msg.get());
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Determination failed.\n");
        return false;
    }
    string xml(msg->response_body->data);
    string parsed = parseXML(xml);
    if(parsed.empty() ||
       parsed == NOTFOUND_IMAGE) {
        fprintf(stderr, "Could not find album art.\n");
        return false;
    }

    msg.reset( soup_message_new("GET", parsed.c_str()));
    status = soup_session_send_message(s.get(), msg.get());
    if(!SOUP_STATUS_IS_SUCCESSFUL(status)) {
        fprintf(stderr, "Image download failed.\n");
        return false;
    }
    FILE *f = fopen(fname.c_str(), "w");
    fwrite(msg->response_body->data, 1, msg->response_body->length, f);
    fclose(f);
    return true;
}

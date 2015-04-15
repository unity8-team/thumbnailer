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

#include <QImage>
#include <QFileInfo>
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>

#include <thumbnailer.h>

#include <internal/audioimageextractor.h>
#include <internal/imagescaler.h>
#include <internal/lastfmdownloader.h>
#include <internal/mediaartcache.h>
#include <internal/thumbnailcache.h>
#include <internal/ubuntuserverdownloader.h>
#include <internal/videoscreenshotter.h>

#include <libexif/exif-loader.h>

#include <gst/gst.h>

#include <cstring>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <random>
#include <stdexcept>

using namespace std;
using namespace unity::thumbnailer::internal;

void store_to_file(QByteArray const& content, QString const& file_path)
{
    // This method is probably not needed here after we modify the whole class
    QFile file(file_path);
    file.open(QIODevice::WriteOnly);
    file.write(content);
    file.close();
}

class ThumbnailerPrivate : public QObject
{
    Q_OBJECT
public:
    enum DownloadType
    {
        ARTIST_ART,
        ALBUM_ART
    };

    struct ActiveDownloadInfo
    {
        QString artist;
        QString album;
        DownloadType type;
    };

    ThumbnailerPrivate(QObject *parent=nullptr);

    std::string get_album_art(std::string const& artist,
                              std::string const& album,
                              ThumbnailSize desiredSize,
                              ThumbnailPolicy policy);

    std::string get_artist_art(std::string const& artist,
                               std::string const& album,
                               ThumbnailSize desiredSize,
                               ThumbnailPolicy policy);

    string create_audio_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy);
    string create_video_thumbnail(string const& abspath, ThumbnailSize desired_size);
    string create_generic_thumbnail(string const& abspath, ThumbnailSize desired_size);

    string create_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy);
    string create_random_filename();
    string extract_exif_thumbnail(std::string const& abspath);

    void download_art(DownloadType type, QString const& artist, QString const &album);

    random_device rnd;
    ThumbnailCache cache;
    AudioImageExtractor audio;
    VideoScreenshotter video;
    ImageScaler scaler;
    MediaArtCache macache;
    std::unique_ptr<QArtDownloader> downloader;
    QMap<QString, ActiveDownloadInfo> map_download_type;

public slots:
    void download_finished(QString, QByteArray const &data);
};

ThumbnailerPrivate::ThumbnailerPrivate(QObject *parent) : QObject(parent)
{
    char* artservice = getenv("THUMBNAILER_ART_PROVIDER");
    if (artservice != nullptr && strcmp(artservice, "lastfm") == 0)
    {
        // Disable LastFM by now
        // downloader.reset(new LastFMDownloader());
    }
    else
    {
        downloader.reset(new UbuntuServerDownloader());
    }

    connect(downloader.get(), &QArtDownloader::file_downloaded, this, &ThumbnailerPrivate::download_finished);
};

std::string ThumbnailerPrivate::get_album_art(std::string const& artist,
                                       std::string const& album,
                                       ThumbnailSize desired_size,
                                       ThumbnailPolicy policy)
{
    if (!macache.has_album_art(artist, album))
    {
//        std::string tmpname = tmpnam(filebuf);
        auto url = downloader->download_album(QString::fromStdString(artist), QString::fromStdString(album));

        return "";
////        std::string image = p_->downloader->download(artist, album);
//        if (!image.empty())
//        {
//            store_to_file(image, tmpname);
//        }
//        else
//        {
//            return image;
//        }
//        QFile qfile(QString::fromStdString(tmpname));
//        if (!qfile.open(QFile::ReadOnly))
//        {
//            // TODO throw exception
//        }
//        p_->macache.add_album_art(artist, album, qfile.readAll().constData(), qfile.size());
    }
    return "";
//    // At this point we know we have the image in our art cache (unless
//    // someone just deleted it concurrently, in which case we can't
//    // really do anything.
//    std::string original = p_->macache.get_album_art_file(artist, album);
//    if (desired_size == TN_SIZE_ORIGINAL)
//    {
//        return original;
//    }
//    return get_thumbnail(original, desired_size, policy);
}

std::string ThumbnailerPrivate::get_artist_art(std::string const& artist,
                                        std::string const& album,
                                        ThumbnailSize desired_size,
                                        ThumbnailPolicy policy)
{
    if (!macache.has_artist_art(artist, album))
    {
        if (policy == TN_LOCAL)
        {
            // We don't have it cached and can't access the net
            // -> nothing to be done.
            return "";
        }
        char filebuf[] = "/tmp/some/long/text/here/so/path/will/fit";
        std::string tmpname = tmpnam(filebuf);
        downloader->download_artist(QString::fromStdString(artist), QString::fromStdString(album));
        return "";
//
//        if (!image.empty())
//        {
//            store_to_file(image, tmpname);
//        }
//        else
//        {
//            return image;
//        }
//        QFile qfile(QString::fromStdString(tmpname));
//        if (!qfile.open(QFile::ReadOnly))
//        {
//            // TODO throw exception
//        }
//        p_->macache.add_artist_art(artist, album, qfile.readAll().constData(), qfile.size());
    }
//    // At this point we know we have the image in our art cache (unless
//    // someone just deleted it concurrently, in which case we can't
//    // really do anything.
//    std::string original = p_->macache.get_artist_art_file(artist, album);
//    if (desired_size == TN_SIZE_ORIGINAL)
//    {
//        return original;
//    }
//    return get_thumbnail(original, desired_size, policy);

    return "";
}

string ThumbnailerPrivate::create_random_filename()
{
    string fname;
    char* dirbase = getenv("TMPDIR");  // Set when in a confined application.
    if (dirbase)
    {
        fname = dirbase;
    }
    else
    {
        fname = "/tmp";
    }
    fname += "/thumbnailer.";
    fname += to_string(rnd());
    fname += ".tmp";
    return fname;
}

string ThumbnailerPrivate::extract_exif_thumbnail(std::string const& abspath)
{
    std::unique_ptr<ExifLoader, void (*)(ExifLoader*)> el(exif_loader_new(), exif_loader_unref);
    if (el)
    {
        exif_loader_write_file(el.get(), abspath.c_str());
        std::unique_ptr<ExifData, void (*)(ExifData* e)> ed(exif_loader_get_data(el.get()), exif_data_unref);
        if (ed && ed->data && ed->size)
        {
            auto outfile = create_random_filename();
            FILE* thumb = fopen(outfile.c_str(), "wb");
            if (thumb)
            {
                fwrite(ed->data, 1, ed->size, thumb);
                fclose(thumb);
                return outfile;
            }
        }
    }
    return "";
}

string ThumbnailerPrivate::create_audio_thumbnail(string const& abspath,
                                                  ThumbnailSize desired_size,
                                                  ThumbnailPolicy /*policy*/)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    bool extracted = false;
    try
    {
        if (audio.extract(abspath, tmpname))
        {
            extracted = true;
        }
    }
    catch (runtime_error& e)
    {
    }
    if (extracted)
    {
        scaler.scale(tmpname, tnfile, desired_size, abspath);  // If this throws, let it propagate.
        unlink(tmpname.c_str());
        return tnfile;
    }
    return "";
}
string ThumbnailerPrivate::create_generic_thumbnail(string const& abspath, ThumbnailSize desired_size)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    // Special case: full size image files are their own preview.
    QImage image;
    if (desired_size == TN_SIZE_ORIGINAL && image.load(QString::fromStdString(abspath)))
    {
        return abspath;
    }
    try
    {
        if (scaler.scale(abspath, tnfile, desired_size, abspath))
        {
            return tnfile;
        }
    }
    catch (runtime_error const& e)
    {
        fprintf(stderr, "Scaling thumbnail failed: %s\n", e.what());
    }
    return "";
}

string ThumbnailerPrivate::create_video_thumbnail(string const& abspath, ThumbnailSize desired_size)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    if (video.extract(abspath, tmpname))
    {
        scaler.scale(tmpname, tnfile, desired_size, abspath);
        unlink(tmpname.c_str());
        return tnfile;
    }
    throw runtime_error("Video extraction failed.");
}

string ThumbnailerPrivate::create_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy)
{
    // Every now and then see if we have too much stuff and delete them if so.
    if ((rnd() % 100) == 0)  // No, this is not perfectly random. It does not need to be.
    {
        cache.prune();
    }
    QFileInfo file_info(QString::fromStdString(abspath));
    if (!file_info.exists() || !file_info.isFile()) {
        // TODO throw exception as something weird happened
    }

    QMimeDatabase mimeDatabase;
    QMimeType mime = mimeDatabase.mimeTypeForFile(file_info);
    if(mime.name().contains("audio"))
    {
        return create_audio_thumbnail(abspath, desired_size, policy);
    }
    else if(mime.name().contains("video"))
    {
        return create_video_thumbnail(abspath, desired_size);
    }
    else
    {
        // TODO throw exception as file is not a video nor an image
    }
    if (desired_size != TN_SIZE_ORIGINAL)
    {
        try
        {
            auto embedded_image = extract_exif_thumbnail(abspath);
            // Note that we always use the embedded thumbnail if it exists.
            // Depending on usage, it may be too low res for our purposes.
            // If this becomes an issue, add a check here once we know the
            // real resolution requirements and fall back to scaling the
            // full image if the thumbnail is too small.
            if (!embedded_image.empty())
            {
                string tnfile = cache.get_cache_file_name(abspath, desired_size);
                auto succeeded = scaler.scale(embedded_image, tnfile, desired_size, abspath, abspath);
                unlink(embedded_image.c_str());
                if (succeeded)
                {
                    return tnfile;
                }
            }
        }
        catch (exception const& e)
        {
        }
    }

    return create_generic_thumbnail(abspath, desired_size);
}

void ThumbnailerPrivate::download_art(ThumbnailerPrivate::DownloadType type, QString const& artist, QString const &album)
{
    auto url = downloader->download_album(artist, album);

    ActiveDownloadInfo download_info;
    download_info.artist = artist;
    download_info.album = album;
    download_info.type = type;
    map_download_type[url] = download_info;
}

void ThumbnailerPrivate::download_finished(QString url, QByteArray const &data)
{
    QMap<QString, ActiveDownloadInfo>::iterator iter = map_download_type.find(url);
    if (iter != map_download_type.end())
    {
        if ((*iter).type == ALBUM_ART)
        {
            macache.add_album_art((*iter).artist.toStdString(), (*iter).album.toStdString(),
                    data.constData(), data.size());
        }
        else
        {
            macache.add_artist_art((*iter).artist.toStdString(), (*iter).album.toStdString(),
                                data.constData(), data.size());
        }
        map_download_type.erase(iter);
    }
}

Thumbnailer::Thumbnailer(QObject *parent) : QObject(parent), p_(new ThumbnailerPrivate())
{
}

Thumbnailer::~Thumbnailer() = default;

std::string Thumbnailer::get_thumbnail(std::string const& filename, ThumbnailSize desired_size, ThumbnailPolicy policy)
{
    string abspath;
    if (filename.empty())
    {
        return "";
    }
    if (filename[0] != '/')
    {
        auto cwd = getcwd(nullptr, 0);
        abspath += cwd;
        free(cwd);
        abspath += "/" + filename;
    }
    else
    {
        abspath = filename;
    }
    std::string estimate = p_->cache.get_if_exists(abspath, desired_size);
    if (!estimate.empty())
    {
        return estimate;
    }

    if (p_->cache.has_failure(abspath))
    {
        throw runtime_error("Thumbnail generation failed previously, not trying again.");
    }

    try
    {
        string generated = p_->create_thumbnail(abspath, desired_size, policy);
        if (generated == abspath)
        {
            return abspath;
        }
    }
    catch (std::runtime_error const& e)
    {
        p_->cache.mark_failure(abspath);
        throw;
    }

    return p_->cache.get_if_exists(abspath, desired_size);
}

string Thumbnailer::get_thumbnail(string const& filename, ThumbnailSize desired_size)
{
    return get_thumbnail(filename, desired_size, TN_LOCAL);
}

std::string Thumbnailer::get_album_art(std::string const& artist,
                                       std::string const& album,
                                       ThumbnailSize desired_size,
                                       ThumbnailPolicy policy)
{
    return p_->get_album_art(artist, album, desired_size, policy);
}

std::string Thumbnailer::get_artist_art(std::string const& artist,
                                        std::string const& album,
                                        ThumbnailSize desired_size,
                                        ThumbnailPolicy policy)
{
    return p_->get_artist_art(artist, album, desired_size, policy);
}

#include "thumbnailer.moc"


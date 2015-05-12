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
 *              Michi Henning <michi.henning@canonical.com>
 *              James Henstridge <james.henstridge@canonical.com>
 */

#include <thumbnailer.h>

#include <internal/artreply.h>
#include <internal/file_io.h>
#include <internal/gobj_memory.h>
#include <internal/image.h>
#include <internal/make_directories.h>
#include <internal/raii.h>
#include <internal/safe_strerror.h>
#include <internal/ubuntuserverdownloader.h>
#include <internal/videoscreenshotter.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <core/persistent_string_cache.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <gio/gio.h>
#pragma GCC diagnostic pop

#include <QTimer>

#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace
{

enum class Location
{
    local, remote
};

}

class ThumbnailerPrivate
{
public:
    core::PersistentStringCache::UPtr full_size_cache_;  // Small cache of full (original) size images.
    core::PersistentStringCache::UPtr thumbnail_cache_;  // Large cache of scaled images.
    core::PersistentStringCache::UPtr failure_cache_;    // Cache for failed attempts (value is always empty).
    unique_ptr<ArtDownloader> downloader_;

    ThumbnailerPrivate();
};

ThumbnailerPrivate::ThumbnailerPrivate()
    : downloader_(new UbuntuServerDownloader())
{
    string xdg_base = g_get_user_cache_dir();
    if (xdg_base == "")
    {
        string s("Thumbnailer(): Could not determine cache dir.");
        throw runtime_error(s);
    }

    string cache_dir = xdg_base + "/unity-thumbnailer";
    cerr << "cachedir: " << cache_dir << endl;
    make_directories(cache_dir, 0700);

    try
    {
        // TODO: No good to hard-wire the cache size.
        full_size_cache_ = core::PersistentStringCache::open(cache_dir + "/images", 50 * 1024 * 1024,
                                                             core::CacheDiscardPolicy::lru_only);
        thumbnail_cache_ = core::PersistentStringCache::open(cache_dir + "/thumbnails", 100 * 1024 * 1024,
                                                             core::CacheDiscardPolicy::lru_only);
        failure_cache_ = core::PersistentStringCache::open(cache_dir + "/failures", 2 * 1024 * 1024,
                                                           core::CacheDiscardPolicy::lru_ttl);
    }
    catch (std::exception const& e)
    {
        string s("Thumbnailer(): Cannot instantiate cache: ");
        s += e.what();
        throw runtime_error(s);
    }
}

namespace
{
class RequestBase : public ThumbnailRequest {
    Q_OBJECT
public:
    virtual ~RequestBase() = default;
    string thumbnail() override;
protected:
    enum class FetchStatus {
        NeedsDownload, Downloaded, NotFound, Error
    };
    enum class CachePolicy
    {
        cache_fullsize, dont_cache_fullsize
    };
    struct ImageData
    {
        FetchStatus status;
        string data;
        CachePolicy cache_policy;
        Location location;
    };

    RequestBase(shared_ptr<ThumbnailerPrivate> const& p, string const& key, QSize const& requested_size);
    virtual ImageData fetch() = 0;

    shared_ptr<ThumbnailerPrivate> p_;

private:
    string key_;
    QSize const requested_size_;
};

class LocalThumbnailRequest : public RequestBase {
    Q_OBJECT
public:
    LocalThumbnailRequest(shared_ptr<ThumbnailerPrivate> const& p, string const& filename, QSize const& requested_size);
protected:
    ImageData fetch() override;
    void download(std::chrono::milliseconds timeout) override;
private:
    string filename_;
    unique_ptr<VideoScreenshotter> screenshotter_;
};

class AlbumRequest : public RequestBase {
    Q_OBJECT
public:
    AlbumRequest(shared_ptr<ThumbnailerPrivate> const& p,
                 string const& artist,
                 string const& album,
                 QSize const& requested_size);
    virtual ~AlbumRequest();
protected:
    ImageData fetch() override;
    void download(std::chrono::milliseconds timeout) override;
private Q_SLOTS:
    void timeout();
private:
    string artist_;
    string album_;
    shared_ptr<ArtReply> artreply_;
    QTimer timer_;
};

class ArtistRequest : public RequestBase {
    Q_OBJECT
public:
    ArtistRequest(shared_ptr<ThumbnailerPrivate> const& p,
                  string const& artist,
                  string const& album,
                  QSize const& requested_size);
    virtual ~ArtistRequest();
protected:
    ImageData fetch() override;
    void download(std::chrono::milliseconds timeout) override;
private Q_SLOTS:
    void timeout();
private:
    string artist_;
    string album_;
    shared_ptr<ArtReply> artreply_;
    QTimer timer_;
};
}

RequestBase::RequestBase(shared_ptr<ThumbnailerPrivate> const& p, string const& key, QSize const& requested_size) :
    p_(p), key_(key), requested_size_(requested_size)
{
}

// Main look-up logic for thumbnails.
//
// key_ is set by the subclass to uniquely identify what is being
// thumbnailed.  For online art, this includes the artist and album.
// For local thumbnails, this includes the path name, inode, mtime,
// and ctime.
//
// We first look in the cache to see if we have a thumbnail already
// for the provided key and size.  If not, we check whether a
// full-size image was downloaded previously and is still hanging
// around. If no image is available in the full size cache, we call
// the fetch() routine (implemented by the subclass), which will
// either (a) report that the data needs to be downloaded, (b) return
// the full size image ready for scaling, or (c) report an error.
//
// If the data needs downloading, we return immediately.  Similarly,
// we return in case of an error.  If the data is available, it may be
// stored in the full size cache.
//
// At this point we have the image data, so scale it to the desired
// size, store the scaled version to the thumbnail cache and return
// it.

string RequestBase::thumbnail()
{
    int MAX_SIZE = 1920;  // TODO: Make limit configurable?
    QSize target_size = requested_size_.isValid() ? requested_size_ : QSize(MAX_SIZE, MAX_SIZE);

    // desired_size is (0,0) if the caller wants original size.
    string sized_key = key_;
    sized_key += '\0';
    sized_key += to_string(target_size.width());
    sized_key += '\0';
    sized_key += to_string(target_size.height());

    // Check if we have the thumbnail in the cache already.
    auto thumbnail = p_->thumbnail_cache_->get(sized_key);
    if (thumbnail)
    {
        return *thumbnail;
    }

    // Don't have the thumbnail yet, see if we have the original image around.
    auto full_size = p_->full_size_cache_->get(key_);
    if (!full_size)
    {
        // Try and download or read the artwork, provided that we don't
        // have this image in the failure cache.
        if (p_->failure_cache_->contains_key(key_))
        {
            return "";
        }
        auto image_data = fetch();
        switch (image_data.status)
        {
            case FetchStatus::Downloaded:  // Success, we'll return the thumbnail below.
                break;
            case FetchStatus::NeedsDownload:  // Caller will call download().
                return "";
            case FetchStatus::NotFound:
            {
                // Authoritative answer that artwork does not exist.
                // For local files, we don't set an expiry time because, if the file is
                // changed (say, such that artwork is added), the file's key will change too.
                // For remote files, we try again after one week.
                // TODO: Make interval configurable.
                // TODO: If the network is down, we don't want to add an
                //       entry to the failure cache, but try again ASAP.
                chrono::time_point<std::chrono::steady_clock> later;  // Infinite expiry time
                if (image_data.location == Location::remote)
                {
                    later = chrono::steady_clock::now() + chrono::hours(24 * 7);  // One week
                }
                p_->failure_cache_->put(key_, "", later);
                return "";
            }
            case FetchStatus::Error:
            {
                // Some non-authoritative failure, such as the server not responding,
                // or an out-of-process codec crashing.
                // For local files, we don't set an expiry time because, if the file is
                // changed, (say, its permissions change), the file's key will change too.
                // For remote files, we try again after two hours.
                // TODO: Make interval configurable.
                // TODO: If the network is down, we don't want to add an
                //       entry to the failure cache, but try again ASAP.
                chrono::time_point<std::chrono::steady_clock> later;  // Infinite expiry time
                if (image_data.location == Location::remote)
                {
                    later = chrono::steady_clock::now() + chrono::hours(2);  // Two hours before we try again.
                }
                p_->failure_cache_->put(key_, "", later);
                return "";
            }
            default:
                abort();  // LCOV_EXCL_LINE  // Impossible
        }

        // We keep the full-size version around for a while because it
        // is likely that the caller will ask for small thumbnail
        // first (for initial search results), followed by a larger
        // thumbnail (for a preview). If so, we don't download the
        // artwork a second time.
        if (image_data.cache_policy == CachePolicy::cache_fullsize)
        {
            Image full_size_image(image_data.data, target_size);
            auto w = full_size_image.width();
            auto h = full_size_image.height();
            if (max(w, h) > MAX_SIZE)
            {
                // Don't put ridiculously large images into the full-size cache.
                full_size_image = Image(image_data.data, QSize(MAX_SIZE, MAX_SIZE));
            }
            p_->full_size_cache_->put(key_, full_size_image.to_jpeg());
        }
        full_size = move(image_data.data);
    }

    Image scaled_image(*full_size, target_size);
    string jpeg = scaled_image.to_jpeg();
    p_->thumbnail_cache_->put(sized_key, jpeg);
    return jpeg;
}

static string local_key(string const& filename)
{
    auto path = boost::filesystem::canonical(filename);

    struct stat st;
    if (stat(path.native().c_str(), &st) == -1)
    {
        throw runtime_error("local_key(): cannot stat " + path.native() + ", errno = " + to_string(errno));
    }

    // The full cache key for the file is the concatenation of path name, device, inode, modification time,
    // and inode modification time (because permissions may change).
    // If the file exists with the same path on different removable media, or the file was modified since
    // we last cached it, the key will be different. There is no point in trying to remove such stale entries
    // from the cache. Instead, we just let the normal eviction mechanism take care of them (because stale
    // thumbnails due to file removal or file update are rare).
    string key = path.native();
    key += '\0';
    key += to_string(st.st_dev);
    key += '\0';
    key += to_string(st.st_ino);
    key += '\0';
    key += to_string(st.st_mtim.tv_sec) + "." + to_string(st.st_mtim.tv_nsec);
    key += '\0';
    key += to_string(st.st_ctim.tv_sec) + "." + to_string(st.st_ctim.tv_nsec);
    return key;
}

LocalThumbnailRequest::LocalThumbnailRequest(shared_ptr<ThumbnailerPrivate> const& p, string const& filename, QSize const& requested_size)
    : RequestBase(p, local_key(filename), requested_size), filename_(filename)
{
}

RequestBase::ImageData LocalThumbnailRequest::fetch() {
    if (screenshotter_)
    {
        // The image data has been extracted via vs-thumb
        if (screenshotter_->success()) {
            return ImageData{FetchStatus::Downloaded, screenshotter_->data(), CachePolicy::cache_fullsize, Location::local};
        } else {
            cerr << "Failed to get thumbnail: " << screenshotter_->error();
            return {FetchStatus::Error, "", CachePolicy::dont_cache_fullsize, Location::local};
        }
    }

    // Work out content type.
    unique_gobj<GFile> file(g_file_new_for_path(filename_.c_str()));
    if (!file)
    {
        return {FetchStatus::Error, "", CachePolicy::dont_cache_fullsize, Location::local};
    }

    unique_gobj<GFileInfo> info(g_file_query_info(file.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
                                                  G_FILE_QUERY_INFO_NONE,
                                                  /* cancellable */ NULL,
                                                  /* error */ NULL));      // TODO: need decent error reporting
    if (!info)
    {
        return {FetchStatus::Error, "", CachePolicy::dont_cache_fullsize, Location::local};
    }

    string content_type = g_file_info_get_attribute_string(info.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
    cerr << "content type: " << content_type << endl;
    if (content_type.empty())
    {
        return {FetchStatus::Error, "", CachePolicy::dont_cache_fullsize, Location::local};
    }

    // Call the appropriate image extractor and return the image data as JPEG (not scaled).
    // We indicate that full-size images are to be cached only for audio and video files,
    // for which extraction is expensive. For local images, we don't cache full size.

    if (content_type.find("audio/") == 0 || content_type.find("video/") == 0)
    {
        return ImageData{FetchStatus::NeedsDownload, "", CachePolicy::cache_fullsize, Location::local};
    }
    if (content_type.find("image/") == 0)
    {
        return ImageData{FetchStatus::Downloaded, read_file(filename_), CachePolicy::dont_cache_fullsize, Location::local};
    }
    return ImageData{FetchStatus::NotFound, "", CachePolicy::dont_cache_fullsize, Location::local};
}

void LocalThumbnailRequest::download(std::chrono::milliseconds timeout) {
    screenshotter_.reset(new VideoScreenshotter(filename_, timeout));
    connect(screenshotter_.get(), &VideoScreenshotter::finished,
            this, &LocalThumbnailRequest::downloadFinished, Qt::DirectConnection);
    screenshotter_->extract();
}

AlbumRequest::AlbumRequest(shared_ptr<ThumbnailerPrivate> const& p,
                           string const& artist,
                           string const& album,
                           QSize const& requested_size)
    : RequestBase(p, artist + '\0' + album, requested_size)
    , artist_(artist)
    , album_(album)
{
}

AlbumRequest::~AlbumRequest()
{
    timer_.stop();
}

RequestBase::ImageData AlbumRequest::fetch() {
    if (artreply_)
    {
        if (artreply_->succeeded())
        {
            auto raw_data = artreply_->data();
            return ImageData{FetchStatus::Downloaded,
                    string(raw_data.data(), raw_data.size()), CachePolicy::cache_fullsize, Location::remote};
        }
        else if (artreply_->not_found_error())
        {
            return ImageData{FetchStatus::NotFound, "", CachePolicy::cache_fullsize, Location::remote};
        }
        else
        {
            return ImageData{FetchStatus::Error, "", CachePolicy::cache_fullsize, Location::remote};
        }
    }
    else
    {
        return ImageData{FetchStatus::NeedsDownload, "", CachePolicy::cache_fullsize, Location::remote};
    }
}

void AlbumRequest::download(chrono::milliseconds timeout) {
    artreply_ = p_->downloader_->download_album(QString::fromStdString(artist_), QString::fromStdString(album_));
    connect(artreply_.get(), &ArtReply::finished,
            this, &AlbumRequest::downloadFinished, Qt::DirectConnection);
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &AlbumRequest::timeout);
    timer_.start(timeout.count());
}

void AlbumRequest::timeout()
{
    artreply_->abort();
}

ArtistRequest::ArtistRequest(shared_ptr<ThumbnailerPrivate> const& p,
                             string const& artist,
                             string const& album,
                             QSize const& requested_size)
    : RequestBase(p, artist + '\0' + album, requested_size)
    , artist_(artist)
    , album_(album)
{
}

ArtistRequest::~ArtistRequest()
{
    timer_.stop();
}

RequestBase::ImageData ArtistRequest::fetch() {
    if (artreply_)
    {
        if (artreply_->succeeded())
        {
            auto raw_data = artreply_->data();
            return ImageData{FetchStatus::Downloaded,
                    string(raw_data.data(), raw_data.size()), CachePolicy::cache_fullsize, Location::remote};
        }
        else if (artreply_->not_found_error())
        {
            return ImageData{FetchStatus::NotFound, "", CachePolicy::cache_fullsize, Location::remote};
        }
        else
        {
            return ImageData{FetchStatus::Error, "", CachePolicy::cache_fullsize, Location::remote};
        }
    }
    else
    {
        return ImageData{FetchStatus::NeedsDownload, "", CachePolicy::cache_fullsize, Location::remote};
    }
}

void ArtistRequest::download(chrono::milliseconds timeout) {
    artreply_ = p_->downloader_->download_artist(QString::fromStdString(artist_), QString::fromStdString(album_));
    connect(artreply_.get(), &ArtReply::finished,
            this, &ThumbnailRequest::downloadFinished, Qt::DirectConnection);
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &ArtistRequest::timeout);
    timer_.start(timeout.count());
}

void ArtistRequest::timeout()
{
    artreply_->abort();
}

Thumbnailer::Thumbnailer()
    : p_(make_shared<ThumbnailerPrivate>())
{
}

Thumbnailer::~Thumbnailer() = default;

unique_ptr<ThumbnailRequest> Thumbnailer::get_thumbnail(string const& filename, QSize const &requested_size)
{
    assert(!filename.empty());

    return unique_ptr<ThumbnailRequest>(
        new LocalThumbnailRequest(p_, filename, requested_size));
}

unique_ptr<ThumbnailRequest> Thumbnailer::get_album_art(string const& artist,
                                                        string const& album,
                                                        QSize const &requested_size)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());

    return unique_ptr<ThumbnailRequest>(
        new AlbumRequest(p_, artist, album, requested_size));
}

unique_ptr<ThumbnailRequest> Thumbnailer::get_artist_art(string const& artist,
                                                         string const& album,
                                                         QSize const &requested_size)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());

    return unique_ptr<ThumbnailRequest>(
        new ArtistRequest(p_, artist, album, requested_size));
}

#include "thumbnailer.moc"

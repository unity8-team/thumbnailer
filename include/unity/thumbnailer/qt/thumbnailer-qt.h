/*
 * Copyright (C) 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#pragma once

#include <QImage>
#include <QObject>
#include <QSharedPointer>

class QDBusConnection;

namespace unity
{

namespace thumbnailer
{

namespace qt
{

namespace internal
{
class ThumbnailerImpl;
class RequestImpl;
}

/**
\brief Holds a thumbnailer request.

This class stores the state of an in-progress or completed thumbnail request.
*/

class Q_DECL_EXPORT Request : public QObject
{
    Q_OBJECT
public:
    /// @cond
    Q_DISABLE_COPY(Request)
    /// @endcond

    /**
    \brief Destroys a request.

    If the request is still in progress, it is implicitly cancelled, and
    the finished() signal is _not_ emitted.

    \warning You _must_ destroy all request instances created by a Thumbnailer _before_
    destroying the Thumbnailer instance.
    */
    ~Request();

    /**
    \brief Returns whether the request has completed.

    \return `false` if the request is still in progress. Otherwise, the return
    value is `true` (whether the request completed successfully or not).
    */
    bool isFinished() const;

    /**
    \brief Returns the thumbnail.
    \return A valid `QImage` if the request was successful and an empty `QImage`, otherwise.
    */
    QImage image() const;

    /**
    \brief Returns the error message for a failed request.
    \return The error message in case of a failure and an empty `QString`, otherwise.
    */
    QString errorMessage() const;

    /**
    \brief Returns whether the request completed successfully.
    \return `true` if the request completed successfully. Otherwise, if the request is still
    in progress, has failed, or was cancelled, the return value is `false`.
    */
    bool isValid() const;

    /**
    \brief Blocks the calling thread until the request completes.

    It is safe to call waitForFinished() on the same request more than once.
    If called on an already-completed (or cancelled) request, waitForFinished() returns immediately.

    \warning Calling this function from the main (GUI) thread might cause your user interface to freeze.

    \warning Calling waitForFinished() causes the request to be scheduled out of order. This means
    that, if you send requests for thumbnails A, B, and C (in that order) and then call waitForFinished()
    on C, you _cannot_ assume that A and B have also finished once waitForFinished() returns.
    */
    void waitForFinished();

    /**
    \brief Cancel the thumbnail request.

    Cancels the request if it has not completed yet and emits the finished() signal.
    Calling cancel() more than once or on a request that has already completed does nothing.
    */
    void cancel();

    /**
    \brief Returns whether the request was cancelled.
    \return `true` if the request was cancelled and `false`, otherwise.
    \note Depending on the time at which cancel() is called,
          the request may complete successfully despite having been cancelled.
    */
    bool isCancelled() const;

Q_SIGNALS:
    /**
    \brief This signal is emitted when the request completes.
    */
    void finished();

private:
    QScopedPointer<internal::RequestImpl> p_;
    explicit Request(internal::RequestImpl* impl) Q_DECL_HIDDEN;

    friend class internal::ThumbnailerImpl;
};

/**
\brief Class to obtain thumbnail images for various media types.

Class Thumbnailer provides thumbnail images for local media (image, audio, and video) files, as well
as album covers and artist images for many musicians and bands.

Most common image formats, such as PNG, JPEG, BMP, and so on, are recognized. For streaming media, the
recognized formats depend on the installed GStreamer codecs.

For local media files, thumbnails are extracted directly from the file. (For audio files, this
requires the file to contain embedded artwork.) For album covers and artist images,
artwork is downloaded from a remote image server (<B>dash.ubuntu.com</B>) that maintains a large
database of albums and musicians.

The requested size for a thumbnail specifies a bounding box (in pixels) of type `QSize` to which
the thumbnail will be scaled.
(The aspect ratio of the original image is preserved.)

The returned thumbnails never exceed <B>max-thumbnail-size</B> (usually 1920, see <B>thumbnailer-settings</B>(7))
in their larger dimension, even if a larger size is requested.

- Passing `QSize(0,`<i>n</i>`)` or `QSize(`<i>n</i>`,0)` defines a bounding box of <i>n</i> pixels
in one dimension, and unconstrained size in the other dimension (subject to the <B>max-thumbnail-size</B>
limit).
- Passing `QSize(0,0)` requests a thumbnail that fits into a square bounding box of <B>max-thumbnail-size</B>.
- Sizes with one or both dimensions &ge;&nbsp;<B>max-thumbnail-size</B>) are clamped to
<B>max-thumbnail-size</B> pixels.
- Sizes with one or both dimensions &lt;&nbsp;0 return an error.

Original images are never scaled up, so the returned thumbnail may be smaller than its requested size.

All methods are asynchronous and are guaranteed not to block.

The return value is a shared pointer to a \link unity::thumbnailer::qt::Request Request\endlink instance that
provides access to the scaled thumbnail (or an error message).
*/

class Q_DECL_EXPORT Thumbnailer final
{
public:
    /// @cond
    Q_DISABLE_COPY(Thumbnailer)
    /// @endcond

    /**
    \brief Constructs a thumbnailer instance.

    A default-constructed Thumbnailer instance communicates with the
    thumbnailer service via the session bus.

    \warning Instantiation and finalization of Thumbnailer instances are expensive
    operations. Do not needlessly destroy a Thumbnailer only to re-create it again later.
    */
    Thumbnailer();

    /**
    \brief Constructs a thumbnailer instance, using the supplied DBus connection.

    Instead of connecting to the session bus, this constructor uses the
    supplied DBus connection to contact the thumbnailer service.

    \param connection The DBus connection via which to send requests.

    \warning Instantiation and finalization of Thumbnailer instances are expensive
    operations. Do not needlessly destroy a Thumbnailer only to re-create it again later.
    */
    explicit Thumbnailer(QDBusConnection const& connection);

    /**
    \brief Destroys a thumbnailer instance.

    \warning You _must_ keep the Thumbnailer instance alive for as long as
    Request instances created by that Thumbnailer instance exist.
    */
    ~Thumbnailer();

    /**
    \brief Retrieves a thumbnail for an album cover from the remote image server.
    \param artist The name of the artist.
    \param album The name of the album.
    \param requestedSize The bounding box for the thumbnail.
    \return A `QSharedPointer` to a unity::thumbnailer::qt::Request holding the request state.
    */
    QSharedPointer<Request> getAlbumArt(QString const& artist, QString const& album, QSize const& requestedSize);

    /**
    \brief Retrieves a thumbnail for an artist from the remote image server.
    \param artist The name of the artist.
    \param album The name of the album.
    \param requestedSize The bounding box for the thumbnail.
    \return A `QSharedPointer` to a unity::thumbnailer::qt::Request holding the request state.
    */
    QSharedPointer<Request> getArtistArt(QString const& artist, QString const& album, QSize const& requestedSize);

    /**
    \brief Extracts a thumbnail from a media file.
    \param filePath The path to the file to extract the thumbnail from.
    \param requestedSize The bounding box for the thumbnail.
    \return A `QSharedPointer` to a unity::thumbnailer::qt::Request holding the request state.
    */
    QSharedPointer<Request> getThumbnail(QString const& filePath, QSize const& requestedSize);

private:
    QScopedPointer<internal::ThumbnailerImpl> p_;
};

}  // namespace qt

}  // namespace thumbnailer

}  // namespace unity

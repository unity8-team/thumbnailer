/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#include <memory>
#include <string>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>

struct HandlerPrivate;

class Handler : public QObject {
    Q_OBJECT
public:
    Handler(QDBusConnection &bus, const QDBusMessage &message);
    ~Handler();

    Handler(const Handler&) = delete;
    Handler& operator=(Handler&) = delete;

    void begin();

protected:
    void sendThumbnail(const QDBusUnixFileDescriptor &unix_fd);
    void sendError(const QString &error);

    // Methods to be overridden by handlers

    // check() determines whether the requested thumbnail exists in
    // the cache.  It is called synchronously in the thread pool.
    //
    // If it is available, it should be returned as a file descriptor,
    // which will be returned to the user.
    //
    // If not, processing will continue.
    virtual QDBusUnixFileDescriptor check() = 0;

    // download() is expected to perform any asynchronous actions and
    // end the request by either calling the downloadFinished() slot
    // or sendError().
    virtual void download() = 0;

    // create() should create the thumbnail and store it in the cache,
    // and return it as a file descriptor.  It is called synchronously
    // in the thread pool.
    virtual QDBusUnixFileDescriptor create() = 0;

protected Q_SLOTS:
    void checkFinished();
    void downloadFinished();
    void createFinished();

Q_SIGNALS:
    void finished();

private:
    std::unique_ptr<HandlerPrivate> p;
};

/*
 * Copyright (C) 2015 Canonical Ltd.
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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#include "get_thumbnail.h"

#include "parse_size.h"
#include <internal/file_io.h>
#include <internal/raii.h>
#include <internal/safe_strerror.h>

#include <boost/filesystem.hpp>

#include <cassert>

#include <fcntl.h>
#include <unistd.h>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace unity
{

namespace thumbnailer
{

namespace tools
{

GetThumbnail::GetThumbnail(QCommandLineParser& parser)
    : Action(parser)
    , size_(0, 0)
{
    parser.addPositionalArgument("get", "Get thumbnail from local file", "get");
    parser.addPositionalArgument("source_file", "Path to image, audio, or video file");
    parser.addPositionalArgument("dir", "Output directory (default: current dir)", "[dir]");
    QCommandLineOption size_option(QStringList() << "s" << "size",
                                   "Thumbnail size, e.g. \"240x480\" or \"480\" (default: largest available size)",
                                   "size");
    parser.addOption(size_option);

    if (!parser.parse(QCoreApplication::arguments()))
    {
        throw parser.errorText() + "\n\n" + parser.helpText();
    }
    if (parser.isSet(help_option_))
    {
        throw parser.helpText();
    }

    auto args = parser.positionalArguments();
    assert(args.first() == QString("get"));

    if (args.size() < 2 || args.size() > 3)
    {
        throw parser.helpText();
    }
    input_path_ = args[1];
    if (args.size() == 3)
    {
        output_dir_ = args[2];
    }
    else
    {
        auto path = getcwd(nullptr, 0);
        output_dir_ = path;
        free(path);
    }

    if (parser.isSet(size_option))
    {
        size_ = parse_size(parser.value(size_option));
        if (!size_.isValid())
        {
            throw QString("GetThumbnail(): invalid size: " + parser.value(size_option));
        }
    }
}

GetThumbnail::~GetThumbnail() {}

void GetThumbnail::run(DBusConnection& conn)
{
    int fd = open(input_path_.toUtf8().data(), O_RDONLY);
    if (fd == -1)
    {
        throw QString("GetThumbnail::run(): cannot open ") + input_path_ + ": " +
                      QString::fromStdString(safe_strerror(errno));
    }
    QDBusUnixFileDescriptor ufd(fd);
    auto reply = conn.thumbnailer().GetThumbnail(input_path_, ufd, size_);
    reply.waitForFinished();
    if (!reply.isValid())
    {
        throw reply.error().message();
    }
    QDBusUnixFileDescriptor thumbnail_fd = reply.value();

    string out_path = output_dir_.toStdString();
    if (!out_path.empty())
    {
        out_path += "/";
    }
    boost::filesystem::path p = input_path_.toStdString();
    out_path += p.filename().stem().native();
    out_path += string("_") + to_string(size_.width()) + "x" + to_string(size_.height());
    out_path += ".jpg";

    FdPtr out_fd(open(out_path.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0600), do_close);
    if (out_fd.get() == -1)
    {
        throw string("GetThumbnail::run(): cannot open ") + out_path + ": " + safe_strerror(errno);
    }
    try
    {
        write_file(thumbnail_fd.fileDescriptor(), out_fd.get());
    }
    // LCOV_EXCL_START
    catch (std::exception const& e)
    {
        throw string("GetThumbnail::run(): cannot create thumbnail ") + out_path + ": " + e.what();
    }
    // LCOV_EXCL_STOP
}

}  // namespace tools

}  // namespace thumbnailer

}  // namespace unity

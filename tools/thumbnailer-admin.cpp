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
#include "dbus_connection.h"
#include "show_stats.h"

#include <boost/filesystem.hpp>

#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <stdio.h>

using namespace std;
using namespace unity::thumbnailer::tools;

namespace
{

string prog_name;

#if 0
void usage()
{
    fprintf(stderr, "usage: %s command [args...]\n", prog_name.c_str());
    fprintf(stderr, "    commands:\n");
    fprintf(stderr, "        - stats [hist] [i|t|f]\n");
    fprintf(stderr, "             Show stats. If hist is provided, add histogram.\n");
    fprintf(stderr, "             If i, t, or f is provided, restrict stats to the\n");
    fprintf(stderr, "             selected (image, thumbnailer, or failure) cache.\n");
    fprintf(stderr, "        - get [-s size] input_file [output_dir]\n");
    fprintf(stderr, "             Create a thumbnail for a local file.\n");
    fprintf(stderr, "             If size is specified as <width>x<size>, scale\n");
    fprintf(stderr, "             to the specified size. If size is specified as a\n");
    fprintf(stderr, "             single dimension, scale to a square bounding box of\n");
    fprintf(stderr, "             the specified size. If no size is available, return\n");
    fprintf(stderr, "             the largest size available. The thumbnail is written\n");
    fprintf(stderr, "             to output_dir (default is the current directory).\n");
}
#endif

template<typename A> Action::UPtr create_action(QStringList const& args)
{
    return Action::UPtr(new A(args));
}

typedef map<char const*, pair<Action::UPtr(*)(QStringList const& args), char const*>> ActionMap;

// Table that maps commands to their actions.
// Add new commands to this table, and implement each command in
// class derived from Action.

ActionMap const valid_actions =
{
    { "stats",  { &create_action<ShowStats>,    "Show statistics" } },
    { "get",    { &create_action<GetThumbnail>, "Get thumbnail from local file" } },
    { "artist", { &create_action<GetThumbnail>, "Get artist thumbnail" } },
    { "album",  { &create_action<GetThumbnail>, "Get album thumbnail" } }
};

QString command_summary()
{
    stringstream s;
    s << "Commands:" << endl;
    for (auto const& a : valid_actions)
    {
        s << "  " << setw(11) << left << a.first << " " << a.second.second << endl;
    }
    return QString::fromStdString(s.str());
}

// Check if we have a valid command. If so, instantiate the
// corresponding action and return it.

Action::UPtr parse_args(QStringList const& args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Thumbnailer admininstrative tool");
    parser.addPositionalArgument("command", "The command to execute.");

    if (!parser.parse(args))
    {
        auto err = parser.errorText() + "\n\n" + parser.helpText() + "\n" + command_summary();
        throw err;
    }

    if (args.size() < 2)
    {
        throw QString("too few arguments") + "\n\n" + parser.helpText() + "\n" + command_summary();
    }

    QString cmd = args[1];
    for (auto a : valid_actions)
    {
        if (QString(a.first) == cmd)
        {
            return a.second.first(move(args));
        }
    }
    fprintf(stderr, "%s: %s: invalid command\n", prog_name.c_str(), qPrintable(cmd));
    throw parser.helpText() + "\n" + command_summary();
}

void parse_and_execute(QCoreApplication const& app)
{
    auto action = parse_args(app.arguments());
    DBusConnection conn;
    action->run(conn);
}

}  // namespace

int main(int argc, char* argv[])
{
    int rc = EXIT_FAILURE;
    try
    {
        QCoreApplication app(argc, argv);
        boost::filesystem::path p = app.applicationName().toStdString();
        prog_name = p.filename().native();
        parse_and_execute(app);
        rc = EXIT_SUCCESS;
    }
    catch (std::exception const& e)
    {
        cerr << prog_name << ": " << e.what() << endl;
    }
    catch (QString const& msg)
    {
        cerr << prog_name << ": " << msg.toStdString() << endl;
    }
    catch (string const& msg)
    {
        cerr << prog_name << ": " << msg << endl;
    }
    catch (char const* msg)
    {
        cerr << prog_name << ": " << msg << endl;
    }
    // LCOV_EXCL_START
    catch (...)
    {
        cerr << prog_name << ": unknown exception" << endl;
    }
    // LCOV_EXCL_STOP
    return rc;
}

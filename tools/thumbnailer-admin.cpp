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

#include "dbus_connection.h"
#include "show_stats.h"

#include <functional>
#include <iostream>

#include <stdio.h>

using namespace std;
using namespace unity::thumbnailer::tools;

namespace
{

string prog_name;

void usage()
{
    fprintf(stderr, "usage: %s command [args...]\n", prog_name.c_str());
    fprintf(stderr, "    commands:\n");
    fprintf(stderr, "        - stats [hist] [i|t|f]\n");
    fprintf(stderr, "             Show stats. If hist is provided, add histogram.\n");
    fprintf(stderr, "             If i, t, or f is provided, restrict stats to the\n");
    fprintf(stderr, "             selected (image, thumbnailer, or failure) cache.\n");
}

template<typename A> Action::UPtr create_action(QStringList const& args)
{
    return Action::UPtr(new A(args));
}

typedef map<QString, Action::UPtr(*)(QStringList const& args)> ActionMap;

// Table that maps commands to their actions.
// Add new commands to this table, and implement each command in
// class derived from Action.

ActionMap valid_actions = { { "stats", &create_action<ShowStats> } };

// Check if we have a valid command. If so, instantiate the
// corresponding action and return it.

Action::UPtr parse_args(QStringList const& args)
{
    if (args.size() < 2)
    {
        usage();
        exit(EXIT_FAILURE);
    }

    QString cmd = args[1];
    auto it = valid_actions.find(cmd);
    if (it == valid_actions.end())
    {
        fprintf(stderr, "%s: %s: invalid command\n", prog_name.c_str(), qPrintable(cmd));
        usage();
        exit(EXIT_FAILURE);
    }

    return it->second(move(args));
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
        prog_name = app.applicationName().toStdString();
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

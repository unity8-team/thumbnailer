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

#include "utils/dbusserver.h"

#include <testsetup.h>

#include <gtest/gtest.h>

using namespace std;

class AdminTest : public ::testing::Test
{
protected:
    AdminTest()
    {
    }
    virtual ~AdminTest()
    {
    }

    virtual void SetUp() override
    {
        // start dbus service
        tempdir.reset(new QTemporaryDir(TESTBINDIR "/dbus-test.XXXXXX"));
        setenv("XDG_CACHE_HOME", (tempdir->path() + "/cache").toUtf8().data(), true);

        // set 3 seconds as max idle time
        setenv("THUMBNAILER_MAX_IDLE", "1000", true);

        dbus_.reset(new DBusServer());
    }

    string temp_dir()
    {
        return tempdir->path().toStdString();
    }

    virtual void TearDown() override
    {
        dbus_.reset();

        unsetenv("THUMBNAILER_MAX_IDLE");
        unsetenv("XDG_CACHE_HOME");
        tempdir.reset();
    }

    unique_ptr<QTemporaryDir> tempdir;
    unique_ptr<DBusServer> dbus_;
};

int run(QStringList const& args)
{
    QProcess process;
    process.setStandardInputFile(QProcess::nullDevice());
    process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    cerr << THUMBNAILER_ADMIN << endl;
    process.start(THUMBNAILER_ADMIN, args);
    process.waitForFinished();
    EXPECT_EQ(QProcess::NormalExit, process.exitStatus());
    return process.exitCode();
}

TEST(ServiceTest, service_not_running)
{
    EXPECT_EQ(1, run(QStringList{"stats"}));
}

TEST_F(AdminTest, no_args)
{
    EXPECT_EQ(0, run(QStringList{"stats"}));
}

TEST_F(AdminTest, image_stats)
{
    EXPECT_EQ(0, run(QStringList{"stats", "i"}));
}

TEST_F(AdminTest, thumbnail_stats)
{
    EXPECT_EQ(0, run(QStringList{"stats", "t"}));
}

TEST_F(AdminTest, failure_stats)
{
    EXPECT_EQ(0, run(QStringList{"stats", "f"}));
}

TEST_F(AdminTest, histogram)
{
    EXPECT_EQ(0, run(QStringList{"stats", "hist"}));
}

TEST_F(AdminTest, too_few_args)
{
    EXPECT_EQ(1, run(QStringList{}));
}

TEST_F(AdminTest, too_many_args)
{
    EXPECT_EQ(1, run(QStringList{"stats", "hist", "i", "t"}));
}

TEST_F(AdminTest, second_arg_wrong_with_two_args)
{
    EXPECT_EQ(1, run(QStringList{"stats", "foo"}));
}

TEST_F(AdminTest, second_arg_wrong_with_three_args)
{
    EXPECT_EQ(1, run(QStringList{"stats", "bar", "i"}));
}

TEST_F(AdminTest, third_arg_wrong_with_three_args)
{
    EXPECT_EQ(1, run(QStringList{"stats", "hist", "x"}));
}

TEST_F(AdminTest, bad_command)
{
    EXPECT_EQ(1, run(QStringList{"no_such_command"}));
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

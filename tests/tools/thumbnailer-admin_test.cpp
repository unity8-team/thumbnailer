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

#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>

using namespace std;
using namespace boost;

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

class AdminRunner
{
public:
    AdminRunner() {}
    int run(QStringList const& args)
    {
        process_.reset(new QProcess);
        process_->setStandardInputFile(QProcess::nullDevice());
        process_->setProcessChannelMode(QProcess::SeparateChannels);
        process_->start(THUMBNAILER_ADMIN, args);
        process_->waitForFinished();
        EXPECT_EQ(QProcess::NormalExit, process_->exitStatus());
        stdout_ = process_->readAllStandardOutput().toStdString();
        stderr_ = process_->readAllStandardError().toStdString();
        return process_->exitCode();
    }

    string stdout() const
    {
        return stdout_;
    }

    string stderr() const
    {
        return stderr_;
    }

private:
    unique_ptr<QProcess> process_;
    string stdout_;
    string stderr_;
};

TEST(ServiceTest, service_not_running)
{
    AdminRunner ar;
    EXPECT_EQ(1, ar.run(QStringList{"stats"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: No such interface")) << ar.stderr();
}

TEST_F(AdminTest, no_args)
{
    AdminRunner ar;
    EXPECT_EQ(0, ar.run(QStringList{"stats"}));
    auto output = ar.stdout();
    EXPECT_TRUE(output.find("Image cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Thumbnail cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Failure cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Histogram:") != string::npos) << output;
}

TEST_F(AdminTest, image_stats)
{
    AdminRunner ar;
    EXPECT_EQ(0, ar.run(QStringList{"stats", "i"}));
    auto output = ar.stdout();
    EXPECT_TRUE(output.find("Image cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Thumbnail cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Failure cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Histogram:") != string::npos) << output;
}

TEST_F(AdminTest, thumbnail_stats)
{
    AdminRunner ar;
    EXPECT_EQ(0, ar.run(QStringList{"stats", "t"}));
    auto output = ar.stdout();
    EXPECT_FALSE(output.find("Image cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Thumbnail cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Failure cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Histogram:") != string::npos) << output;
}

TEST_F(AdminTest, failure_stats)
{
    AdminRunner ar;
    EXPECT_EQ(0, ar.run(QStringList{"stats", "f"}));
    auto output = ar.stdout();
    EXPECT_FALSE(output.find("Image cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Thumbnail cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Failure cache:") != string::npos) << output;
    EXPECT_FALSE(output.find("Histogram:") != string::npos) << output;
}

TEST_F(AdminTest, histogram)
{
    AdminRunner ar;
    EXPECT_EQ(0, ar.run(QStringList{"stats", "hist"}));
    auto output = ar.stdout();
    EXPECT_TRUE(output.find("Image cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Thumbnail cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Failure cache:") != string::npos) << output;
    EXPECT_TRUE(output.find("Histogram:") != string::npos) << output;
}

TEST_F(AdminTest, stats_parsing)
{
    AdminRunner ar;

    // Too few args
    EXPECT_EQ(1, ar.run(QStringList{}));
    EXPECT_TRUE(starts_with(ar.stderr(), "usage: thumbnailer-admin")) << ar.stderr();

    // Too many args
    EXPECT_EQ(1, ar.run(QStringList{"stats", "hist", "i", "t"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: too many arguments")) << ar.stderr();

    // Second arg wrong with two args
    EXPECT_EQ(1, ar.run(QStringList{"stats", "foo"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: invalid argument for stats command: foo")) << ar.stderr();

    // Second arg wrong with three args
    EXPECT_EQ(1, ar.run(QStringList{"stats", "bar", "i"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: invalid argument for stats command: bar")) << ar.stderr();

    // Third arg wrong with three args
    EXPECT_EQ(1, ar.run(QStringList{"stats", "hist", "x"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: invalid argument for stats command: x")) << ar.stderr();

    // Bad command
    EXPECT_EQ(1, ar.run(QStringList{"no_such_command"}));
    EXPECT_TRUE(starts_with(ar.stderr(), "thumbnailer-admin: no_such_command: invalid command")) << ar.stderr();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    setenv("LC_ALL", "C", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

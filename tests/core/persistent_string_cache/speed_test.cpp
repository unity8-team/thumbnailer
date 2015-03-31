/*
 * Copyright (C) 2015 Canonical Ltd
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

#include <core/persistent_string_cache.h>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <random>

using namespace std;
using namespace core;

// Removes the contents of db_dir, but not db_dir itself.

void unlink_db(string const& db_dir)
{
    namespace fs = boost::filesystem;
    try
    {
        for (fs::directory_iterator end, it(db_dir); it != end; ++it)
        {
            remove_all(it->path());
        }
    }
    catch (...)
    {
    }
}

string const test_db = TEST_DIR "/perf";

char random_char()
{
    static auto seed = random_device()();
    static mt19937 engine(seed);
    static uniform_int_distribution<int> uniform_dist(0, 255);
    return uniform_dist(engine);
}

int random_int(int min, int max)
{
    static auto seed = random_device()();
    static mt19937 engine(seed);
    static uniform_int_distribution<int> uniform_dist(min, max);
    return uniform_dist(engine);
}

int random_size(double mean, double dev, int min, int max)
{
    static auto seed = random_device()();
    static mt19937 engine(seed);
    static normal_distribution<double> normal_dist(mean, dev);

    double val = round(normal_dist(engine));
    return val < min ? min : (val > max ? max : val);
}

string const& random_string(int size)
{
    assert(size >= 0);

    static string s;
    s.resize(size);
    for (auto i = 0; i < size; ++i)
    {
        s[i] = random_char();
    }
    return s;
}

TEST(PersistentStringCache, basic)
{
    unlink_db(test_db);

    int const max_cache_size = 100 * 1024 * 1024;
    double const headroom_percent = 0.05;
    int const record_size = 20 * 1024;
    double const dev = 7000.0;
    int const num_records = max_cache_size / record_size;
    double const hit_rate = 0.8;

    int const min_key = 0;
    int const max_key = ((1 - hit_rate) + 1) * num_records;

    int const iterations = 10000;
    double const MB = 1024.0 * 1024;

    cout.setf(ios::fixed, ios::floatfield);
    cout.precision(3);

    cout << "Cache size:    " << max_cache_size / MB << " MB" << endl;
    cout << "Records:       " << num_records << endl;
    cout << "Record size:   " << record_size / 1024.0 << " kB" << endl;
    cout << "Iterations:    " << iterations << endl;

    auto c = PersistentStringCache::open(test_db, max_cache_size, CacheDiscardPolicy::LRU_only);
    c->set_headroom(int64_t(max_cache_size * headroom_percent));

    static PersistentStringCache::Optional<string> val;

    auto start = chrono::system_clock::now();
    bool full = false;
    while (!full)
    {
        string key = to_string(random_int(min_key, max_key));

        val = c->get(key);
        if (!val)
        {
            string const& new_val = random_string(random_size(record_size, dev, 0, max_cache_size));
            c->put(key, new_val);
            if (c->size_in_bytes() >= max_cache_size * 0.99)
            {
                auto now = chrono::system_clock::now();
                double secs = chrono::duration_cast<chrono::milliseconds>(now - start).count() / 1000.0;
                cout << "Cache full, inserted " << c->size_in_bytes() / MB << " MB in " << secs << " seconds ("
                     << c->size_in_bytes() / MB / secs << " MB/sec)" << endl;
                full = true;
            }
        }
    }

    int64_t bytes_read = 0;
    int64_t bytes_written = 0;
    c->clear_stats();
    start = chrono::system_clock::now();
    for (auto i = 0; i < iterations; ++i)
    {
        string key = to_string(random_int(min_key, max_key));

        val = c->get(key);
        if (!val)
        {
            string const& new_val = random_string(random_size(record_size, dev, 0, max_cache_size));
            c->put(key, new_val);
            bytes_written += new_val.size();
        }
        else
        {
            bytes_read += val->size();
        }
    }

    auto now = chrono::system_clock::now();
    double secs = chrono::duration_cast<chrono::milliseconds>(now - start).count() / 1000.0;
    cout << endl;
    cout << "Performed " << iterations << " lookups with " << hit_rate * 100 << "% hit rate in " << secs << " seconds."
         << endl;
    cout << "Read:          " << bytes_read / MB << " MB (" << bytes_read / MB / secs << " MB/sec)" << endl;
    cout << "Wrote:         " << bytes_written / MB << " MB (" << bytes_written / MB / secs << " MB/sec)" << endl;
    auto sum = bytes_read + bytes_written;
    cout << "Total:         " << sum / MB << " MB (" << sum / MB / secs << " MB/sec)" << endl;
    cout << "Records/sec:   " << iterations / secs << endl;
    auto s = c->stats();
    cout << "Avg rec. size: " << bytes_written / double(s.misses()) / 1024 << " kB" << endl;
    cout << "Hits:          " << s.hits() << endl;
    cout << "Misses:        " << s.misses() << endl;
    cout << "Disk size:     " << c->disk_size_in_bytes() / MB << " MB" << endl;

    cout << endl
         << "Compacting cache... " << flush;
    start = chrono::system_clock::now();
    c.reset();
    now = chrono::system_clock::now();
    secs = chrono::duration_cast<chrono::milliseconds>(now - start).count() / 1000;
    cout << "done" << endl;
    cout << "Time:          " << secs << " sec" << endl;

    {
        auto c = PersistentStringCache::open(test_db);
        cout << "New size:      " << c->disk_size_in_bytes() / MB << " MB" << endl;
    }

    unlink_db(test_db);  // Reclaim disk space
}

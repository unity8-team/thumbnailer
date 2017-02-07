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
 *              Michi Henning <michi.henning@canonical.com>
 */

#include "inactivityhandler.h"

#include <internal/env_vars.h>

#include <QCoreApplication>
#include <QDebug>

#include <cassert>
#include <sstream>
#include <string>

namespace unity
{

namespace thumbnailer
{

namespace service
{

InactivityHandler::InactivityHandler(std::function<void()> const& timer_func)
    : timer_func_(timer_func)
    , num_active_requests_(0)
{
    assert(timer_func);
    connect(&timer_, &QTimer::timeout, this, &InactivityHandler::timer_expired);
    timer_.setInterval(internal::EnvVars::get_max_idle());
}

InactivityHandler::~InactivityHandler()
{
    assert(num_active_requests_ == 0);
    timer_.stop();
}

void InactivityHandler::request_started()
{
    assert(num_active_requests_ >= 0);

    if (num_active_requests_++ == 0)
    {
        timer_.stop();
    }
}

void InactivityHandler::request_completed()
{
    assert(num_active_requests_ > 0);

    if (--num_active_requests_ == 0)
    {
        timer_.start();
    }
}

void InactivityHandler::timer_expired()
{
    try
    {
        timer_func_();
    }
    // LCOV_EXCL_START
    catch (std::exception const& e)
    {
        qCritical() << "Inactivity handler::timer_expired: timer_func threw an exception:" << e.what();
    }
    catch (...)
    {
        qCritical() << "Inactivity handler::timer_expired: timer_func threw an unknown exception";
    }
    // LCOV_EXCL_STOP
}

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity

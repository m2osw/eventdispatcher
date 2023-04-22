// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

/** \file
 * \brief The declaration of the alert appender to generate alerts.
 *
 * This file declares the alert appender class which is used to count the
 * number of log messages in various ways and generate alerts whenever
 * one of the counters becomes too large.
 *
 * The counters increase whenever a certain message arrives and gets
 * decremented over a given period of time.
 */

// self
//
#include    <snaplogger/network/tcp_appender.h>

#include    <snaplogger/component.h>







namespace snaplogger_network
{


constexpr char const            COMPONENT_ALERT[]   = "alert";

extern snaplogger::component::pointer_t             g_alert_component;




class alert_appender
    : public tcp_appender
{
public:
    typedef std::shared_ptr<alert_appender>     pointer_t;

                                alert_appender(std::string const & name);
    virtual                     ~alert_appender() override;

    // appender implementation
    //
    virtual void                set_config(advgetopt::getopt const & params) override;

protected:
    // implement appender
    //
    virtual void                process_message(
                                          snaplogger::message const & msg
                                        , std::string const & formatted_message) override;

private:
    std::int64_t                f_limit = 10;
    std::int64_t                f_counter = 0;

    std::int64_t                f_alert_limit = 0;
    std::int64_t                f_alert_counter = 0;
};


} // snaplogger_network namespace
// vim: ts=4 sw=4 et

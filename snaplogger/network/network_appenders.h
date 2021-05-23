/*
 * Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
 *
 * https://snapwebsites.org/project/snaplogger
 * contact@m2osw.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once

/** \file
 * \brief Common definitions for the TCP and UDP appenders.
 *
 * This file declares various common definitions used by the various network
 * appenders.
 */

// snaplogger lib
//
#include    <snaplogger/appender.h>


// libaddr lib
//
#include    <libaddr/addr.h>



namespace snaplogger_network
{


enum class compression_t
{
    COMPRESSION_NONE,
    COMPRESSION_PER_MESSAGE,
    COMPRESSION_BLOCKS,
};


enum class acknowledge_t
{
    ACKNOWLEDGE_NONE,
    ACKNOWLEDGE_SEVERITY,
    ACKNOWLEDGE_ALL,
};


class base_network_appender
    : public appender
{
public:
    typedef std::shared_ptr<base_network_appender>
                        pointer_t;

                        base_network_appender(std::string const & name, std::string const & type);

    // appender implementation
    //
    virtual void        set_config(advgetopt::getopt const & params) override;

    void                set_server_address(addr::addr const & server_address)

    // new virtual
    //
    virtual void        server_address_changed();

private:
    addr::addr          f_server_address = addr::addr();
    acknowledge_t       f_acknowledge = acknowledge_t::ACKNOWLEDGE_ALL;
    severity_t          f_acknowledge_severity = severity_t::SEVERITY_ERROR;
    bool                f_fallback_to_console = false;
};


} // snaplogger_network namespace
// vim: ts=4 sw=4 et

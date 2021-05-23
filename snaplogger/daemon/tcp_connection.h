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
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/appender.h"


// snapdev lib
//
#include    <snapdev/raii_generic_deleter.h>



namespace snaplogger
{


class file_appender
    : public appender
{
public:
    typedef std::shared_ptr<file_appender>      pointer_t;

                        file_appender(std::string const name);
    virtual             ~file_appender() override;

    virtual void        set_config(advgetopt::getopt const & params) override;
    virtual void        reopen() override;

    void                set_filename(std::string const & filename);

protected:
    virtual void        process_message(message const & msg, std::string const & formatted_message) override;

private:
    std::string         f_path = std::string("/var/log/snaplogger");
    std::string         f_filename = std::string();
    snap::raii_fd_t     f_fd = snap::raii_fd_t();
    bool                f_initialized = false;
    bool                f_lock = true;
    bool                f_flush = true;
    bool                f_secure = false;
    bool                f_fallback_to_console = false;
};


} // snaplogger namespace
// vim: ts=4 sw=4 et

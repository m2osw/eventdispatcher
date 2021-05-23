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

/** \file
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/file_appender.h"

#include    "snaplogger/guard.h"
#include    "snaplogger/map_diagnostic.h"


// snapdev lib
//
#include    <snapdev/lockfile.h>


// C++ lib
//
#include    <iostream>


// C lib
//
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger
{


namespace
{


APPENDER_FACTORY(file);


}
// no name namespace



file_appender::file_appender(std::string const name)
    : appender(name, "file")
{
}


file_appender::~file_appender()
{
}


void file_appender::set_config(advgetopt::getopt const & opts)
{
    appender::set_config(opts);

    // PATH
    //
    std::string const path_field(get_name() + "::path");
    if(opts.is_defined(path_field))
    {
        f_path = opts.get_string(path_field);
    }
    else if(opts.is_defined("path"))
    {
        f_path = opts.get_string("path");
    }

    // FILENAME
    //
    std::string const filename_field(get_name() + "::filename");
    if(opts.is_defined(filename_field))
    {
        f_filename = opts.get_string(filename_field);
    }
    // else -- we'll try to dynamically determine a filename when we
    //         reach the send_message() function

    // LOCK
    //
    std::string const lock_field(get_name() + "::lock");
    if(opts.is_defined(lock_field))
    {
        f_lock = opts.get_string(lock_field) == "true";
    }

    // FLUSH
    //
    std::string const flush_field(get_name() + "::flush");
    if(opts.is_defined(flush_field))
    {
        f_flush = opts.get_string(flush_field) == "true";
    }

    // SECURE
    //
    std::string const secure_field(get_name() + "::secure");
    if(opts.is_defined(secure_field))
    {
        f_secure = opts.get_string(secure_field) != "false";
    }

    // FALLBACK TO CONSOLE
    //
    std::string const fallback_to_console_field(get_name() + "::fallback_to_console");
    if(opts.is_defined(fallback_to_console_field))
    {
        f_fallback_to_console = opts.get_string(fallback_to_console_field) == "true";
    }
}


void file_appender::reopen()
{
    guard g;

    f_fd.reset();
    f_initialized = false;
}


void file_appender::set_filename(std::string const & filename)
{
    if(f_filename != filename)
    {
        f_filename = filename;
        f_initialized = false;
    }
}


void file_appender::process_message(message const & msg, std::string const & formatted_message)
{
    snap::NOTUSED(msg);

    guard g;

    if(!f_initialized)
    {
        f_initialized = true;

        if(f_filename.empty())
        {
            // try to generate a filename
            //
            map_diagnostics_t map(get_map_diagnostics());
            auto const it(map.find("progname"));
            if(it == map.end())
            {
                return;
            }
            if(it->second.empty())
            {
                return;
            }

            f_filename = f_path + "/";
            if(f_secure)
            {
                f_filename += "secure/";
            }
            f_filename += it->second;
            f_filename += ".log";
        }
        else if(f_filename.find('/') == std::string::npos)
        {
            f_filename = f_path + "/" + f_filename;
        }
        std::string::size_type pos(f_filename.rfind('/'));
        if(pos == std::string::npos)
        {
            pos = 0;
        }
        if(f_filename.find('.', pos + 1) == std::string::npos)
        {
            f_filename += ".log";
        }

        if(access(f_filename.c_str(), R_OK | W_OK) != 0
        && errno != ENOENT)
        {
            return;
        }

        int flags(O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC | O_LARGEFILE | O_NOCTTY);
        int mode(S_IRUSR | S_IWUSR);
        if(!f_secure)
        {
            mode |= S_IRGRP;
        }

        f_fd.reset(open(f_filename.c_str(), flags, mode));
    }

    if(!f_fd)
    {
        return;
    }

    std::unique_ptr<snap::lockfd> lock_file;
    if(f_lock)
    {
        lock_file = std::make_unique<snap::lockfd>(f_fd.get(), snap::lockfd::mode_t::LOCKFILE_EXCLUSIVE);
    }

    ssize_t const l(write(f_fd.get(), formatted_message.c_str(), formatted_message.length()));
    if(static_cast<size_t>(l) != formatted_message.length())
    {
        // how could we report that? we are the logger...
        if(f_fallback_to_console
        && isatty(fileno(stdout)))
        {
            std::cout << formatted_message.c_str();
        }
    }
}





} // snaplogger namespace
// vim: ts=4 sw=4 et

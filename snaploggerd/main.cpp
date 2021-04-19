/*
 * Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Main snaplogger daemon start process.
 *
 * The main command of the daemon. This function creates the main service
 * object and calls its run() function.
 */

// self
//
//#include    "snaplogger/file_appender.h"

//#include    "snaplogger/guard.h"
//#include    "snaplogger/map_diagnostic.h"


// snapdev lib
//
//#include    <snapdev/lockfile.h>


// C++ lib
//
#include    <iostream>


// C lib
//
//#include    <sys/types.h>
//#include    <sys/stat.h>
//#include    <fcntl.h>
//#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>





namespace
{




}
// no name namespace




int main(int argc, char *argv[])
{
    ed::signal_handler::create_handler();

    try
    {
        service s(argc, argv);
        return s.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(0);
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred (3): " << e.what() << std::endl;
        SNAP_LOG_FATAL
            << "an exception occurred (3): "
            << e.what()
            << SNAP_LOG_SEND;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred (4)." << std::endl;
        SNAP_LOG_FATAL
            << "an unknown exception occurred (4)."
            << SNAP_LOG_SEND;
        exit(2);
    }
}






// vim: ts=4 sw=4 et

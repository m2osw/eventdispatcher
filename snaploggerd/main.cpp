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
    try
    {
        // panel_ve initialize the logger so we place it first
        //
        panel_ve p(argc, argv);

        // finalize the initialization of the Jetson panel
        //

        // the script can either redo its job each time or create a file
        // and do it only if the file did not exist yet
        //
        int r(system("/usr/bin/final-system-setup"));
        if(r != 0)
        {
            // ignore errors, just go ahead and start the panel either way
            // but still signal that an error happened
            //
            SNAP_LOG_WARNING
                << "the final-system-setup returned an error code: "
                << r
                << SNAP_LOG_SEND;
        }

        // this second command needs to run as root so we use a C++ command
        // which can do a setuid(0)
        //
        r = system("/usr/bin/final-jetson-setup");
        if(r != 0)
        {
            SNAP_LOG_WARNING
                << "the final-jetson-setup returned an error code: "
                << r
                << SNAP_LOG_SEND;
        }

        try
        {
            p.init();
            return p.run();
        }
        catch(std::exception const & e)
        {
            std::cerr << "error: an exception occurred (1): " << e.what() << std::endl;
            SNAP_LOG_FATAL
                << "an exception occurred (1): "
                << e.what()
                << SNAP_LOG_SEND;
            exit(1);
        }
        catch(...)
        {
            std::cerr << "error: an unknown exception occurred (2)." << std::endl;
            SNAP_LOG_FATAL
                << "an unknown exception occurred (2)."
                << SNAP_LOG_SEND;
            exit(2);
        }
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

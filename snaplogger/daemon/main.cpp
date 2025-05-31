// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Main snaplogger daemon start process.
 *
 * The main command of the daemon. This function creates the main service
 * object and calls its run() function.
 */

// self
//
#include    "snaplogger/daemon/snaploggerd.h"

#include    "snaplogger/daemon/network_component.h"


// advgetopt
//
#include    <advgetopt/exception.h>


// eventdispatcher
//
#include    <eventdispatcher/signal_handler.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>




int main(int argc, char *argv[])
{
    // handle Unix signals and log if one happens
    //
    ed::signal_handler::create_instance();

    try
    {
        snaplogger_daemon::snaploggerd daemon(argc, argv);
        if(!daemon.init())
        {
            return 1;
        }
        return daemon.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(0);
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred: " << e.what() << std::endl;
        SNAP_LOG_FATAL
            << snaplogger::section(snaplogger::g_normal_component)
            << snaplogger::section(snaplogger_daemon::g_network_component)
            << snaplogger::section(snaplogger_daemon::g_daemon_component)
            << "an exception occurred: "
            << e.what()
            << SNAP_LOG_SEND;
        exit(2);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred." << std::endl;
        SNAP_LOG_FATAL
            << snaplogger::section(snaplogger::g_normal_component)
            << snaplogger::section(snaplogger_daemon::g_network_component)
            << snaplogger::section(snaplogger_daemon::g_daemon_component)
            << "an unknown exception occurred."
            << SNAP_LOG_SEND;
        exit(3);
    }
}






// vim: ts=4 sw=4 et

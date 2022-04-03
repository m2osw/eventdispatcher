// Snap Websites Server -- server to handle inter-process communication
// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include    "server.h"


// eventdispatcher
//
#include    <eventdispatcher/signal_handler.h>


// advgetopt
//
#include    <advgetopt/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>




int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();

    try
    {
        sc::server::pointer_t server(std::make_shared<sc::server>(argc, argv));
        return server->run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        // expected exit from the advgetopt (i.e. --help)
        return e.code();
    }
    catch(std::exception const & e)
    {
        std::cerr << "exception caught: "
            << e.what()
            << '\n';
        SNAP_LOG_FATAL
            << "exception caught: "
            << e.what()
            << SNAP_LOG_SEND;
        return 1;
    }
    catch(...)
    {
        std::cerr << "unknown exception caught!\n";
        SNAP_LOG_FATAL
            << "unknown exception caught!"
            << SNAP_LOG_SEND;
        return 1;
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et

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

/** \file
 * \brief Implementation of load_timer object.
 *
 * We use a timer to know when to check the load average of the computer.
 * This is used to know whether a computer is too heavily loaded and
 * so whether it should or not be accessed.
 */

// self
//
#include    "load_timer.h"


//// snapwebsites lib
////
//#include <snapwebsites/chownnm.h>
//#include <snapwebsites/flags.h>
//#include <snapwebsites/glob_dir.h>
//#include <snapwebsites/loadavg.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/qcompatibility.h>
//#include <snapwebsites/snap_communicator.h>
//#include <snapwebsites/snapwebsites.h>
//
//
//// snapdev lib
////
//#include <snapdev/not_used.h>
//#include <snapdev/tokenize_string.h>
//
//
//// libaddr lib
////
//#include <libaddr/addr_exception.h>
//#include <libaddr/addr_parser.h>
//#include <libaddr/iface.h>
//
//
//// Qt lib
////
//#include <QFile>
//
//
//// C++ lib
////
//#include <atomic>
//#include <cmath>
//#include <fstream>
//#include <iomanip>
//#include <sstream>
//#include <thread>
//
//
//// C lib
////
//#include <grp.h>
//#include <pwd.h>
//#include <sys/resource.h>


// included last
//
#include <snapdev/poison.h>







namespace sc
{



/** \class load_timer
 * \brief Provide a tick to offer load balancing information.
 *
 * This class is an implementation of a timer to offer load balancing
 * information between various front and backend computers in the cluster.
 */


/** \brief The timer initialization.
 *
 * The timer ticks once per second to retrieve the current load of the
 * system and forward it to whichever computer that requested the
 * information.
 *
 * \param[in] cs  The snap communicator server we are listening for.
 */
load_timer::load_timer(server::pointer_t cs)
    : timer(1'000'000LL)  // 1 second in microseconds
    , f_server(cs)
{
    set_enable(false);
}


void load_timer::process_timeout()
{
    f_server->process_load_balancing();
}



} // sc namespace
// vim: ts=4 sw=4 et

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
 * \brief Implementation of an interrupt handler.
 *
 * This class is used to allow for a clean exit on an Ctrl-C.
 */

// self
//
#include    "interrupt.h"


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


// C lib
//
//#include <grp.h>
//#include <pwd.h>
#include    <signal.h>


// included last
//
#include <snapdev/poison.h>



namespace sc
{



/** \class interrupt
 * \brief Handle the SIGINT that is expected to stop the server.
 *
 * This class is an implementation of the snap_signal that listens
 * on the SIGINT.
 */



/** \brief The interrupt initialization.
 *
 * The interrupt uses the signalfd() function to obtain a way to listen on
 * incoming Unix signals.
 *
 * Specifically, it listens on the SIGINT signal, which is the equivalent
 * to the Ctrl-C.
 *
 * \param[in] cs  The snapcommunicator we are listening for.
 */
interrupt::interrupt(server::pointer_t cs)
    : signal(SIGINT)
    , f_server(cs)
{
    unblock_signal_on_destruction();
    set_name("snap communicator interrupt");
}


/** \brief Call the stop function of the snapcommunicator object.
 *
 * When this function is called, the signal was received and thus we are
 * asked to quit as soon as possible.
 */
void interrupt::process_signal()
{
    // we simulate the STOP, so pass 'false' (i.e. not quitting)
    //
    f_server->shutdown(false);
}



} // sc namespace
// vim: ts=4 sw=4 et

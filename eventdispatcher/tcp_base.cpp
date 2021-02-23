// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// self
//
#include    "eventdispatcher/tcp_base.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_private.h"


// cppthread lib
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>
#include    <cppthread/thread.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// OpenSSL lib
//
#include    <openssl/bio.h>
#include    <openssl/err.h>
#include    <openssl/ssl.h>


// last include
//
#include    <snapdev/poison.h>




#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Event Dispatcher requires OpenSSL to support multi-threading."
#endif

namespace ed
{



/** \brief Clean up the BIO environment.
 *
 * This function cleans up the BIO environment.
 *
 * \note
 * This function is here for documentation rather than to get called.
 * Whenever you exit a process that uses the BIO calls it will leak
 * a few things. To make the process really spanking clean, you want
 * to call this function before exit(3). You have to make sure that
 * you call this function only after every single BIO object was
 * closed and none must be opened after this call.
 */
void cleanup()
{
    detail::thread_cleanup();
    detail::bio_cleanup();
}


/** \brief Before a thread exits, this function must be called.
 *
 * Any error which is still attached to a thread must be removed
 * before the thread dies or it will be lost. This function must
 * be called before you return from your
 * cppthread::runner::run()
 * function.
 *
 * The thread must be pro-active and make sure to catch() errors
 * if necessary to ensure that this function gets called before
 * it exists.
 *
 * Also, this means all BIO connections were properly terminated
 * before the thread returns.
 *
 * \note
 * TBD--this may not be required. I read a few things a while back
 * saying that certain things were now automatic in the BIO library
 * and this may very well be one of them. To test this function,
 * see the snapwebsites/snapdbproxy/src/snapdbproxy_connection.cpp
 * in the snapwebsites project and see how it works one way or the other.
 */
void cleanup_on_thread_exit()
{
    detail::per_thread_cleanup();
}



} // namespace ed
// vim: ts=4 sw=4 et

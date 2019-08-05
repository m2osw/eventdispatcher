// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


// self
//
#include    "eventdispatcher/dispatcher_base.h"


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implementation of the dispatcher_base class.
 *
 * This class is used to allow you to supply a dispatcher to the library.
 */

namespace ed
{



/** \brief The dispatcher base destructor.
 *
 * This destructor is virtual allowing clean derivation at all levels.
 */
dispatcher_base::~dispatcher_base()
{
}



} // namespace ed
// vim: ts=4 sw=4 et

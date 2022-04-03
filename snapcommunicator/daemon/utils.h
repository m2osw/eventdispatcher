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
#pragma once

/** \file
 * \brief Various utilities.
 *
 * Useful types and functions for the Snap! Communicator.
 */

// libaddr
//
#include    <libaddr/addr.h>


// C++
//
#include    <set>
#include    <string>



namespace sc
{


typedef std::set<std::string>                 sorted_list_of_strings_t;
typedef std::set<addr::addr>                  sorted_list_of_addresses_t;


sorted_list_of_strings_t    canonicalize_services(std::string const & services);
std::string                 canonicalize_server_types(std::string const & server_types);
std::string                 canonicalize_neighbors(std::string const & neighbors);


} // sc namespace
// vim: ts=4 sw=4 et

// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Various useful functions and declarations.
 *
 * Some functions/declarations that are used throughout the library.
 */


// snapdev lib
//
#include    <snapdev/raii_generic_deleter.h>


// C++ lib
//
#include    <cstdint>
#include    <map>
#include    <string>
#include    <vector>


// C lib
//
#include    <netdb.h>



namespace ed
{



typedef std::vector<std::string>            string_list_t;
typedef std::map<std::string, std::string>  string_map_t;


/** \brief Address info pointer which auto-frees the structures.
 *
 * This smart pointer is used so the addrinfo structure is automatically
 * freed once done with the concerned variable.
 *
 * The pointer is exception safe as well.
 */
typedef std::unique_ptr<
                  addrinfo
                , snap::raii_pointer_deleter<
                                      addrinfo
                                    , decltype(&::freeaddrinfo)
                                    , &::freeaddrinfo>> raii_addrinfo_t;


constexpr int                       MAX_CONNECTIONS = 50;


std::int64_t                        get_current_date();
std::int64_t                        get_current_date_ns();




} // namespace ed
// vim: ts=4 sw=4 et

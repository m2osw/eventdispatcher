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
 * \brief Various utilities.
 *
 * Useful types and functions for the Snap! Communicator.
 */


//// self
////
//#include "version.h"
//
//
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


// C++ lib
//
#include <set>
#include <string>
//#include <fstream>
//#include <iomanip>
//#include <sstream>
//#include <thread>


//// C lib
////
//#include <grp.h>
//#include <pwd.h>
//#include <sys/resource.h>



namespace sc
{


typedef std::set<std::string>                 sorted_list_of_strings_t;


sorted_list_of_strings_t    canonicalize_services(std::string const & services);
std::string                 canonicalize_server_types(std::string const & server_types)
std::string                 canonicalize_neighbors(std::string const & neighbors)


} // sc namespace
// vim: ts=4 sw=4 et

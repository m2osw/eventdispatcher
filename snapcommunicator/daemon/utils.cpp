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
 * \brief Implementation of the utilities.
 *
 * This file includes the implementation of various useful standalone
 * functions.
 */


// self
//
#include    "utils.h"


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


// snapdev
//
#include <snapdev/join_strings.h>
#include <snapdev/tokenize_string.h>


// libaddr lib
//
//#include <libaddr/addr_exception.h>
#include <libaddr/addr_parser.h>
//#include <libaddr/iface.h>


// snaplogger
//
#include    <snaplogger/message.h>


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
#include    <snapdev/poison.h>






namespace sc
{


namespace
{


/** \brief List of valid types.
 *
 * A service is expected to be assigned a valid type. The following are
 * considered valid:
 *
 * \li proxy -- this is a frontend which is used to proxy traffic in
 * some way (i.e. to a specific server, as a load balancer, etc.)
 *
 * \li frontend -- this is the frontend which directly communicates with
 * a remote client (opposed to any service running on your Snap! C++
 * computers)
 *
 * \li backend -- this is a service running as a backend; it is not
 * accessible from a remote computer outside of the Snap! C++ cluster
 *
 * \li database -- this is a service specifically running a database;
 * in most cases, this is also a backend service
 */
sorted_list_of_strings_t const g_valid_types =
{
      "proxy"
    , "frontend"
    , "backend"
    , "database"
};


} // no name namespace



/** \brief Converts a string of server names to a set of names.
 *
 * This function breaks up the input string in service names at each comma.
 * Then it trims the names from all spaces. Empty entries are ignored.
 *
 * \todo
 * At this point the function doesn't verify that the name is valid.
 * The names should be checked with: `"[A-Za-z_][A-Za-z0-9_]*"`
 *
 * \param[in] services  The list of services.
 *
 * \return A set of string each representing a service.
 */
sorted_list_of_strings_t canonicalize_services(std::string const & services)
{
    sorted_list_of_strings_t list;
    snapdev::tokenize_string(list, services, ",", true, " ");
    return list;
}


/** \brief Make sure the list of types is valid and canonicalize it.
 *
 * This function splits the input \p server_types string at commas.
 * It trims all the strings (removes all spaces). It removes any empty
 * entries. The it joins the resulting list back in a string.
 *
 * \param[in] server_types  The raw list of server types.
 *
 * \return The canonicalized list of server types.
 */
std::string canonicalize_server_types(std::string const & server_types)
{
    sorted_list_of_strings_t raw_types;
    snapdev::tokenize_string(raw_types, server_types, ",", true, " ");

    sorted_list_of_strings_t types;
    std::set_intersection(
              raw_types.begin()
            , raw_types.end()
            , g_valid_types.begin()
            , g_valid_types.end()
            , std::inserter(types, types.begin()));

    if(types.size() != raw_types.size())
    {
        sorted_list_of_strings_t unwanted;
        std::set_difference(
                  raw_types.begin()
                , raw_types.end()
                , g_valid_types.begin()
                , g_valid_types.end()
                , std::inserter(unwanted, unwanted.begin()));

        SNAP_LOG_WARNING
            << "received "
            << unwanted.size()
            << " invalid server type(s): \""
            << snapdev::join_strings(unwanted, ", ")
            << "\", ignoring."
            << SNAP_LOG_SEND;
    }

    return snapdev::join_strings(types, ",");
}


/** \brief Canonicalize a list of neighbors.
 *
 * This function takes a strings, verify all the IP:port addresses and
 * then returns the same address back out.
 *
 * The function also rejects IP addresses with a range or a mask.
 *
 * The invalid addresses are simply removed from the list and we emit
 * an error. The function goes on doing its work otherwise.
 *
 * \note
 * The function has the side effect of \em sorting the addresses. The
 * sort is textual, not numeric.
 *
 * \note
 * In the new version of Snap! Communicator, we actually make use of
 * the fluid settings so the addresses should already be canonicalized.
 *
 * \note
 * The list is expected to be a list of comma separated addresses. This
 * function also accepts spaces and the canonicalization replaces those
 * spaces with commas.
 *
 * \param[in] neighbors  The comma separated list of IP:port addresses.
 *
 * \return The canonicalized list of IP:port addresses.
 */
std::string canonicalize_neighbors(std::string const & neighbors)
{
    addr::addr_parser p;
    p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
    p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
    p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
    p.set_default_port(4040);
    p.set_protocol("tcp");
    addr::addr_range::vector_t list(p.parse(neighbors));

    std::string result;
    result.reserve(neighbors.length());
    for(auto const & a : list)
    {
        if(a.has_to()
        || a.is_range()
        || !a.has_from())
        {
            // emit errors on invalid IP:port info and go on
            //
            SNAP_LOG_ERROR
                << "invalid neighbor address \""
                << "TBD" // 'a' is an addr now
                << "\", we could not convert it to a valid IP:port."
                << SNAP_LOG_SEND;
            continue;
        }

        if(!result.empty())
        {
            result += ',';
        }
        result += a.get_from().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ALL);
    }

    return result;
}


} // sc namespace
// vim: ts=4 sw=4 et

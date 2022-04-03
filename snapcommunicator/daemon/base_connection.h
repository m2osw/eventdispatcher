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
 * \brief Declaration of the base_connection class.
 *
 * All the client connections are derived from this base connection class
 * which allows us to manage many function in one place instead of having
 * them duplicated in three or more places.
 */



// self
//
#include    "server.h"


// eventdispatcher lib
//
#include    <eventdispatcher/connection.h>


// libaddr lib
//
//#include <libaddr/addr_exception.h>
//#include <libaddr/addr_parser.h>
//#include <libaddr/iface.h>


// C++ lib
//
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
//#include <sys/resource.h>



namespace sc
{




class base_connection
{
public:
    typedef std::shared_ptr<base_connection>    pointer_t;
    typedef std::vector<pointer_t>              vector_t;

    enum class connection_type_t
    {
        CONNECTION_TYPE_DOWN,   // not connected
        CONNECTION_TYPE_LOCAL,  // a service on this computer
        CONNECTION_TYPE_REMOTE  // another snapcommunicator on another computer
    };

                                base_connection(server::pointer_t cs);
    virtual                     ~base_connection();

    void                        connection_started();
    time_t                      get_connection_started() const;
    void                        connection_ended();
    time_t                      get_connection_ended() const;
    void                        set_server_name(std::string const & server_name);
    std::string                 get_server_name() const;
    void                        set_my_address(addr::addr const & my_address);
    addr::addr                  get_my_address() const;
    void                        set_connection_type(connection_type_t type);
    connection_type_t           get_connection_type() const;
    void                        set_services(std::string const & services);
    void                        get_services(sorted_list_of_strings_t & services);
    bool                        has_service(std::string const & name);
    void                        set_services_heard_of(std::string const & services);
    void                        get_services_heard_of(sorted_list_of_strings_t & services);
    void                        set_commands(std::string const & commands);
    bool                        understand_command(std::string const & command);
    bool                        has_commands() const;
    void                        remove_command(std::string const & command);
    void                        mark_as_remote();
    bool                        is_remote() const;
    void                        set_wants_loadavg(bool wants_loadavg);
    bool                        wants_loadavg() const;

protected:
    server::pointer_t           f_server = server::pointer_t();

private:
    sorted_list_of_strings_t    f_understood_commands = sorted_list_of_strings_t();
    time_t                      f_started_on = -1;
    time_t                      f_ended_on = -1;
    connection_type_t           f_type = connection_type_t::CONNECTION_TYPE_DOWN;
    std::string                 f_server_name = std::string();
    addr::addr                  f_my_address = addr::addr();
    sorted_list_of_strings_t    f_services = sorted_list_of_strings_t();
    sorted_list_of_strings_t    f_services_heard_of = sorted_list_of_strings_t();
    bool                        f_remote_connection = false;
    bool                        f_wants_loadavg = false;
};



} // namespace sc
// vim: ts=4 sw=4 et

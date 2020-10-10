// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    "eventdispatcher/utils.h"



namespace ed
{



typedef int                         message_version_t;

// this version defines the protocol version, it should really rarely
// change if ever
//
constexpr message_version_t         MESSAGE_VERSION = 1;

constexpr char const                MESSAGE_VERSION_NAME[]  = "version";



class message
{
public:
    typedef std::vector<message>    vector_t;
    typedef string_map_t            parameters_t;

    bool                    from_message(std::string const & msg);
    std::string             to_message() const;

    std::string const &     get_sent_from_server() const;
    void                    set_sent_from_server(std::string const & server);
    std::string const &     get_sent_from_service() const;
    void                    set_sent_from_service(std::string const & service);
    std::string const &     get_server() const;
    void                    set_server(std::string const & server);
    std::string const &     get_service() const;
    void                    set_service(std::string const & service);
    void                    reply_to(message const & msg);
    std::string const &     get_command() const;
    void                    set_command(std::string const & command);
    message_version_t       get_message_version() const;
    bool                    check_version_parameter() const;
    void                    add_version_parameter();
    void                    add_parameter(std::string const & name, std::string const & value);
    void                    add_parameter(std::string const & name, std::int32_t value);
    void                    add_parameter(std::string const & name, std::uint32_t value);
    void                    add_parameter(std::string const & name, long long value);
    void                    add_parameter(std::string const & name, unsigned long long value);
    void                    add_parameter(std::string const & name, std::int64_t value);
    void                    add_parameter(std::string const & name, std::uint64_t value);
    bool                    has_parameter(std::string const & name) const;
    std::string             get_parameter(std::string const & name) const;
    std::int64_t            get_integer_parameter(std::string const & name) const;
    parameters_t const &    get_all_parameters() const;

private:
    std::string             f_sent_from_server = std::string();
    std::string             f_sent_from_service = std::string();
    std::string             f_server = std::string();
    std::string             f_service = std::string();
    std::string             f_command = std::string();
    parameters_t            f_parameters = parameters_t();
    mutable std::string     f_cached_message = std::string();
};


void        verify_message_name(
                  std::string const & name
                , bool can_be_empty = false
                , bool can_be_lowercase = true);



} // namespace ed
// vim: ts=4 sw=4 et

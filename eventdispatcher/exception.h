// Event Dispatcher
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
#pragma once

/** \file
 * \brief Exceptions.
 *
 * The following are all the exceptions used by the Event Dispatcher
 * library.
 */


// libexcept
//
#include "libexcept/exception.h"



namespace ed
{



DECLARE_LOGIC_ERROR(event_dispatcher_parameter_error);
DECLARE_LOGIC_ERROR(event_dispatcher_implementation_error);

DECLARE_MAIN_EXCEPTION(event_dispatcher_exception);

DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_initialization_error);
DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_initialization_missing);
DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_invalid_message);
DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_invalid_parameter);
DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_runtime_error);
DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_unexpected_data);

//class tcp_client_server_logic_error : public std::logic_error
//{
//public:
//    tcp_client_server_logic_error(std::string const & errmsg) : logic_error(errmsg) {}
//};
//
//class tcp_client_server_runtime_error : public std::runtime_error
//{
//public:
//    tcp_client_server_runtime_error(std::string const & errmsg) : runtime_error(errmsg) {}
//};
//
//class tcp_client_server_parameter_error : public tcp_client_server_logic_error
//{
//public:
//    tcp_client_server_parameter_error(std::string const & errmsg) : tcp_client_server_logic_error(errmsg) {}
//};
//
//class tcp_client_server_initialization_error : public tcp_client_server_runtime_error
//{
//public:
//    tcp_client_server_initialization_error(std::string const & errmsg) : tcp_client_server_runtime_error(errmsg) {}
//};
//
//class tcp_client_server_initialization_missing_error : public tcp_client_server_runtime_error
//{
//public:
//    tcp_client_server_initialization_missing_error(std::string const & errmsg) : tcp_client_server_runtime_error(errmsg) {}
//};

//class udp_client_server_runtime_error : public std::runtime_error
//{
//public:
//    udp_client_server_runtime_error(const std::string & errmsg) : std::runtime_error(errmsg) {}
//};
//
//class udp_client_server_parameter_error : public std::logic_error
//{
//public:
//    udp_client_server_parameter_error(const std::string & errmsg) : std::logic_error(errmsg) {}
//};


} // namespace ed
// vim: ts=4 sw=4 et

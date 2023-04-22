// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/** \file
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/daemon/network_component.h"



// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_daemon
{


snaplogger::component::pointer_t        g_network_component(snaplogger::get_component(COMPONENT_NETWORK));
snaplogger::component::pointer_t        g_daemon_component(snaplogger::get_component(COMPONENT_DAEMON));
snaplogger::component::pointer_t        g_remote_component(snaplogger::get_component(COMPONENT_REMOTE, { g_daemon_component }));
snaplogger::component::pointer_t        g_local_component(snaplogger::get_component(COMPONENT_LOCAL, { g_daemon_component, g_remote_component }));
snaplogger::component::pointer_t        g_tcp_component(snaplogger::get_component(COMPONENT_TCP));
snaplogger::component::pointer_t        g_udp_component(snaplogger::get_component(COMPONENT_UDP, { g_tcp_component }));


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et

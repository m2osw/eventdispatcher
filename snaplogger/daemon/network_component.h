// Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief A component used with all the daemon logs.
 *
 * In order to distinguish the daemon and the remote logs, we use a component
 * called the "network component". This is just a standard snaplogger component
 * named "daemon".
 *
 * In a similar manner, we use a component to distinguish the remote logs
 * named the "remote component". This one is set to "remote".
 *
 * For other local services and tools that want to send their logs to the
 * snaplogger daemon, we also offer a "local" component.
 *
 * A message also includes environment parameters. For remote items, those
 * are the parameters found on the remote computer. (TODO)
 */

// self
//
//#include    "snaplogger/appender.h"


// snaplogger lib
//
#include    <snaplogger/component.h>



namespace snaplogger_daemon
{


constexpr char const                        COMPONENT_NETWORK[] = "network";
constexpr char const                        COMPONENT_DAEMON[]  = "daemon";
constexpr char const                        COMPONENT_REMOTE[]  = "remote";
constexpr char const                        COMPONENT_LOCAL[]   = "local";
constexpr char const                        COMPONENT_TCP[]     = "tcp";
constexpr char const                        COMPONENT_UDP[]     = "udp";

extern snaplogger::component::pointer_t     g_network_component;
extern snaplogger::component::pointer_t     g_daemon_component;
extern snaplogger::component::pointer_t     g_remote_component;
extern snaplogger::component::pointer_t     g_local_component;
extern snaplogger::component::pointer_t     g_tcp_component;
extern snaplogger::component::pointer_t     g_udp_component;



} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et

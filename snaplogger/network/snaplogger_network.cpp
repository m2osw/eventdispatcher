// Copyright (c) 2006-2025  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "snaplogger_network.h"


// serverplugins
//
#include    "serverplugins/factory.h"


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


SERVERPLUGINS_START(snaplogger_network)
    , ::serverplugins::description("snaplogger network plugin extension to allow sending log messages to other servers via TCP or UDP.")
    , ::serverplugins::help_uri("https://snapwebsites.org/project/eventdispatcher")
    , ::serverplugins::categorization_tag("snaplogger")
    , ::serverplugins::categorization_tag("network")
SERVERPLUGINS_END(snaplogger_network)



} // optional_namespace namespace
// vim: ts=4 sw=4 et

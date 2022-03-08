// Copyright (c) 2006-2022  Made to Order Software Corp.  All Rights Reserved
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


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


CPPTHREAD_PLUGIN_START(snaplogger_network, 5, 3)
    , ::cppthread::plugin_description("snaplogger network plugin extension to allow sending log messages to other servers via TCP or UDP.")
    , ::cppthread::plugin_help_uri("https://snapwebsites.org/project/eventdispatcher")
    , ::cppthread::plugin_categorization_tag("snaplogger")
    , ::cppthread::plugin_categorization_tag("network")
CPPTHREAD_PLUGIN_END(snaplogger_network)



} // optional_namespace namespace
// vim: ts=4 sw=4 et

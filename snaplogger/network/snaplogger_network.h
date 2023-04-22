// Copyright (c) 2022-2023  Made to Order Software Corp.  All Rights Reserved
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

// serverplugins
//
#include    <serverplugins/plugin.h>



namespace snaplogger_network
{



/** \brief The Snap! Logger Network plugin class.
 *
 * All plugins must have a plugin instance which gets created on load.
 * Without this, the plugin loader fails. At this time, there is no other
 * use for this class.
 */
class snaplogger_network
    : public serverplugins::plugin
{
public:
    SERVERPLUGINS_DEFAULTS(snaplogger_network);

private:
};



} // snaplogger_network namespace
// vim: ts=4 sw=4 et

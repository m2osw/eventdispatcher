// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Exceptions.
 *
 * The following are all the exceptions used by the Snap Communicator.
 */


// libexcept
//
#include    "libexcept/exception.h"



namespace ed
{



DECLARE_LOGIC_ERROR(snapcommunicator_logic_error);

DECLARE_MAIN_EXCEPTION(snapcommunicator_error);

DECLARE_EXCEPTION(snapcommunicator_error, unexpected_message);
DECLARE_EXCEPTION(snapcommunicator_error, unexpected_message_parameter);
DECLARE_EXCEPTION(snapcommunicator_error, unknown_exception);



} // namespace ed
// vim: ts=4 sw=4 et

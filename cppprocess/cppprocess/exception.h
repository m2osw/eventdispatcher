// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Exceptions for the process environment.
 *
 * This file includes the definitions of exceptions used by the C++ process
 * environment.
 */

// libexcept lib
//
#include    <libexcept/exception.h>


namespace cppprocess
{



DECLARE_LOGIC_ERROR(cppprocess_logic_error);

DECLARE_OUT_OF_RANGE(cppprocess_out_of_range);

DECLARE_MAIN_EXCEPTION(cppprocess_exception);

DECLARE_EXCEPTION(cppprocess_exception, cppprocess_incorrect_pipe_type);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_recursive_call);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_initialization_failed);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_in_use);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_invalid_parameters);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_not_started);
DECLARE_EXCEPTION(cppprocess_exception, cppprocess_directory_not_found);



} // namespace cppprocess
// vim: ts=4 sw=4 et

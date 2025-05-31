// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

/** \file
 * \brief Messages sent between services.
 *
 * Class used to manage messages.
 */


// advgetopt
//
#include    <advgetopt/advgetopt.h>


// C++
//
#include    <cstdint>
#include    <map>
#include    <memory>
#include    <string>
#include    <vector>



namespace ed
{



enum class parameter_type_t : std::uint8_t
{
    PARAMETER_TYPE_STRING,
    PARAMETER_TYPE_INTEGER,
    PARAMETER_TYPE_ADDRESS,
    PARAMETER_TYPE_TIMESPEC,

    PARAMETER_TYPE_DEFAULT = PARAMETER_TYPE_STRING,
};


typedef std::uint32_t               parameter_flag_t;

constexpr parameter_flag_t const    PARAMETER_FLAG_REQUIRED     = 0x0001;
constexpr parameter_flag_t const    PARAMETER_FLAG_EMPTY        = 0x0002;
constexpr parameter_flag_t const    PARAMETER_FLAG_FORBIDDEN    = 0x0004;

constexpr parameter_flag_t const    PARAMETER_FLAG_DEFAULT = PARAMETER_FLAG_REQUIRED | PARAMETER_FLAG_EMPTY;


struct message_parameter
{
    typedef std::vector<message_parameter>  vector_t;

    std::string             f_name = std::string();
    parameter_type_t        f_type = parameter_type_t::PARAMETER_TYPE_DEFAULT;
    parameter_flag_t        f_flags = PARAMETER_FLAG_DEFAULT;
};


struct message_definition
{
    typedef std::shared_ptr<message_definition> pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

    std::string                 f_command = std::string();
    message_parameter::vector_t f_parameters = message_parameter::vector_t();
};


void                            add_message_definition_options(advgetopt::getopt & opts);
void                            process_message_definition_options(advgetopt::getopt const & opts);
void                            set_message_definition_paths(std::string const & paths);
message_definition::pointer_t   get_message_definition(std::string const & command);


// useful for tests, see set_message_definition_paths() for details
class manage_message_definition_paths
{
public:
    manage_message_definition_paths(std::string const & paths)
    {
        set_message_definition_paths(paths);
    }

    virtual ~manage_message_definition_paths()
    {
        set_message_definition_paths(std::string());
    }
};



} // namespace ed
// vim: ts=4 sw=4 et

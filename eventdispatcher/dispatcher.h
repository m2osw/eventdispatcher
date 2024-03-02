// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Declaration of the dispatcher class.
 *
 * Class used to handle events.
 */


// self
//
#include    <eventdispatcher/dispatcher_match.h>
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/utils.h>



namespace ed
{




class dispatcher
{
public:
    typedef std::shared_ptr<dispatcher> pointer_t;

                        dispatcher(ed::connection_with_send_message * c);
                        dispatcher(dispatcher const &) = delete;
                        dispatcher & operator = (dispatcher const &) = delete;

    void                add_communicator_commands(bool auto_catch_all = true);
    dispatcher_match::vector_t const &
                        get_matches() const;
    void                add_match(dispatcher_match const & m);
    void                add_matches(dispatcher_match::vector_t const & matches);
    bool                dispatch(message & msg);
    void                set_trace(bool trace = true);
    void                set_show_matches(bool show_matches = true);
    bool                get_commands(string_list_t & commands);
    dispatcher_match    define_catch_all() const;

private:
    connection_with_send_message *  f_connection = nullptr;
    dispatcher_match::vector_t      f_matches = {};
    bool                            f_ended = false;
    bool                            f_trace = false;
    bool                            f_show_matches = false;
};



} // namespace ed
// vim: ts=4 sw=4 et

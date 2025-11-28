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
 * \brief Declaration of the communicator class.
 *
 * The communicator is the manager of the event dispatcher connections.
 * It handles the run() function with a poll() loop listening to all the
 * connections and calling your virtual connection functions.
 */


// self
//
#include    <eventdispatcher/connection.h>


// snaplogger
//
#include    <snaplogger/severity.h>


// snapdev
//
#include    <snapdev/timespec_ex.h>



namespace ed
{



// WARNING: a communicator object must be allocated and held in a shared pointer (see pointer_t)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class communicator
    : public std::enable_shared_from_this<communicator>
{
public:
    typedef std::shared_ptr<communicator>       pointer_t;

    static pointer_t                    instance();

    connection::vector_t const &        get_connections() const;
    bool                                add_connection(connection::pointer_t connection);
    bool                                remove_connection(connection::pointer_t connection);
    void                                set_force_sort(bool status = true);
    bool                                is_running() const;
    void                                debug_connections(snaplogger::severity_t severity = snaplogger::severity_t::SEVERITY_TRACE);
    void                                log_connections(snaplogger::severity_t severity = snaplogger::severity_t::SEVERITY_DEBUG);
    bool                                get_show_connections() const;
    void                                set_show_connections(bool status);
    snapdev::timespec_ex const &        get_idle() const;

    virtual bool                        run();

private:
                                        communicator();
                                        communicator(communicator const &) = delete;

    communicator &                      operator = (communicator const &) = delete;

    connection::vector_t                f_connections = connection::vector_t();
    bool                                f_force_sort = true;
    bool                                f_running = false;
    bool                                f_show_connections = false;
    snaplogger::severity_t              f_debug_connections = snaplogger::severity_t::SEVERITY_OFF;
    snapdev::timespec_ex                f_idle = snapdev::timespec_ex();
};
#pragma GCC diagnostic pop


} // namespace ed
// vim: ts=4 sw=4 et

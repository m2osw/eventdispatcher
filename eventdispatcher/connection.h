// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Declaration of the connection.
 *
 * The base class used to create connections. A connection must have a
 * file descriptor which can be poll()'ed on. This is how the poll()
 * function works. It's called a socket in the connection because the
 * communicator was first created to work with network connections.
 * Now it works with others too such as the signalfd and  file
 * listener.
 */

// snapdev
//
#include    <snapdev/timespec_ex.h>


// C++
//
#include    <memory>
#include    <string>
#include    <vector>



namespace ed
{



class communicator;


typedef int                             priority_t;

constexpr priority_t                    EVENT_MIN_PRIORITY = 0;
constexpr priority_t                    EVENT_DEFAULT_PRIORITY = 100;
constexpr priority_t                    EVENT_MAX_PRIORITY = 255;



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class connection
    : public std::enable_shared_from_this<connection>
{
public:
    typedef std::shared_ptr<connection> pointer_t;
    typedef std::weak_ptr<connection>   weak_pointer_t;
    typedef std::vector<pointer_t>      vector_t;
    typedef std::uint8_t                event_limit_t;

                                connection();
                                connection(connection const &) = delete;
    virtual                     ~connection();

    connection &                operator = (connection const &) = delete;

    void                        remove_from_communicator();

    std::string const &         get_name() const;
    void                        set_name(std::string const & name);

    virtual bool                is_listener() const;
    virtual bool                is_signal() const;
    virtual bool                is_reader() const;
    virtual bool                is_writer() const;
    virtual int                 get_socket() const = 0;
    virtual bool                valid_socket() const;

    bool                        is_enabled() const;
    virtual void                set_enable(bool enabled);

    priority_t                  get_priority() const;
    void                        set_priority(priority_t priority);
    static bool                 compare(pointer_t const & lhs, pointer_t const & rhs);

    event_limit_t               get_event_limit() const;
    void                        set_event_limit(event_limit_t event_limit);
    std::int32_t                get_processing_time_limit() const;
    void                        set_processing_time_limit(std::int32_t processing_time_limit);

    std::int64_t                get_timeout_delay() const;
    void                        set_timeout_delay(std::int64_t timeout_us);
    void                        set_timeout_delay(snapdev::timespec_ex const & date);
    void                        calculate_next_tick();
    std::int64_t                get_timeout_date() const;
    void                        set_timeout_date(std::int64_t date_us);
    void                        set_timeout_date(snapdev::timespec_ex const & date);
    std::int64_t                get_timeout_timestamp() const;

    void                        non_blocking();
    bool                        is_non_blocking() const;
    void                        keep_alive();
    bool                        is_keep_alive() const;

    bool                        is_done() const;
    void                        mark_done();
    void                        mark_not_done();

    // callbacks
    virtual void                process_timeout();
    virtual void                process_signal();
    virtual void                process_read();
    virtual void                process_write();
    virtual void                process_empty_buffer();
    virtual void                process_accept();
    virtual void                process_error();
    virtual void                process_hup();
    virtual void                process_invalid();
    virtual void                connection_added();
    virtual void                connection_removed();

protected:
    std::int64_t                save_timeout_timestamp();

private:
    enum class non_blocking_state_t : std::uint8_t
    {
        NON_BLOCKING_STATE_UNKNOWN,
        NON_BLOCKING_STATE_BLOCKING,
        NON_BLOCKING_STATE_NON_BLOCKING,
    };

    friend communicator;

    std::int64_t                get_saved_timeout_timestamp() const;

    std::string                 f_name = std::string();
    bool                        f_enabled = true;
    bool                        f_done = false;
    mutable non_blocking_state_t
                                f_non_blocking_state = non_blocking_state_t::NON_BLOCKING_STATE_UNKNOWN;
    event_limit_t               f_event_limit = 5;                  // limit before giving other events a chance
    priority_t                  f_priority = EVENT_DEFAULT_PRIORITY;
    std::int64_t                f_timeout_delay_start_date = 0;     // in microseconds
    std::int64_t                f_timeout_delay = -1;               // in microseconds
    std::int64_t                f_timeout_next_date = -1;           // in microseconds, when we use the f_timeout_delay
    std::int64_t                f_timeout_date = -1;                // in microseconds
    std::int64_t                f_saved_timeout_stamp = -1;         // in microseconds
    std::int32_t                f_processing_time_limit = 500'000;  // in microseconds
    int                         f_fds_position = -1;
};
#pragma GCC diagnostic pop



} // namespace ed
// vim: ts=4 sw=4 et

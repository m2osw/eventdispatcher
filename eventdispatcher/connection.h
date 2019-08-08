// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

// C++ lib
//
#include    <memory>
#include    <vector>



namespace ed
{



class communicator;


typedef int                             priority_t;

constexpr priority_t                    EVENT_MIN_PRIORITY = 0;
constexpr priority_t                    EVENT_DEFAULT__PRIORITY = 100;
constexpr priority_t                    EVENT_MAX_PRIORITY = 255;



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class connection
    : public std::enable_shared_from_this<connection>
{
public:
    typedef std::shared_ptr<connection> pointer_t;
    typedef std::vector<pointer_t>      vector_t;

                                connection();
                                connection(connection const & connection) = delete;
    virtual                     ~connection();

    connection &                operator = (connection const & connection) = delete;

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

    std::uint16_t               get_event_limit() const;
    void                        set_event_limit(uint16_t event_limit);
    std::uint16_t               get_processing_time_limit() const;
    void                        set_processing_time_limit(std::int32_t processing_time_limit);

    std::int64_t                get_timeout_delay() const;
    void                        set_timeout_delay(std::int64_t timeout_us);
    void                        calculate_next_tick();
    std::int64_t                get_timeout_date() const;
    void                        set_timeout_date(std::int64_t date_us);
    std::int64_t                get_timeout_timestamp() const;

    void                        non_blocking() const;
    void                        keep_alive() const;

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
    friend communicator;

    std::int64_t                get_saved_timeout_timestamp() const;

    std::string                 f_name = std::string();
    bool                        f_enabled = true;
    bool                        f_done = false;
    std::uint16_t               f_event_limit = 5;                  // limit before giving other events a chance
    priority_t                  f_priority = EVENT_DEFAULT__PRIORITY;
    std::int64_t                f_timeout_delay = -1;               // in microseconds
    std::int64_t                f_timeout_next_date = -1;           // in microseconds, when we use the f_timeout_delay
    std::int64_t                f_timeout_date = -1;                // in microseconds
    std::int64_t                f_saved_timeout_stamp = -1;         // in microseconds
    std::int32_t                f_processing_time_limit = 500000;   // in microseconds
    int                         f_fds_position = -1;
};
#pragma GCC diagnostic pop



} // namespace snap
// vim: ts=4 sw=4 et

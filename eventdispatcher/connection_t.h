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
 * \brief Declaration of the connection template.
 *
 * The base class used to create connections. A connection must have a
 * file descriptor which can be poll()'ed on. This is how the poll()
 * function works. It's called a socket in the connection because the
 * communicator was first created to work with network connections.
 * Now it works with others too such as the signalfd and  file
 * listener.
 */

// self
//
#include    <eventdispatcher/connection.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <memory>
#include    <string>
#include    <vector>



namespace ed
{



class communicator;


// with C++20 we can use `require` instead
//
//     if constexpr (requires { obj->toString(); })
//
#define CALL_EVENT(name)                                                    \
    template<bool> class call_##name##_event;                               \
    template<>                                                              \
    class call_##name##_event<true>                                         \
    {                                                                       \
    public:                                                                 \
        template<typename H, typename C>                                    \
        static void call(H & h, C & c)                                      \
        {                                                                   \
            h.name##_event(c);                                              \
        }                                                                   \
    };                                                                      \
    template<>                                                              \
    class call_##name##_event<false>                                        \
    {                                                                       \
    public:                                                                 \
        template<typename H, typename C>                                    \
        static void call(H & h, C & c)                                      \
        {                                                                   \
            snapdev::NOT_USED(h, c);                                        \
            SNAP_LOG_WARNING                                                \
                << "connection \""                                          \
                << c.get_name()                                             \
                << "\" received a process_timer() event without a corresponding event handler." \
                << SNAP_LOG_SEND;                                           \
        }                                                                   \
    };                                                                      \
    template<typename T, typename C>                                        \
    constexpr auto has_##name##_event(C & c) -> decltype(std::declval<T>().name##_event(c), bool()) \
    {                                                                       \
        return true;                                                        \
    }                                                                       \
    template<typename T, typename C>                                        \
    constexpr auto has_##name##_event(...)                                  \
    {                                                                       \
        return false;                                                       \
    }


CALL_EVENT(timeout)



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template<
    typename BaseConnectionT
  , typename EventHandlerT>
class connection_template
    : public BaseConnectionT
{
public:
    typedef std::shared_ptr<connection> pointer_t;
    typedef std::weak_ptr<connection>   weak_pointer_t;
    typedef std::vector<pointer_t>      vector_t;

    connection_template(EventHandlerT * event_handler)
        : BaseConnectionT(60)
        , f_event_handler(event_handler)
    {
    }

    virtual ~connection_template()
    {
    }

    connection_template(connection const &) = delete;
    connection_template & operator = (connection_template const &) = delete;

    // callbacks
    virtual void process_timeout() override
    {
        call_timeout_event<has_timeout_event<EventHandlerT, BaseConnectionT>(0)>::call(*f_event_handler, *this);
    }

    //virtual void                process_signal();
    //virtual void                process_read();
    //virtual void                process_write();
    //virtual void                process_empty_buffer();
    //virtual void                process_accept();
    //virtual void                process_error();
    //virtual void                process_hup();
    //virtual void                process_invalid();
    //virtual void                connection_added();
    //virtual void                connection_removed();

private:
    EventHandlerT *             f_event_handler = nullptr;
};
#pragma GCC diagnostic pop



} // namespace ed
// vim: ts=4 sw=4 et

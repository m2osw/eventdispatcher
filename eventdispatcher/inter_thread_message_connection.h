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
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/connection_with_send_message.h>


// cppthread
//
#include    <cppthread/fifo.h>


// snapdev
//
#include    <snapdev/raii_generic_deleter.h>



namespace ed
{



class inter_thread_message_connection
    : public connection
    , public connection_with_send_message
{
public:
    typedef std::shared_ptr<inter_thread_message_connection>    pointer_t;

                                inter_thread_message_connection();
    virtual                     ~inter_thread_message_connection() override;

    void                        close();

    // the child cannot have its own communicator object, so...
    int                         poll(int timeout);

    // connection implementation
    virtual bool                is_reader() const override;
    //virtual bool                is_writer() const override;
    virtual int                 get_socket() const override;
    virtual void                process_read() override;

    // connection_with_send_message
    virtual bool                send_message(message & msg, bool cache = false) override;

    // new callback
    virtual void                process_message_a(message & msg) = 0;
    virtual void                process_message_b(message & msg) = 0;

private:
    pid_t                       f_creator_id = -1;

    snapdev::raii_fd_t          f_thread_a = snapdev::raii_fd_t();
    cppthread::fifo<message>    f_message_a = cppthread::fifo<message>();

    snapdev::raii_fd_t          f_thread_b = snapdev::raii_fd_t();
    cppthread::fifo<message>    f_message_b = cppthread::fifo<message>();
};



} // namespace ed
// vim: ts=4 sw=4 et

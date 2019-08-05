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

// self
//
#include "eventdispatcher/connection.h"
#include "eventdispatcher/connection_with_send_message.h"


// cppthread lib
//
#include "cppthread/fifo.h"


// snapdev lib
//
#include "snapdev/raii_generic_deleter.h"



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

    // the child cannot have its own snap_communicator object, so...
    int                         poll(int timeout);

    // snap_connection implementation
    virtual bool                is_reader() const override;
    //virtual bool                is_writer() const override;
    virtual int                 get_socket() const override;
    virtual void                process_read() override;

    // connection_with_send_message
    virtual bool                send_message(message const & msg, bool cache = false) override;

    // new callback
    virtual void                process_message_a(message const & msg) = 0;
    virtual void                process_message_b(message const & msg) = 0;

private:
    pid_t                       f_creator_id = -1;

    snap::raii_fd_t             f_thread_a = snap::raii_fd_t();
    cppthread::fifo<message>    f_message_a = cppthread::fifo<message>();

    snap::raii_fd_t             f_thread_b = snap::raii_fd_t();
    cppthread::fifo<message>    f_message_b = cppthread::fifo<message>();
};



} // namespace ed
// vim: ts=4 sw=4 et

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
#include    "eventdispatcher/tcp_client_message_connection.h"
//#include "eventdispatcher/udp_client_server.h"
//#include "eventdispatcher/utils.h"
//
//
//// cppthread lib
////
//#include "cppthread/thread.h"
//
//
//// snapdev lib
////
//#include "snapdev/not_used.h"
//
//
//// C lib
////
//#include <signal.h>
//#include <sys/signalfd.h>



namespace ed
{



class tcp_blocking_client_message_connection
    : public tcp_client_message_connection
{
public:
                                tcp_blocking_client_message_connection(
                                          std::string const & addr
                                        , int port
                                        , mode_t mode = mode_t::MODE_PLAIN);

    void                        run();
    void                        peek();

    // connection_with_send_message
    virtual bool                send_message(message const & msg, bool cache = false) override;

    // connection callback
    virtual void                process_error() override;

private:
    std::string                 f_line = std::string();
};



} // namespace ed
// vim: ts=4 sw=4 et

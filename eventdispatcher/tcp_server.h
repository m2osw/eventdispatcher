// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/utils.h"


// libaddr
//
#include    <libaddr/addr.h>



namespace ed
{



// TODO: implement a bio_server then like with the client remove
//       this basic tcp_server if it was like the bio version
class tcp_server
{
public:
    typedef std::shared_ptr<tcp_server>     pointer_t;

                        tcp_server(
                                  addr::addr const & address
                                , int max_connections = MAX_CONNECTIONS
                                , bool reuse_addr = false
                                , bool auto_close = false);
                        tcp_server(tcp_server const &) = delete;
                        ~tcp_server();

    tcp_server &        operator = (tcp_server const &) = delete;

    int                 get_socket() const;
    int                 get_max_connections() const;
    addr::addr          get_address() const;
    bool                get_keepalive() const;
    void                set_keepalive(bool yes = true);
    bool                get_close_on_exec() const;
    void                set_close_on_exec(bool yes = true);

    int                 accept(int const max_wait_ms = -1);
    int                 get_last_accepted_socket() const;

private:
    int                 f_max_connections = MAX_CONNECTIONS;
    int                 f_socket = -1;
    int                 f_port = -1;
    addr::addr          f_address = addr::addr();
    int                 f_accepted_socket = -1;
    bool                f_keepalive = true;
    bool                f_auto_close = false;
    bool                f_close_on_exec = false;
};



} // namespace ed
// vim: ts=4 sw=4 et

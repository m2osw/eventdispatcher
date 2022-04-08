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
#include    "eventdispatcher/tcp_bio_options.h"
#include    "eventdispatcher/utils.h"


// libaddr
//
#include    <libaddr/addr.h>


// C++
//
#include    <memory>



namespace ed
{



namespace detail
{
class tcp_bio_client_impl;
}



class tcp_bio_server;



// Create/manage certificates details:
// https://help.ubuntu.com/lts/serverguide/certificates-and-security.html
class tcp_bio_client
{
public:
    typedef std::shared_ptr<tcp_bio_client>     pointer_t;

                        tcp_bio_client(
                                  addr::addr const & address
                                , mode_t mode = mode_t::MODE_PLAIN
                                , tcp_bio_options const & opt = tcp_bio_options());
                        tcp_bio_client(tcp_bio_client const & src) = delete;
    virtual             ~tcp_bio_client();

    tcp_bio_client &    operator = (tcp_bio_client const & rhs) = delete;

    void                close();

    int                 get_socket() const;
    addr::addr          get_address() const;
    addr::addr          get_client_address();

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    friend class tcp_bio_server;

                        tcp_bio_client();

    addr::addr          f_address = addr::addr();
    addr::addr          f_client_address = addr::addr();
    std::shared_ptr<detail::tcp_bio_client_impl>
                        f_impl = std::shared_ptr<detail::tcp_bio_client_impl>();
};



} // namespace ed
// vim: ts=4 sw=4 et

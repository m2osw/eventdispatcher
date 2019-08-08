// TCP Client & Server -- classes to ease handling sockets
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
#include    "eventdispatcher/tcp_bio_client.h"
#include    "eventdispatcher/utils.h"


// addr lib
//
#include "libaddr/addr.h"



namespace ed
{



namespace detail
{
class tcp_bio_server_impl;
}
// namespace detail






// try `man BIO_f_ssl` or go to:
// https://www.openssl.org/docs/manmaster/crypto/BIO_f_ssl.html
class tcp_bio_server
{
public:
    typedef std::shared_ptr<tcp_bio_server>     pointer_t;

    enum class mode_t
    {
        MODE_PLAIN,             // no encryption
        MODE_SECURE             // use TLS encryption
    };

                                tcp_bio_server(
                                      addr::addr const & addr_port
                                    , int max_connections
                                    , bool reuse_addr
                                    , std::string const & certificate
                                    , std::string const & private_key
                                    , mode_t mode);
    virtual                     ~tcp_bio_server();

    bool                        is_secure() const;
    int                         get_socket() const;
    tcp_bio_client::pointer_t   accept();

private:
    std::shared_ptr<detail::tcp_bio_server_impl>
                                f_impl = std::shared_ptr<detail::tcp_bio_server_impl>();
};



} // namespace tcp_client_server
// vim: ts=4 sw=4 et

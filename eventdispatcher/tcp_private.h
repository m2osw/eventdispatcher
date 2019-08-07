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
//#include    "eventdispatcher/utils.h"


//// addr lib
////
//#include "libaddr/addr.h"


// C++ lib
//
#include <memory>


//// C lib
////
//#include <arpa/inet.h>


// OpenSSL lib
//
// BIO versions of the TCP client/server
// TODO: move to an impl
//#include <openssl/bio.h>
//#include <openssl/err.h>
#include <openssl/ssl.h>



namespace ed
{


namespace detail
{


// shared with the tcp_biod_server
class tcp_bio_client_impl
{
public:
    std::shared_ptr<SSL_CTX>    f_ssl_ctx = std::shared_ptr<SSL_CTX>();
    std::shared_ptr<BIO>        f_bio = std::shared_ptr<BIO>();
};


void        bio_cleanup();
void        bio_deleter(BIO * bio);
void        bio_initialize();
int         bio_log_errors();
void        per_thread_cleanup();
void        ssl_ctx_deleter(SSL_CTX * ssl_ctx);
void        thread_cleanup();


}
// namespace detail


} // namespace ed
// vim: ts=4 sw=4 et

// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// C++
//
#include    <memory>


// OpenSSL
//
#include    <openssl/ssl.h>



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

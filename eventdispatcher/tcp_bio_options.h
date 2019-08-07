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

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the SSL flags worked on!)
#define OPENSSL_THREAD_DEFINES


// C++ lib
//
#include <string>


// OpenSSL lib
//
// TODO: create our own set of flags
#include <openssl/ssl.h>



namespace ed
{



class tcp_bio_options
{
public:
    typedef std::uint32_t       ssl_options_t;
    typedef size_t              verification_depth_t;

    static constexpr verification_depth_t   MAX_VERIFICATION_DEPTH = 100;
    static constexpr ssl_options_t          DEFAULT_SSL_OPTIONS = SSL_OP_NO_SSLv2
                                                                | SSL_OP_NO_SSLv3
                                                                | SSL_OP_NO_TLSv1
                                                                | SSL_OP_NO_COMPRESSION;

                                tcp_bio_options();

    void                        set_verification_depth(verification_depth_t depth);
    verification_depth_t        get_verification_depth() const;

    void                        set_ssl_options(ssl_options_t ssl_options);
    ssl_options_t               get_ssl_options() const;

    void                        set_ssl_certificate_path(std::string const path);
    std::string const &         get_ssl_certificate_path() const;
    void                        set_keepalive(bool keepalive = true);
    bool                        get_keepalive() const;

    void                        set_sni(bool sni = true);
    bool                        get_sni() const;

    void                        set_host(std::string const & host);
    std::string const &         get_host() const;

private:
    verification_depth_t        f_verification_depth = 4;
    ssl_options_t               f_ssl_options = DEFAULT_SSL_OPTIONS;
    std::string                 f_ssl_certificate_path = std::string("/etc/ssl/certs");
    bool                        f_keepalive = true;
    bool                        f_sni = true;
    std::string                 f_host = std::string();
};



} // namespace ed
// vim: ts=4 sw=4 et

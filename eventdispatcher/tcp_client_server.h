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
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// addr lib
//
#include "libaddr/addr.h"

// C++ lib
//
#include <stdexcept>
#include <memory>

// C lib
//
#include <arpa/inet.h>

// OpenSSL lib
//
// BIO versions of the TCP client/server
// TODO: move to an impl
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace tcp_client_server
{




// TODO: assuming that bio_client with MODE_PLAIN works the same way
//       as a basic tcp_client, we should remove this class
class tcp_client
{
public:
    typedef std::shared_ptr<tcp_client>     pointer_t;

                        tcp_client(std::string const & addr, int port);
                        tcp_client(tcp_client const & src) = delete;
    tcp_client &        operator = (tcp_client const & rhs) = delete;
                        ~tcp_client();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    int                 f_socket = -1;
    int                 f_port = -1;
    std::string         f_addr = std::string();
};


// TODO: implement a bio_server then like with the client remove
//       this basic tcp_server if it was like the bio version
class tcp_server
{
public:
    typedef std::shared_ptr<tcp_server>     pointer_t;

    static int const    MAX_CONNECTIONS = 50;

                        tcp_server(std::string const & addr, int port, int max_connections = MAX_CONNECTIONS, bool reuse_addr = false, bool auto_close = false);
                        tcp_server(tcp_server const & src) = delete;
    tcp_server &        operator = (tcp_server const & rhs) = delete;
                        ~tcp_server();

    int                 get_socket() const;
    int                 get_max_connections() const;
    int                 get_port() const;
    std::string         get_addr() const;
    bool                get_keepalive() const;
    void                set_keepalive(bool yes = true);

    int                 accept( int const max_wait_ms = -1 );
    int                 get_last_accepted_socket() const;

private:
    int                 f_max_connections = MAX_CONNECTIONS;
    int                 f_socket = -1;
    int                 f_port = -1;
    std::string         f_addr = std::string();
    int                 f_accepted_socket = -1;
    bool                f_keepalive = true;
    bool                f_auto_close = false;
};


class bio_server;

// Create/manage certificates details:
// https://help.ubuntu.com/lts/serverguide/certificates-and-security.html
class bio_client
{
public:
    typedef std::shared_ptr<bio_client>     pointer_t;

    enum class mode_t
    {
        MODE_PLAIN,             // avoid SSL/TLS
        MODE_SECURE,            // WARNING: may return a non-verified connection
        MODE_ALWAYS_SECURE      // fails if cannot be 100% secure
    };

    class options
    {
    public:
        typedef uint32_t            ssl_options_t;
        static int const            MAX_VERIFICATION_DEPTH = 100;
        static ssl_options_t const  DEFAULT_SSL_OPTIONS = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_COMPRESSION;

                            options();

        void                set_verification_depth(size_t depth);
        size_t              get_verification_depth() const;

        void                set_ssl_options(ssl_options_t ssl_options);
        ssl_options_t       get_ssl_options() const;

        void                set_ssl_certificate_path(std::string const path);
        std::string const & get_ssl_certificate_path() const;
        void                set_keepalive(bool keepalive = true);
        bool                get_keepalive() const;

        void                set_sni(bool sni = true);
        bool                get_sni() const;

        void                set_host(std::string const & host);
        std::string const & get_host() const;

    private:
        size_t              f_verification_depth = 4;
        uint32_t            f_ssl_options = DEFAULT_SSL_OPTIONS;
        std::string         f_ssl_certificate_path = std::string("/etc/ssl/certs");
        bool                f_keepalive = true;
        bool                f_sni = true;
        std::string         f_host = std::string();
    };

                        bio_client(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN, options const & opt = options());
                        bio_client(bio_client const & src) = delete;
    bio_client &        operator = (bio_client const & rhs) = delete;
                        ~bio_client();

    void                close();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    friend bio_server;

                        bio_client(std::shared_ptr<BIO> bio);

    std::shared_ptr<SSL_CTX>    f_ssl_ctx = std::shared_ptr<SSL_CTX>();
    std::shared_ptr<BIO>        f_bio = std::shared_ptr<BIO>();
};


// try `man BIO_f_ssl` or go to:
// https://www.openssl.org/docs/manmaster/crypto/BIO_f_ssl.html
class bio_server
{
public:
    typedef std::shared_ptr<bio_server>     pointer_t;

    static int const    MAX_CONNECTIONS = 50;

    enum class mode_t
    {
        MODE_PLAIN,             // no encryption
        MODE_SECURE             // use TLS encryption
    };

                            bio_server(addr::addr const & addr_port, int max_connections, bool reuse_addr, std::string const & certificate, std::string const & private_key, mode_t mode);

    bool                    is_secure() const;
    int                     get_socket() const;
    bio_client::pointer_t   accept();

private:
    int                         f_max_connections = MAX_CONNECTIONS;
    std::shared_ptr<SSL_CTX>    f_ssl_ctx = std::shared_ptr<SSL_CTX>();
    std::shared_ptr<BIO>        f_listen = std::shared_ptr<BIO>();
    bool                        f_keepalive = true;
};


void cleanup();
void cleanup_on_thread_exit();



bool is_ipv4(char const * ip);
bool is_ipv6(char const * ip);
void get_addr_port(QString const & addr_port, QString & addr, int & port, char const * protocol);


} // namespace tcp_client_server
// vim: ts=4 sw=4 et

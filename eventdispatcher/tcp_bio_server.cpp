// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Low level TCP server implementation.
 *
 * This class is the low level TCP server implementation which supports
 * encrypted connections using TLS.
 */

// make sure we use OpenSSL with multi-thread support
//
#define OPENSSL_THREAD_DEFINES


// self
//
#include    "eventdispatcher/tcp_bio_server.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_private.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <algorithm>


// OpenSSL
//
#include    <openssl/ssl.h>


// C
//
#include    <fcntl.h>


// last include
//
#include    <snapdev/poison.h>




#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Snap! requires support for multiple threads in OpenSSL."
#endif

namespace ed
{


namespace detail
{


class tcp_bio_server_impl
{
public:
    int                         f_max_connections = MAX_CONNECTIONS;
    std::shared_ptr<SSL_CTX>    f_ssl_ctx = std::shared_ptr<SSL_CTX>();
    std::shared_ptr<BIO>        f_listen = std::shared_ptr<BIO>();
    bool                        f_keepalive = true;
    bool                        f_close_on_exec = false;
};


}


/** \class tcp_bio_server
 * \brief Create a BIO server, bind it, and listen for connections.
 *
 * This class is a server socket implementation used to listen for
 * connections that are to use TLS encryptions.
 *
 * The bind address must be available for the server initialization
 * to succeed.
 *
 * The BIO extension is from the OpenSSL library and it allows the server
 * to allow connections using SSL (TLS really now a day). The server
 * expects to be given information about a certificate and a private
 * key to function. You may also use the server in a non-secure manner
 * (without the TLS layer) so you do not need to implement two instances
 * of your server, one with tcp_bio_server and one with tcp_server.
 */




/** \brief Construct a tcp_bio_server object.
 *
 * The tcp_bio_server constructor initializes a BIO server and listens
 * for connections from the specified address and port.
 *
 * The \p certificate and \p private_key filenames are expected to point
 * to a PEM file (.pem extension) that include the encryption information.
 *
 * The certificate file may include a chain in which case the whole chain
 * will be taken in account.
 *
 * \param[in] address  The address and port defined in an addr object.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] certificate  The server certificate filename (PEM).
 * \param[in] private_key  The server private key filename (PEM).
 * \param[in] mode  The mode used to create the listening socket.
 */
tcp_bio_server::tcp_bio_server(
          addr::addr const & address
        , int max_connections
        , bool reuse_addr
        , std::string const & certificate
        , std::string const & private_key
        , mode_t mode)
    : f_impl(std::make_shared<detail::tcp_bio_server_impl>())
{
    f_impl->f_max_connections = std::clamp(max_connections <= 0 ? MAX_CONNECTIONS : max_connections, 5, 1000);

    detail::bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_ALWAYS_SECURE:
    case mode_t::MODE_SECURE:
        {
            // the following code is based on the example shown in the man page
            //
            //        man BIO_f_ssl
            //
            if(certificate.empty()
            || private_key.empty())
            {
                throw initialization_error("with MODE_SECURE you must specify a certificate and a private_key filename");
            }

            std::shared_ptr<SSL_CTX> ssl_ctx; // use reset(), see SNAP-507
            ssl_ctx.reset(SSL_CTX_new(SSLv23_server_method()), detail::ssl_ctx_deleter);
            if(!ssl_ctx)
            {
                detail::bio_log_errors();
                throw initialization_error("failed creating an SSL_CTX server object");
            }

            SSL_CTX_set_cipher_list(ssl_ctx.get(), "ALL");//"HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");

            // Assign the certificate to the SSL context
            //
            // TBD: we may want to use SSL_CTX_use_certificate_file() instead
            //      (i.e. not the "chained" version)
            //
            if(!SSL_CTX_use_certificate_chain_file(ssl_ctx.get(), certificate.c_str()))
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing an SSL_CTX server object certificate");
            }

            // Assign the private key to the SSL context
            //
            if(!SSL_CTX_use_PrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
            {
                // on failure, try again again with the RSA version, just in case
                // (probably useless?)
                //
#if OPENSSL_VERSION_NUMBER < 0x30000020L
                if(!SSL_CTX_use_RSAPrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
                {
#endif
                    detail::bio_log_errors();
                    throw initialization_error("failed initializing an SSL_CTX server object private key");
#if OPENSSL_VERSION_NUMBER < 0x30000020L
                }
#endif
            }

            // Verify that the private key and certificate are a match
            //
            if(!SSL_CTX_check_private_key(ssl_ctx.get()))
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing an SSL_CTX server object private key");
            }

            // create a BIO connection with SSL
            //
            std::unique_ptr<BIO, void (*)(BIO *)> bio(BIO_new_ssl(ssl_ctx.get(), 0), detail::bio_deleter);
            if(bio == nullptr)
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing a BIO server object");
            }

            // get the SSL pointer, which generally means that the BIO
            // allocate succeeded fully, so we can set auto-retry
            //
            SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
            if(ssl == nullptr)
            {
                // TBD: does this mean we would have a plain connection?
                detail::bio_log_errors();
                throw initialization_error("failed connecting BIO object with SSL_CTX object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            //
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // create a listening connection
            //
            std::shared_ptr<BIO> socket;  // use reset(), see SNAP-507
            socket.reset(BIO_new_accept(address.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT).c_str()), detail::bio_deleter);
            if(socket == nullptr)
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(socket.get(), reuse_addr ? BIO_BIND_REUSEADDR : BIO_BIND_NORMAL);

            // Attach the SSL bio to the listening BIO, this means whenever
            // a new connection is accepted, it automatically attaches it to
            // an SSL connection
            //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_accept_bios(socket.get(), bio.get());
#pragma GCC diagnostic pop

            // WARNING: the listen object takes ownership of the `bio`
            //          pointer and thus we have to make sure that we
            //          do not keep it in our unique_ptr<>().
            //
            snapdev::NOT_USED(bio.release());

            // Actually call bind() and listen() on the socket
            //
            // I called BIO_do_accept() before, but this looks cleaner
            // (although both calls do the same thing)
            //
            int const r(BIO_do_connect(socket.get()));
            if(r <= 0)
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing the secure BIO server socket to listen for client connections");
            }

            int c(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_fd(socket.get(), &c);
#pragma GCC diagnostic pop
            if(c < 0)
            {
                std::stringstream ss;
                ss << "secure: bind() failed to connect to "
                   << address;
                throw initialization_error(ss.str());
            }
            int const l(listen(c, f_impl->f_max_connections));
            if(l != 0)
            {
                SNAP_LOG_CONFIGURATION
                    << "failed setting the socket backlog to "
                    << f_impl->f_max_connections
                    << '.'
                    << SNAP_LOG_SEND;
            }
            int error_code(ENOTCONN);
            socklen_t error_code_size = sizeof(error_code);
            int const g(getsockopt(c, SOL_SOCKET, SO_ERROR, reinterpret_cast<void *>(&error_code), &error_code_size));
            if(g != 0)
            {
                error_code = errno;
            }
            if(error_code != 0)
            {
                std::stringstream ss;
                ss << "secure: bind() failed to connect to "
                   << address
                   << " and reported error #"
                   << error_code
                   << ", "
                   << strerror(error_code);
                throw initialization_error(ss.str());
            }

            // it worked, save the results
            f_impl->f_ssl_ctx.swap(ssl_ctx);
            f_impl->f_listen.swap(socket);

            // secure connection ready
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            std::shared_ptr<BIO> socket; // use reset(), see SNAP-507
            socket.reset(BIO_new_accept(address.to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT).c_str()), detail::bio_deleter);
            if(socket == nullptr)
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(socket.get(), BIO_BIND_REUSEADDR);

            // Call bind() and listen() on the socket
            //
            // I called BIO_do_accept() before, but this looks cleaner
            // (although both calls do the same thing)
            //
            int const r(BIO_do_connect(socket.get()));
            if(r <= 0)
            {
                detail::bio_log_errors();
                throw initialization_error("failed initializing the plain BIO server socket to listen for client connections");
            }

            int c(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_fd(socket.get(), &c);
#pragma GCC diagnostic pop
            if(c < 0)
            {
                std::stringstream ss;
                ss << "plain: bind() failed to connect to "
                   << address;
                throw initialization_error(ss.str());
            }
            int const l(listen(c, f_impl->f_max_connections));
            if(l != 0)
            {
                SNAP_LOG_CONFIGURATION
                    << "failed setting the socket backlog to "
                    << f_impl->f_max_connections
                    << '.'
                    << SNAP_LOG_SEND;
            }
            int error_code(ENOTCONN);
            socklen_t error_code_size = sizeof(error_code);
            int const g(getsockopt(c, SOL_SOCKET, SO_ERROR, reinterpret_cast<void *>(&error_code), &error_code_size));
            if(g != 0)
            {
                error_code = errno;
            }
            if(error_code != 0)
            {
                std::stringstream ss;
                ss << "plain: bind() failed to connect to "
                   << address
                   << " and reported error #"
                   << error_code
                   << ", "
                   << strerror(error_code);
                throw initialization_error(ss.str());
            }

            // it worked, save the results
            //
            f_impl->f_listen.swap(socket);
        }
        break;

    }
}


/** \brief Clean up the TCP Bio Server object.
 *
 * This function cleans up the object on destruction.
 */
tcp_bio_server::~tcp_bio_server()
{
}


/** \brief Get the IP address bound to this TCP server.
 *
 * This function retrieves the IP address used to bound this TCP server.
 *
 * The server listens on an IP address that was bound to it in the
 * constructor. If the socket is still open, then this function returns
 * that IP address. If the socket was called, then the functions returns
 * the default IP address (:: or 0.0.0.0).
 *
 * If you allow the default IP address (:: or 0.0.0.0) to be used on this
 * socket and this function returns that address, you want to also check
 * that get_socket() does not return -1 to make sure that the returned
 * address represents a valid address.
 *
 * \return The IP address bound to this server.
 */
addr::addr tcp_bio_server::get_address() const
{
    addr::addr a;
    int const s(get_socket());
    if(s >= 0)
    {
        a.set_from_socket(s, false);
    }
    return a;
}


/** \brief Return the current status of the keepalive flag.
 *
 * This function returns the current status of the keepalive flag. This
 * flag is set to true by default (in the constructor.) It can be
 * changed with the set_keepalive() function.
 *
 * The flag is used to mark new connections with the SO_KEEPALIVE flag.
 * This is used whenever a service may take a little to long to answer
 * and avoid losing the TCP connection before the answer is sent to
 * the client.
 *
 * \warning
 * It is very likely that the BIO interface forces the keepalive flag
 * automatically so even if false here, it is very likely that your
 * connection will either way be marked as keepalive.
 *
 * \return The current status of the keepalive flag.
 */
bool tcp_bio_server::get_keepalive() const
{
    return f_impl->f_keepalive;
}


/** \brief Set the keepalive flag.
 *
 * This function sets the keepalive flag to either true (i.e. mark connection
 * sockets with the SO_KEEPALIVE flag) or false. The default is true (as set
 * in the constructor,) because in most cases this is a feature people want.
 *
 * \warning
 * The keepalive flag is likely force within the BIO interface, so setting it
 * to true here (which is the default anyway) probably has no real effect
 * (i.e. the fact is set a second time).
 *
 * \param[in] yes  Whether to keep new connections alive even when no traffic
 * goes through.
 */
void tcp_bio_server::set_keepalive(bool yes)
{
    f_impl->f_keepalive = yes;
}


/** \brief Return the current status of the close_on_exec flag.
 *
 * This function returns the current status of the close_on_exec flag. This
 * flag is set to false by default (in the constructor.) It can be
 * changed with the set_close_on_exec() function.
 *
 * The flag is used to atomically mark new connections with the FD_CLOEXEC
 * flag. This prevents child processes from inheriting the socket (i.e. if
 * you use the system() function, for example, that process would inherit
 * your socket).
 *
 * \return The current status of the close_on_exec flag.
 */
bool tcp_bio_server::get_close_on_exec() const
{
    return f_impl->f_close_on_exec;
}


/** \brief Set the close_on_exec flag.
 *
 * This function sets the close_on_exec flag to either true (i.e. mark connection
 * sockets with the FD_CLOEXEC flag) or false. The default is false (as set
 * in the constructor,) because in our legacy code, the flag is not expected
 * to be set.
 *
 * \note
 * When set to true, the FD_CLOEXEC is also set on the listening socket so
 * the child can't snatch connections from under our feet.
 *
 * \warning
 * This is not thread safe. The BIO_do_accept() implementation uses the
 * accept() function which then returns and we set the FD_CLOEXEC flag
 * on the socket. This means it's not secure if you use exec() in a
 * separate thread (i.e. it may share the socket anyway unless your accept
 * is protected from such things). If you need to have a separate process,
 * look into using a fork() instead of force close the sockets in the
 * child process.
 *
 * \param[in] yes  Whether to close on exec() or not.
 */
void tcp_bio_server::set_close_on_exec(bool yes)
{
    f_impl->f_close_on_exec = yes;

    if(yes)
    {
        // retrieve the socket (we do not yet have a bio_client object
        // so we cannot call a get_socket() function...)
        //
        int socket(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_impl->f_listen.get(), &socket);
#pragma GCC diagnostic pop
        if(socket >= 0)
        {
            // if this call fails, we ignore the error, but still log the event
            //
            if(fcntl(socket, F_SETFD, FD_CLOEXEC) != 0)
            {
                int const e(errno);
                SNAP_LOG_WARNING
                    << "tcp_bio_server::set_close_on_exec(): an error occurred trying"
                       " to mark accepted socket with FD_CLOEXEC ("
                    << e
                    << ", "
                    << strerror(e)
                    << ")."
                    << SNAP_LOG_SEND;
            }
        }
    }
}


/** \brief Tell you whether the server uses a secure BIO or not.
 *
 * This function checks whether the BIO is using encryption (true)
 * or is a plain connection (false).
 *
 * \return true if the BIO was created in secure mode.
 */
bool tcp_bio_server::is_secure() const
{
    return f_impl->f_ssl_ctx != nullptr;
}


/** \brief Get the listening socket.
 *
 * This function returns the file descriptor of the listening socket.
 * By default the socket is in blocking mode.
 *
 * \return The listening socket file descriptor.
 */
int tcp_bio_server::get_socket() const
{
    if(f_impl->f_listen)
    {
        int c(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_impl->f_listen.get(), &c);
#pragma GCC diagnostic pop
        return c;
    }

    return -1;
}


/** \brief Retrieve one new connection.
 *
 * This function waits until a new connection arrives and returns a
 * new bio_client object for each new connection.
 *
 * If the socket is made non-blocking then the function may return without
 * a bio_client object (i.e. a null pointer instead.)
 *
 * \exception runtime_error
 * If the accept() fails returning a new valid client connection, then
 * this error is raised.
 *
 * \return A shared pointer to a newly allocated bio_client object.
 */
tcp_bio_client::pointer_t tcp_bio_server::accept()
{
    // TBD: does one call to BIO_do_accept() accept at most one connection
    //      at a time or could it be that 'r' will be set to 2, 3, 4...
    //      as more connections get accepted?
    //
    int const r(BIO_do_accept(f_impl->f_listen.get()));
    if(r <= 0)
    {
        // TBD: should we instead return an empty shared pointer in this case?
        //
        detail::bio_log_errors();
        throw runtime_error("failed accepting a new BIO client");
    }

    // retrieve the new connection by "popping it"
    //
    std::shared_ptr<BIO> bio; // use reset(), see SNAP-507
    bio.reset(BIO_pop(f_impl->f_listen.get()), detail::bio_deleter);
    if(bio == nullptr)
    {
        detail::bio_log_errors();
        throw runtime_error("failed retrieving the accepted BIO");
    }

    // mark the new connection with the SO_KEEPALIVE flag
    //
    // note that it is likely that the BIO implementation already does that
    // automatically once the accept() succeeded.
    //
    if(f_impl->f_keepalive)
    {
        // retrieve the socket (we do not yet have a bio_client object
        // so we cannot call a get_socket() function...)
        //
        int socket(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(bio.get(), &socket);
#pragma GCC diagnostic pop
        if(socket >= 0)
        {
            // if this call fails, we ignore the error, but still log the event
            //
            int optval(1);
            socklen_t const optlen(sizeof(optval));
            if(setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
            {
                SNAP_LOG_WARNING
                    << "tcp_bio_server::accept(): an error occurred trying"
                       " to mark accepted socket with SO_KEEPALIVE."
                    << SNAP_LOG_SEND;
            }
        }
    }

    // force a close on exec() to avoid sharing the socket in child processes
    if(f_impl->f_close_on_exec)
    {
        // retrieve the socket (we do not yet have a bio_client object
        // so we cannot call a get_socket() function...)
        //
        int socket(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(bio.get(), &socket);
#pragma GCC diagnostic pop
        if(socket >= 0)
        {
            // if this call fails, we ignore the error, but still log the event
            //
            if(fcntl(socket, F_SETFD, FD_CLOEXEC) != 0)
            {
                SNAP_LOG_WARNING
                    << "tcp_bio_server::accept(): an error occurred trying"
                       " to mark accepted socket with FD_CLOEXEC."
                    << SNAP_LOG_SEND;
            }
        }
    }

    // tcp_bio_client is private so we can't use the std::make_shared<>
    //
    tcp_bio_client::pointer_t client(new tcp_bio_client);

    client->f_impl->f_bio = bio;

    // define this computer's address (otherwise it remains at "default")
    {
        int const socket(client->get_socket());
        if(socket >= 0)
        {
            client->f_address.set_from_socket(socket, false);
        }
    }

    // TODO: somehow this does not seem to give us any information
    //       about the cipher and other details...
    //
    //       this is because it is (way) too early, we did not even
    //       receive the HELLO yet!
    //
    SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
    if(ssl != nullptr)
    {
        char const * cipher_name(SSL_get_cipher(ssl));
        int cipher_bits(0);
        SSL_get_cipher_bits(ssl, &cipher_bits);
        SNAP_LOG_DEBUG
            << "accepted BIO client with SSL cipher \""
            << cipher_name
            << "\" representing "
            << cipher_bits
            << " bits of encryption."
            << SNAP_LOG_SEND;
    }

    return client;
}



} // namespace ed
// vim: ts=4 sw=4 et

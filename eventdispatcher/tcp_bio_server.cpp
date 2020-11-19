// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// self
//
#include    "eventdispatcher/tcp_bio_server.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_private.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// OpenSSL lib
//
#include    <openssl/ssl.h>


// C lib
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




/** \brief Contruct a tcp_bio_server object.
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
 * \warning
 * Currently the max_connections parameter is pretty much ignored since
 * there is no way to pass that paramter down to the BIO interface. In
 * that code they use the SOMAXCONN definition which under Linux is
 * defined at 128 (Ubuntu 16.04.1). See:
 * /usr/include/x86_64-linux-gnu/bits/socket.h
 *
 * \param[in] addr_port  The address and port defined in an addr object.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] certificate  The server certificate filename (PEM).
 * \param[in] private_key  The server private key filename (PEM).
 * \param[in] mode  The mode used to create the listening socket.
 */
tcp_bio_server::tcp_bio_server(addr::addr const & addr_port, int max_connections, bool reuse_addr, std::string const & certificate, std::string const & private_key, mode_t mode)
    : f_impl(std::make_shared<detail::tcp_bio_server_impl>())
{
    f_impl->f_max_connections = std::clamp(max_connections <= 0 ? MAX_CONNECTIONS : max_connections, 5, 1000);
    //if(f_impl->f_max_connections < 5)
    //{
    //    f_impl->f_max_connections = 5;
    //}
    //else if(f_impl->f_max_connections > 1000)
    //{
    //    f_impl->f_max_connections = 1000;
    //}

    detail::bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_SECURE:
        {
            // the following code is based on the example shown in the man page
            //
            //        man BIO_f_ssl
            //
            if(certificate.empty()
            || private_key.empty())
            {
                throw event_dispatcher_initialization_error("with MODE_SECURE you must specify a certificate and a private_key filename");
            }

            std::shared_ptr<SSL_CTX> ssl_ctx; // use reset(), see SNAP-507
            ssl_ctx.reset(SSL_CTX_new(SSLv23_server_method()), detail::ssl_ctx_deleter);
            if(!ssl_ctx)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed creating an SSL_CTX server object");
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
                throw event_dispatcher_initialization_error("failed initializing an SSL_CTX server object certificate");
            }

            // Assign the private key to the SSL context
            //
            if(!SSL_CTX_use_PrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
            {
                // on failure, try again again with the RSA version, just in case
                // (probably useless?)
                //
                if(!SSL_CTX_use_RSAPrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
                {
                    detail::bio_log_errors();
                    throw event_dispatcher_initialization_error("failed initializing an SSL_CTX server object private key");
                }
            }

            // Verify that the private key and certifcate are a match
            //
            if(!SSL_CTX_check_private_key(ssl_ctx.get()))
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing an SSL_CTX server object private key");
            }

            // create a BIO connection with SSL
            //
            std::unique_ptr<BIO, void (*)(BIO *)> bio(BIO_new_ssl(ssl_ctx.get(), 0), detail::bio_deleter);
            if(!bio)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing a BIO server object");
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
                throw event_dispatcher_initialization_error("failed connecting BIO object with SSL_CTX object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            //
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // create a listening connection
            //
            std::shared_ptr<BIO> listen;  // use reset(), see SNAP-507
            listen.reset(BIO_new_accept(addr_port.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str()), detail::bio_deleter);
            if(!listen)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(listen.get(), reuse_addr ? BIO_BIND_REUSEADDR : BIO_BIND_NORMAL);

            // Attach the SSL bio to the listening BIO, this means whenever
            // a new connection is accepted, it automatically attaches it to
            // an SSL connection
            //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_accept_bios(listen.get(), bio.get());
#pragma GCC diagnostic pop

            // WARNING: the listen object takes ownership of the `bio`
            //          pointer and thus we have to make sure that we
            //          do not keep it in our unique_ptr<>().
            //
            snap::NOTUSED(bio.release());

            // Actually call bind() and listen() on the socket
            //
            // IMPORTANT NOTE: The BIO_do_accept() is overloaded, it does
            // two things: (a) it bind() + listen() when called the very
            // first time (i.e. the call right here); (b) it actually
            // accepts a client connection
            //
            int const r(BIO_do_accept(listen.get()));
            if(r <= 0)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing the BIO server socket to listen for client connections");
            }

            // it worked, save the results
            f_impl->f_ssl_ctx.swap(ssl_ctx);
            f_impl->f_listen.swap(listen);

            // secure connection ready
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            std::shared_ptr<BIO> listen; // use reset(), see SNAP-507
            listen.reset(BIO_new_accept(addr_port.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str()), detail::bio_deleter);
            if(!listen)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(listen.get(), BIO_BIND_REUSEADDR);

            // Actually call bind() and listen() on the socket
            //
            // IMPORTANT NOTE: The BIO_do_accept() is overloaded, it does
            // two things: (a) it bind() + listen() when called the very
            // first time (i.e. the call right here); (b) it actually
            // accepts a client connection
            //
            int const r(BIO_do_accept(listen.get()));
            if(r <= 0)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing the BIO server socket to listen for client connections");
            }

            // it worked, save the results
            //
            f_impl->f_listen.swap(listen);
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
 * flag. This prevents child processes from inhiriting the socket (i.e. if
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
                SNAP_LOG_WARNING
                    << "tcp_bio_server::accept(): an error occurred trying"
                       " to mark accepted socket with FD_CLOEXEC."
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
        int c;
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
 * This function will wait until a new connection arrives and returns a
 * new bio_client object for each new connection.
 *
 * If the socket is made non-blocking then the function may return without
 * a bio_client object (i.e. a null pointer instead.)
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
        throw event_dispatcher_runtime_error("failed accepting a new BIO client");
    }

    // retrieve the new connection by "popping it"
    //
    std::shared_ptr<BIO> bio; // use reset(), see SNAP-507
    bio.reset(BIO_pop(f_impl->f_listen.get()), detail::bio_deleter);
    if(bio == nullptr)
    {
        detail::bio_log_errors();
        throw event_dispatcher_runtime_error("failed retrieving the accepted BIO");
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

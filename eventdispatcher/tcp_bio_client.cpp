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
#include    "eventdispatcher/tcp_bio_client.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_private.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// OpenSSL lib
//
#include    <openssl/bio.h>
#include    <openssl/err.h>
#include    <openssl/ssl.h>


// C lib
//
#include    <netdb.h>
#include    <arpa/inet.h>


// last include
//
#include    <snapdev/poison.h>




#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Snap! requires support for multiple threads in OpenSSL."
#endif

namespace ed
{



/** \class tcp_bio_client
 * \brief Create a BIO client and connect to a server, eventually with TLS.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 *
 * The BIO extension is from the OpenSSL library and it allows the client
 * to connect using SSL. At this time connections are either secure or
 * not secure. If a secure connection fails, you may attempt again without
 * TLS or other encryption mechanism.
 */


#if 0
// code to do traces
namespace
{


char const * tls_rt_type(int type)
{
    switch(type)
    {
#ifdef SSL3_RT_HEADER
    case SSL3_RT_HEADER:
        return "TLS header";
#endif
    case SSL3_RT_CHANGE_CIPHER_SPEC:
        return "TLS change cipher";
    case SSL3_RT_ALERT:
        return "TLS alert";
    case SSL3_RT_HANDSHAKE:
        return "TLS handshake";
    case SSL3_RT_APPLICATION_DATA:
        return "TLS app data";
    default:
        return "TLS Unknown";
    }
}


char const * ssl_msg_type(int ssl_ver, int msg)
{
#ifdef SSL2_VERSION_MAJOR
    if(ssl_ver == SSL2_VERSION_MAJOR)
    {
        switch(msg)
        {
        case SSL2_MT_ERROR:
            return "Error";
        case SSL2_MT_CLIENT_HELLO:
            return "Client hello";
        case SSL2_MT_CLIENT_MASTER_KEY:
            return "Client key";
        case SSL2_MT_CLIENT_FINISHED:
            return "Client finished";
        case SSL2_MT_SERVER_HELLO:
            return "Server hello";
        case SSL2_MT_SERVER_VERIFY:
            return "Server verify";
        case SSL2_MT_SERVER_FINISHED:
            return "Server finished";
        case SSL2_MT_REQUEST_CERTIFICATE:
            return "Request CERT";
        case SSL2_MT_CLIENT_CERTIFICATE:
            return "Client CERT";
        }
    }
    else
#endif
    if(ssl_ver == SSL3_VERSION_MAJOR)
    {
        switch(msg)
        {
        case SSL3_MT_HELLO_REQUEST:
            return "Hello request";
        case SSL3_MT_CLIENT_HELLO:
            return "Client hello";
        case SSL3_MT_SERVER_HELLO:
            return "Server hello";
#ifdef SSL3_MT_NEWSESSION_TICKET
        case SSL3_MT_NEWSESSION_TICKET:
            return "Newsession Ticket";
#endif
        case SSL3_MT_CERTIFICATE:
            return "Certificate";
        case SSL3_MT_SERVER_KEY_EXCHANGE:
            return "Server key exchange";
        case SSL3_MT_CLIENT_KEY_EXCHANGE:
            return "Client key exchange";
        case SSL3_MT_CERTIFICATE_REQUEST:
            return "Request CERT";
        case SSL3_MT_SERVER_DONE:
            return "Server finished";
        case SSL3_MT_CERTIFICATE_VERIFY:
            return "CERT verify";
        case SSL3_MT_FINISHED:
            return "Finished";
#ifdef SSL3_MT_CERTIFICATE_STATUS
        case SSL3_MT_CERTIFICATE_STATUS:
            return "Certificate Status";
#endif
        }
    }
    return "Unknown";
}


void ssl_trace(
        int direction,
        int ssl_ver,
        int content_type,
        void const * buf, size_t len,
        SSL * ssl,
        void * userp)
{
    snapdev::NOT_USED(ssl, userp);

    std::stringstream out;
    char const * msg_name;
    int msg_type;

    // VERSION
    //
    out << SSL_get_version(ssl);

    // DIRECTION
    //
    out << (direction == 0 ? " (IN), " : " (OUT), ");

    // keep only major version
    //
    ssl_ver >>= 8;

    // TLS RT NAME
    //
    if(ssl_ver == SSL3_VERSION_MAJOR
    && content_type != 0)
    {
        out << tls_rt_type(content_type);
    }
    else
    {
        out << "(no tls_tr_type)";
    }

    if(len >= 1)
    {
        msg_type = * reinterpret_cast<unsigned char const *>(buf);
        msg_name = ssl_msg_type(ssl_ver, msg_type);

        out << ", ";
        out << msg_name;
        out << " (";
        out << std::to_string(msg_type);
        out << "):";
    }

    out << std::hex;
    for(size_t line(0); line < len; line += 16)
    {
        out << std::endl
            << (direction == 0 ? "<" : ">")
            << " "
            << std::setfill('0') << std::setw(4) << line
            << "-  ";
        size_t idx;
        for(idx = 0; line + idx < len && idx < 16; ++idx)
        {
            if(idx == 8)
            {
                out << "   ";
            }
            else
            {
                out << " ";
            }
            int const c(reinterpret_cast<unsigned char const *>(buf)[line + idx]);
            out << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        for(; idx < 16; ++idx)
        {
            if(idx == 8)
            {
                out << "  ";
            }
            out << "   ";
        }
        out << "   ";
        for(idx = 0; line + idx < len && idx < 16; ++idx)
        {
            if(idx == 8)
            {
                out << " ";
            }
            char c(reinterpret_cast<char const *>(buf)[line + idx]);
            if(c < ' ' || c > '~')
            {
                c = '.';
            }
            out << c;
        }
    }

    std::cerr << out.str() << std::endl;
}


} // no name namespace
#endif











/** \brief Construct a tcp_bio_client object.
 *
 * The tcp_bio_client constructor initializes a BIO connector and connects
 * to the specified server. The server is defined with the \p address
 * specified as parameter. The connection tries to use TLS if the \p mode
 * parameter is set to MODE_SECURE. Note that you may force a secure
 * connection using MODE_SECURE_REQUIRED. With MODE_SECURE,
 * the connection to the server can be obtained even if a secure
 * connection was not available.
 *
 * \todo
 * Create another client with BIO_new_socket() so one can create an SSL
 * connection with a socket retrieved from an accept() call.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_initialization_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] address  The address of the server to connect to. It must be valid.
 * \param[in] mode  Whether to use SSL when connecting.
 * \param[in] opt  Additional options.
 */
tcp_bio_client::tcp_bio_client(
              addr::addr const & address
            , mode_t mode
            , tcp_bio_options const & opt)
    : f_address(address)
    , f_impl(std::make_shared<detail::tcp_bio_client_impl>())
{
    if(address.is_default())
    {
        throw event_dispatcher_invalid_parameter("the default address is not valid for a client socket");
    }

    detail::bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_SECURE:
    case mode_t::MODE_ALWAYS_SECURE:
        {
            // Use TLS v1 only as all versions of SSL are flawed...
            // (see below the SSL_CTX_set_options() for additional details
            // about that since here it does indeed say SSLv23...)
            //
            std::shared_ptr<SSL_CTX> ssl_ctx; // use a reset(), see SNAP-507
            ssl_ctx.reset(SSL_CTX_new(SSLv23_client_method()), detail::ssl_ctx_deleter);
            if(ssl_ctx == nullptr)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed creating an SSL_CTX object");
            }

            // allow up to `depth` certificates in the chain otherwise fail
            // (this is not a very strong security feature though); the depth
            // can be changed before calling this function using a
            // tcp_bio_options object
            //
            SSL_CTX_set_verify_depth(ssl_ctx.get(), opt.get_verification_depth());

            // make sure SSL v2/3 is not used, also compression in SSL is
            // known to have security issues
            //
            SSL_CTX_set_options(ssl_ctx.get(), opt.get_ssl_options());

            // limit the number of ciphers the connection can use
            if(mode == mode_t::MODE_SECURE)
            {
                // this is used by local connections and we get a very strong
                // algorithm anyway, but at this point I do not know why it
                // does not work with the limited list below...
                //
                // TODO: test with adding DH support in the server then
                //       maybe (probably) that the "HIGH" will work for
                //       this entry too...
                //
                SSL_CTX_set_cipher_list(ssl_ctx.get(), "ALL");
            }
            else
            {
                SSL_CTX_set_cipher_list(ssl_ctx.get(), "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");
            }

            // load root certificates (correct path for Ubuntu?)
            // TODO: allow client to set the path to certificates
            if(SSL_CTX_load_verify_locations(ssl_ctx.get(), nullptr, "/etc/ssl/certs") != 1)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed loading verification certificates in an SSL_CTX object");
            }
            //SSL_CTX_set_msg_callback(ssl_ctx.get(), ssl_trace);
            //SSL_CTX_set_msg_callback_arg(ssl_ctx.get(), this);

            // create a BIO connected to SSL ciphers
            //
            std::shared_ptr<BIO> bio;
            bio.reset(BIO_new_ssl_connect(ssl_ctx.get()), detail::bio_deleter);
            if(!bio)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing a BIO object");
            }

            // verify that the connection worked
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
                throw event_dispatcher_initialization_error("failed retrieving the SSL contact from BIO object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            //
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // setup the Server Name Indication (SNI)
            //
            bool using_sni(false);
            if(opt.get_sni())
            {
                std::string host(opt.get_host());
                if(host.empty()
                && !address.is_hostname_an_ip())
                {
                    // addr is not an IP address written as is,
                    // it must be a hostname
                    //
                    host = address.get_hostname();
                }
                if(!host.empty())
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
                    // not only old style cast (but it's C, so expected)
                    // but they want a non-constant pointer!?
                    //
                    SSL_set_tlsext_host_name(ssl, const_cast<char *>(host.c_str()));
#pragma GCC diagnostic pop
                    using_sni = true;
                }
            }

            // TODO: other SSL initialization?

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY).c_str()));
            BIO_set_conn_port(bio.get(), const_cast<char *>(std::to_string(address.get_port()).c_str()));
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            //
            if(BIO_do_connect(bio.get()) <= 0)
            {
                if(!using_sni)
                {
                    SNAP_LOG_WARNING
                        << "the SNI feature is turned off,"
                           " often failure to connect with SSL is because the"
                           " SSL Hello message is missing the SNI (Server Name In)."
                           " See the tcp_bio_options::set_sni()."
                        << SNAP_LOG_SEND;
                }
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("SSL BIO_do_connect() failed connecting BIO object to server");
            }

            // encryption handshake
            //
            if(BIO_do_handshake(bio.get()) != 1)
            {
                if(!using_sni)
                {
                    SNAP_LOG_WARNING
                        << "the SNI feature is turned off,"
                           " often failure to connect with SSL is because the"
                           " SSL Hello message is missing the SNI (Server Name In)."
                           " See the tcp_bio_options::set_sni()."
                        << SNAP_LOG_SEND;
                }
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed establishing a secure BIO connection with server, handshake failed."
                            " Often such failures to process SSL is because the SSL Hello message is missing the SNI (Server Name In)."
                            " See the tcp_bio_options::set_sni().");
            }

            // verify that the peer certificate was signed by a
            // recognized root authority
            //
            if(SSL_get_peer_certificate(ssl) == nullptr)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("peer failed presenting a certificate for security verification");
            }

            // XXX: check that the call below is similar to the example
            //      usage of SSL_CTX_set_verify() which checks the name
            //      of the certificate, etc.
            //
            if(SSL_get_verify_result(ssl) != X509_V_OK)
            {
                if(mode != mode_t::MODE_SECURE)
                {
                    detail::bio_log_errors();
                    throw event_dispatcher_initialization_error("peer certificate could not be verified");
                }
                SNAP_LOG_WARNING
                    << "connecting with SSL but certificate verification failed."
                    << SNAP_LOG_SEND;
            }

            // it worked, save the results
            //
            f_impl->f_ssl_ctx.swap(ssl_ctx);
            f_impl->f_bio.swap(bio);

            // secure connection ready
            //
            char const * cipher_name(SSL_get_cipher(ssl));
            int cipher_bits(0);
            SSL_get_cipher_bits(ssl, &cipher_bits);
            SNAP_LOG_DEBUG
                << "connected with SSL cipher \""
                << cipher_name
                << "\" representing "
                << cipher_bits
                << " bits of encryption."
                << SNAP_LOG_SEND;
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            // create a plain BIO connection
            //
            std::shared_ptr<BIO> bio;  // use reset(), see SNAP-507
            bio.reset(BIO_new(BIO_s_connect()), detail::bio_deleter);
            if(!bio)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed initializing a BIO object");
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY).c_str()));
            BIO_set_conn_port(bio.get(), const_cast<char *>(std::to_string(address.get_port()).c_str()));
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            //
            if(BIO_do_connect(bio.get()) <= 0)
            {
                detail::bio_log_errors();
                throw event_dispatcher_initialization_error("failed connecting BIO object to server");
            }

            // it worked, save the results
            //
            f_impl->f_bio.swap(bio);

            // plain connection ready
        }
        break;

    }

    if(opt.get_keepalive())
    {
        // retrieve the socket (we are still in the constructor so avoid
        // calling other functions...)
        //
        int socket(-1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_impl->f_bio.get(), &socket);
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
                    << "an error occurred trying to mark client socket with SO_KEEPALIVE."
                    << SNAP_LOG_SEND;
            }
        }
    }
}


/** \brief Create a BIO client object from an actual BIO pointer.
 *
 * This function is called by the server whenever it accepts a new BIO
 * connection. The server then can return the tcp_bio_client object instead
 * of a BIO object.
 *
 * The BIO is saved directly in the f_impl class (the server is given access.)
 */
tcp_bio_client::tcp_bio_client()
    : f_impl(std::make_shared<detail::tcp_bio_client_impl>())
{
}


/** \brief Clean up the BIO client object.
 *
 * This function cleans up the BIO client object by freeing the SSL_CTX
 * and the BIO objects.
 */
tcp_bio_client::~tcp_bio_client()
{
    // f_bio and f_ssl_ctx are allocated using shared pointers with
    // a deleter so we have nothing to do here.
}


/** \brief Close the connection.
 *
 * This function closes the connection by losing the f_bio object.
 *
 * As we are at it, we also lose the SSL context since we are not going
 * to use it anymore either.
 */
void tcp_bio_client::close()
{
    f_impl->f_bio.reset();
    f_impl->f_ssl_ctx.reset();
}


/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \note
 * If the socket was closed, then the function returns -1.
 *
 * \warning
 * This socket is generally managed by the BIO library and thus it may
 * create unwanted side effects to change the socket under the feet of
 * the BIO library...
 *
 * \return The socket descriptor.
 */
int tcp_bio_client::get_socket() const
{
    if(f_impl->f_bio)
    {
        int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_impl->f_bio.get(), &c);
#pragma GCC diagnostic pop
        return c;
    }

    return -1;
}


/** \brief Return a reference to the address used to connect.
 *
 * This function returns a reference to the address used to connect to
 * a server. This is the server IP address and port information.
 *
 * If specified, the host (i.e. the domain name) used can be retrieved
 * using the get_host() function of the addr object returned.
 *
 * \return The address as specified in the constructor.
 */
addr::addr tcp_bio_client::get_address() const
{
    return f_address;
}


/** \brief Get the address of the client computer.
 *
 * This function retrieves the address and port of the client computer
 * (i.e. yourself). The port will have been auto-selected, so this is
 * useful if you need that information which is not otherwise available.
 *
 * The function caches the information, which will continue to be available
 * even after the connection is closed (assuming it was retrieved at least
 * once while the connection was up).
 *
 * \return A reference to the remote computer address.
 */
addr::addr tcp_bio_client::get_client_address()
{
    if(f_client_address.is_default())
    {
        int const s(get_socket());
        if(s > 0)
        {
            f_client_address.set_from_socket(s, false);
        }
    }

    return f_client_address;
}


/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \note
 * If the connection was closed, this function returns -1.
 *
 * \warning
 * When the function returns zero, it is likely that the server closed
 * the connection. It may also be that the buffer was empty and that
 * the BIO decided to return early. Since we use a blocking mechanism
 * by default, that should not happen.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *
 * \sa read_line()
 * \sa write()
 */
int tcp_bio_client::read(char * buf, size_t size)
{
    if(!f_impl->f_bio)
    {
        errno = EBADF;
        return -1;
    }

    int const r(static_cast<int>(BIO_read(f_impl->f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        //
        detail::bio_log_errors();
        errno = EIO;
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_impl->f_bio.get()))
        {
            errno = EAGAIN;
            return 0;
        }
        // did we reach the "end of the file"? i.e. did the server
        // close our connection? (this better replicates what a
        // normal socket does when reading from a closed socket)
        //
        if(BIO_eof(f_impl->f_bio.get()))
        {
            return 0;
        }
        if(r != 0)
        {
            // the BIO generated an error
            detail::bio_log_errors();
            errno = EIO;
            return -1;
        }
    }
    return r;
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 (an empty string) when the server closes
 * the connection.
 *
 * \note
 * If the connection was closed then this function returns -1.
 *
 * \warning
 * A return value of zero can mean "empty line" and not end of file. It
 * is up to you to know whether your protocol allows for empty lines or
 * not. If so, you may not be able to make use of this function.
 *
 * \param[out] line  The resulting line read from the server. The function
 *                   first clears the contents.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network without the '\n'.
 *
 * \sa read()
 */
int tcp_bio_client::read_line(std::string & line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A BIO socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \note
 * If the connection was closed, return -1.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 *
 * \sa read()
 */
int tcp_bio_client::write(char const * buf, size_t size)
{
#ifdef _DEBUG
    // This write is useful when developing APIs against 3rd party
    // servers, otherwise, it's just too much debug
    //SNAP_LOG_TRACE
    //    << "tcp_bio_client::write(): buf="
    //    << buf
    //    << ", size="
    //    << size
    //    << SNAP_LOG_SEND;
#endif
    if(!f_impl->f_bio)
    {
        errno = EBADF;
        return -1;
    }

    int const r(static_cast<int>(BIO_write(f_impl->f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        detail::bio_log_errors();
        errno = EIO;
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_impl->f_bio.get()))
        {
            errno = EAGAIN;
            return 0;
        }
        // the BIO generated an error (TBD should we check BIO_eof() too?)
        detail::bio_log_errors();
        errno = EIO;
        return -1;
    }
    BIO_flush(f_impl->f_bio.get());
    return r;
}



} // namespace ed
// vim: ts=4 sw=4 et

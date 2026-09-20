// Copyright (c) 2012-2026  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#include    "eventdispatcher/tcp_bio_options.h"

#include    "eventdispatcher/tcp_private.h"
#include    "eventdispatcher/exception.h"


// OpenSSL
//
#include    <openssl/bio.h>
#include    <openssl/err.h>


// C++
//
#include    <memory>
#include    <numeric>


// last include
//
#include    <snapdev/poison.h>




#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Snap! requires support for multiple threads in OpenSSL."
#endif

namespace ed
{



/** \brief Initialize the options object to the defaults.
 *
 * This constructor sets up the default options in this structure.
 */
tcp_bio_options::tcp_bio_options()
{
}


/** \brief Specify the depth of SSL certificate verification.
 *
 * When verifying a certificate, you may end up with a very long chain.
 * In most cases, a very long chain is not sensible and probably means
 * something fishy is going on. For this reason, this is verified here.
 *
 * The default is 4. Some people like to use 5 or 6. The full range
 * allows for way more, although really it should be very much
 * limited.
 *
 * \exception
 * This function accepts a number between 1 and 100. Any number outside
 * of that range and this exception is raised.
 *
 * \param[in] depth  The depth for the verification of certificates.
 */
void tcp_bio_options::set_verification_depth(size_t depth)
{
    if(depth == 0
    || depth > MAX_VERIFICATION_DEPTH)
    {
        throw invalid_parameter("the depth parameter must be defined between 1 and 100 inclusive");
    }

    f_verification_depth = depth;
}


/** \brief Retrieve the verification maximum depth allowed.
 *
 * This function returns the verification depth parameter. This number
 * will always be between 1 and 100 inclusive.
 *
 * The inclusive maximum is actually defined as MAX_VERIFICATION_DEPTH.
 *
 * The default depth is 4.
 *
 * \return The current verification depth.
 */
size_t tcp_bio_options::get_verification_depth() const
{
    return f_verification_depth;
}


/** \brief Change the SSL options.
 *
 * This function sets the SSL options to the new \p ssl_options
 * values.
 *
 * By default the bio_client forbids:
 *
 * * SSL version 2
 * * SSL version 3
 * * TLS version 1.0
 * * SSL compression
 *
 * which are parameter that are known to create security issues.
 *
 * To make it easier to add options to the defaults, the class
 * offers the DEFAULT_SSL_OPTIONS option. Just add and remove
 * bits starting from that value.
 *
 * \param[in] ssl_options  The new SSL options.
 */
void tcp_bio_options::set_ssl_options(ssl_options_t ssl_options)
{
    f_ssl_options = ssl_options;
}


/** \brief Retrieve the current SSL options.
 *
 * This function can be used to add and remove SSL options to
 * bio_client connections.
 *
 * For example, to also prevent TLS 1.1, add the new flag:
 *
 * \code
 *      bio.set_ssl_options(bio.get_ssl_options() | SSL_OP_NO_TLSv1_1);
 * \endcode
 *
 * And to allow compression, remove a flag which is set by default:
 *
 * \code
 *      bio.set_ssl_options(bio.get_ssl_options() & ~(SSL_OP_NO_COMPRESSION));
 * \endcode
 *
 * \return The current SSL options.
 */
tcp_bio_options::ssl_options_t tcp_bio_options::get_ssl_options() const
{
    return f_ssl_options;
}


/** \brief Change the default path to SSL certificates.
 *
 * By default, we define the path to the SSL certificate as defined
 * on Ubuntu. This is under "/etc/ssl/certs".
 *
 * This function let you change that path to another one. Maybe you
 * would prefer to not allow all certificates to work in your
 * circumstances.
 *
 * \param[in] path  The new path to SSL certificates used to verify
 *                  secure connections.
 */
void tcp_bio_options::set_ssl_certificate_path(std::string const path)
{
    f_ssl_certificate_path = path;
}


/** \brief Return the current SSL certificate path.
 *
 * This function returns the path where the SSL interface will
 * look for the root certificates used to verify a connection's
 * security.
 *
 * \return The current SSL certificate path.
 */
std::string const & tcp_bio_options::get_ssl_certificate_path() const
{
    return f_ssl_certificate_path;
}


/** \brief Set whether the SO_KEEPALIVE should be set.
 *
 * By default this option is turned ON meaning that all BIO_client have their
 * SO_KEEPALIVE turned on when created.
 *
 * You may turn this off if you are creating a socket for a very short
 * period of time, such as to send a fast REST command to a server.
 *
 * \attention
 * As per the TCP RFC, you should only use keepalive on a server, not a
 * client. (The client can quit any time and if it tries to access the
 * server and it fails, it can either quit or reconnect then.) That being
 * said, at times a server does not set the Keep-Alive and the client may
 * want to use it to maintain the connection when not much happens for
 * long durations.
 *
 * https://tools.ietf.org/html/rfc1122#page-101
 *
 * Some numbers about Keep-Alive:
 *
 * https://www.veritas.com/support/en_US/article.100028680
 *
 * For Linux (in seconds):
 *
 * \code
 * tcp_keepalive_time = 7200
 * tcp_keepalive_intvl = 75
 * tcp_keepalive_probes = 9
 * \endcode
 *
 * These can be access through the /proc file system:
 *
 * \code
 * /proc/sys/net/ipv4/tcp_keepalive_time
 * /proc/sys/net/ipv4/tcp_keepalive_intvl
 * /proc/sys/net/ipv4/tcp_keepalive_probes
 * \endcode
 *
 * See: http://tldp.org/HOWTO/TCP-Keepalive-HOWTO/usingkeepalive.html
 *
 * \warning
 * These numbers are used by all applications using TCP. Remember that
 * changing them will affect all your clients and servers.
 *
 * \param[in] keepalive  true if you want the SO_KEEP_ALIVE turned on.
 *
 * \sa get_keepalive()
 */
void tcp_bio_options::set_keepalive(bool keepalive)
{
    f_keepalive = keepalive;
}


/** \brief Retrieve the SO_KEEPALIVE flag.
 *
 * This function returns the current value of the SO_KEEPALIVE flag. By
 * default this is true.
 *
 * Note that this function returns the flag status in the options, not
 * a connected socket.
 *
 * \return The current status of the SO_KEEPALIVE flag (true or false).
 *
 * \sa set_keepalive()
 */
bool tcp_bio_options::get_keepalive() const
{
    return f_keepalive;
}


/** \brief Set whether the SNI should be included in the SSL request.
 *
 * Whenever SSL connects a server, it has the option to include the
 * Server Name Indication, which is the server hostname to which
 * are think you are connecting. That way the server can verify that
 * you indeed were sent to the right server.
 *
 * The default is set to true, however, if you create a bio_client
 * object using an IP address (opposed to the hostname) then no
 * SNI will be included unless you also call the set_host() function
 * to setup the host.
 *
 * In other words, you can use the IP address on the bio_client
 * constructor and the hostname in the options and you will still
 * be able to get the SNI setup as expected.
 *
 * \param[in] sni  true if you want the SNI to be included.
 *
 * \sa get_sni()
 * \sa set_host()
 */
void tcp_bio_options::set_sni(bool sni)
{
    f_sni = sni;
}


/** \brief Retrieve the SNI flag.
 *
 * This function returns the current value of the SNI flag. By
 * default this is true.
 *
 * Note that although the flag is true by default, the SSL request
 * may still not get to work if you don't include the host with
 * the set_host() and construct a bio_client object with an IP
 * address (opposed to a hostname.)
 *
 * \return The current status of the SNI (true or false).
 *
 * \sa set_sni()
 * \sa set_host()
 */
bool tcp_bio_options::get_sni() const
{
    return f_sni;
}


/** \brief Add one protocol supported by a client.
 *
 * This function adds one protocol that the client trying to connect to
 * an HTTP server knows.
 *
 * Some of the known protocol:
 * * "h2" for HTTP/2
 * * "http/1.1" for HTTP/1.1
 * * "grpc-exp" for gRPC communication over HTTP/2
 * * "x-amzn-mqtt-ca" (or "mqtt") for Amazon Web Services IoT Core Standard
 * * "STARTTLS" for SMTP/IMAP/POP3
 *
 * The idea is to avoid trying an HTTP/1.1 and Upgrade: ... to HTTP/2.
 * That does not work well with modern HTTP servers.
 *
 * You want to call the function once per protocol that your client
 * supports.
 *
 * \note
 * The \p mark_as_default flag can always be set to true in which case the
 * last protocol added is marked as the default. It is wise to only mark
 * one protocol as the default to make it clear which one is the default.
 * If none of the protocols are marked as the default, then no default
 * gets defined.
 *
 * \param[in] protocol  The name of one protocol.
 * \param[in] mark_as_default  Whether to mark that protocol as the default.
 */
void tcp_bio_options::add_alpn(std::string const & protocol, bool mark_as_default)
{
    if(protocol.empty())
    {
        throw invalid_parameter("an ALPN protocol name cannot be the empty string.");
    }
    if(protocol.length() > 255)
    {
        throw invalid_parameter(
              "an ALPN protocol name cannot be more than 255 characters; \""
            + protocol
            + "\" is invalid.");
    }

    f_alpn.push_back(protocol);

    if(mark_as_default)
    {
        f_default_alpn = protocol;
    }
}


/** \brief Clear all the ALPN entries.
 *
 * This function clears all the ALPN that added earlier. It also clears
 * the default protocol.
 */
void tcp_bio_options::clear_alpn()
{
    f_alpn.clear();
    f_default_alpn.clear();
}


/** \brief Retrieve the list of protocols.
 *
 * This function returns a reference to the list of protocols defined
 * in ALPN list.
 *
 * \return A reference to the internal list of ALPN protocol names.
 */
advgetopt::string_list_t const & tcp_bio_options::get_alpn() const
{
    return f_alpn;
}


/** \brief Get the ALPN buffer to send via SSL.
 *
 * The ALPN list of strings is transferred to the server as a block
 * of P-strings (like a Pascal ShortString). This function generates
 * that block.
 *
 * \return A buffer with all the ALPN strings transformed in P-strings.
 */
std::string tcp_bio_options::get_alpn_buffer() const
{
    std::string result;

    if(!f_alpn.empty())
    {
        std::size_t const size(std::accumulate(
              f_alpn.begin()
            , f_alpn.end()
            , f_alpn.size()
            , [](std::size_t init, std::string const & alpn)
            {
                return init + alpn.length();
            }));
        result.reserve(size);

        for(auto const & alpn : f_alpn)
        {
            result += static_cast<char>(alpn.size());
            result += alpn;
        }
    }

    return result;
}


/** \brief Return the protocol marked as the default.
 *
 * When you add protocols, one of them can be marked as the default.
 * This function returns that string.
 *
 * \return The protocol marked as the default in add_alpn().
 */
std::string const & tcp_bio_options::get_default_alpn() const
{
    return f_default_alpn;
}


/** \brief Set the hostname.
 *
 * This function is used to setup the SNI hostname.
 *
 * The Server Name Indication is added to the SSL Hello message if
 * available (i.e. the host was specified here or the bio_client
 * constructor is called with the hostname and not an IP address.)
 *
 * If you construct the bio_client object with an IP address, you
 * can use this set_host() function to specify the hostname, but
 * you still need to make sure that both are a match.
 *
 * \param[in] host  The host being accessed.
 */
void tcp_bio_options::set_host(std::string const & host)
{
    f_host = host;
}


/** \brief Retrieve the hostname.
 *
 * This function is used to retrieve the hostname. This name has
 * priority over the \p addr parameter specified to the
 * bio_client constructor.
 *
 * By default this name is empty in which case the bio_client
 * constructor checks the \p addr parameter and if it is
 * a hostname (opposed to direct IP addresses) then it uses
 * that \p addr parameter instead.
 *
 * If you do not want the Server Name Indication in the SSL
 * request, you must call set_sni(false) so even if the
 * bio_client constructor is called with a hostname, the
 * SNI won't be included in the request.
 *
 * \return A reference string with the hostname.
 */
std::string const & tcp_bio_options::get_host() const
{
    return f_host;
}



/** \brief Call the bio_cleanup() function.
 *
 * This can be used in the main() function of your tests so that way the
 * coverage tests do not detect any memory leaks from the OpenSSL libraries.
 *
 * It automatically calls the bio_cleanup() function.
 *
 * Note that this is not necessary in your standard tools and services
 * since those can have memory still allocated at the time you leave your
 * application. For tests, though, we verify memory leaks (through the
 * sanitizer) and if such are discovered, the test fails.
 *
 * Here is an example one can use to implement such. Note that the order
 * is probably not important (i.e. it just needs to get initialized
 * at some point before you quit). The tests/catch_main.cpp of this
 * very project (eventdispatcher) makes use of this feature.
 *
 * \code
 * int main(...)
 * {
 *     ...
 *     bio_auto_cleanup bio_cleanup;
 *     ...
 * }
 * \endcode
 */
bio_auto_cleanup::~bio_auto_cleanup()
{
    detail::bio_cleanup();
}



} // namespace ed
// vim: ts=4 sw=4 et

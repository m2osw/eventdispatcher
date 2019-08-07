// Event Dispatcher
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

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// self
//
#include "eventdispatcher/tcp_bio_options.h"

#include "eventdispatcher/exception.h"


// OpenSSL lib
//
#include <openssl/bio.h>
#include <openssl/err.h>


// C++
//
#include <memory>


// last include
//
#include "snapdev/poison.h"




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
        throw event_dispatcher_invalid_parameter("the depth parameter must be defined between 1 and 100 inclusive");
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
 * By default the bio_clent forbids:
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
 * \return A referemce string with the hostname.
 */
std::string const & tcp_bio_options::get_host() const
{
    return f_host;
}



} // namespace ed
// vim: ts=4 sw=4 et

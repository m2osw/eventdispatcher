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

/** \file
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


// self
//
#include "eventdispatcher/snap_communicator.h"

#include "eventdispatcher/snap_communicator_dispatcher.h"


// snaplogger lib
//
#include "snaplogger/message.h"


// snapdev lib
//
#include "snapdev/not_reached.h"
#include "snapdev/not_used.h"
#include "snapdev/string_replace_many.h"


// libaddr lib
//
#include "libaddr/addr_parser.h"


// C++ lib
//
#include <sstream>
#include <limits>
#include <atomic>


// C lib
//
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>


// last include
//
#include <snapdev/poison.h>



namespace ed
{
namespace
{


/** \brief The instance of the snap_communicator singleton.
 *
 * This pointer is the one instance of the snap_communicator
 * we create to run an event loop.
 */
snap_communicator::pointer_t        g_instance;


} // no name namespace






































































































////////////////////////////////
// Snap TCP Buffer Connection //
////////////////////////////////

/** \brief Initialize a client socket.
 *
 * The client socket gets initialized with the specified 'socket'
 * parameter.
 *
 * This constructor creates a writer connection too. This gives you
 * a read/write connection. You can get the writer with the writer()
 * function. So you may write data with:
 *
 * \code
 *      my_reader.writer().write(buf, buf_size);
 * \endcode
 *
 * \param[in] addr  The address to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  The mode to connect as (PLAIN or SECURE).
 * \param[in] blocking  If true, keep a blocking socket, other non-blocking.
 */
snap_communicator::snap_tcp_client_buffer_connection::snap_tcp_client_buffer_connection(std::string const & addr, int const port, mode_t const mode, bool const blocking)
    : snap_tcp_client_connection(addr, port, mode)
{
    if(!blocking)
    {
        non_blocking();
    }
}


/** \brief Check whether this connection still has some input in its buffer.
 *
 * This function returns true if there is partial incoming data in this
 * object's buffer.
 *
 * \return true if some buffered input is waiting for completion.
 */
bool snap_communicator::snap_tcp_client_buffer_connection::has_input() const
{
    return !f_line.empty();
}



/** \brief Check whether this connection still has some output in its buffer.
 *
 * This function returns true if there is still some output in the client
 * buffer. Output is added by the write() function, which is called by
 * the send_message() function.
 *
 * \return true if some buffered output is waiting to be sent out.
 */
bool snap_communicator::snap_tcp_client_buffer_connection::has_output() const
{
    return !f_output.empty();
}



/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchroneous.
 *
 * \todo
 * Optimization: look into writing the \p data buffer directly in
 * the socket if the f_output cache is empty. If that works then
 * we can completely bypass our intermediate cache. This works only
 * if we make sure that the socket is non-blocking, though.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_signal() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 *
 * \return The number of bytes that were saved in our buffer, 0 if
 *         no data was written to the buffer (i.e. the socket is
 *         closed, length is zero, or data is a null pointer.)
 */
ssize_t snap_communicator::snap_tcp_client_buffer_connection::write(void const * data, size_t length)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }

    if(data != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(data));
        f_output.insert(f_output.end(), d, d + length);
        return length;
    }

    return 0;
}


/** \brief The buffer is a writer when the output buffer is not empty.
 *
 * This function returns true as long as the output buffer of this
 * client connection is not empty.
 *
 * \return true if the output buffer is not empty, false otherwise.
 */
bool snap_communicator::snap_tcp_client_buffer_connection::is_writer() const
{
    return get_socket() != -1 && !f_output.empty();
}


/** \brief Instantiation of process_read().
 *
 * This function reads incoming data from a socket.
 *
 * The function is what manages our low level TCP/IP connection protocol
 * which is to read one line of data (i.e. bytes up to the next '\n'
 * character; note that '\r' are not understood.)
 *
 * Once a complete line of data was read, it is converted to UTF-8 and
 * sent to the next layer using the process_line() function passing
 * the line it just read (without the '\n') to that callback.
 *
 * \sa process_write()
 * \sa process_line()
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    if(get_socket() != -1)
    {
        int count_lines(0);
        int64_t const date_limit(snap_communicator::get_current_date() + f_processing_time_limit);
        std::vector<char> buffer;
        buffer.resize(1024);
        for(;;)
        {
            errno = 0;
            ssize_t const r(read(&buffer[0], buffer.size()));
            if(r > 0)
            {
                for(ssize_t position(0); position < r; )
                {
                    std::vector<char>::const_iterator it(std::find(buffer.begin() + position, buffer.begin() + r, '\n'));
                    if(it == buffer.begin() + r)
                    {
                        // no newline, just add the whole thing
                        f_line += std::string(&buffer[position], r - position);
                        break; // do not waste time, we know we are done
                    }

                    // retrieve the characters up to the newline
                    // character and process the line
                    //
                    f_line += std::string(&buffer[position], it - buffer.begin() - position);
                    process_line(QString::fromUtf8(f_line.c_str()));
                    ++count_lines;

                    // done with that line
                    //
                    f_line.clear();

                    // we had a newline, we may still have some data
                    // in that buffer; (+1 to skip the '\n' itself)
                    //
                    position = it - buffer.begin() + 1;
                }

                // when we reach here all the data read in `buffer` is
                // now either fully processed or in f_line
                //
                // TODO: change the way this works so we can test the
                //       limit after each process_line() call
                //
                if(count_lines >= f_event_limit
                || snap_communicator::get_current_date() >= date_limit)
                {
                    // we reach one or both limits, stop processing so
                    // the other events have a chance to run
                    //
                    break;
                }
            }
            else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                break;
            }
            else //if(r < 0)
            {
                // TODO: do something about the error
                int const e(errno);
                SNAP_LOG_ERROR("an error occurred while reading from socket (errno: ")(e)(" -- ")(strerror(e))(").");
                process_error();
                return;
            }
        }
    }

    // process next level too
    snap_tcp_client_connection::process_read();
}


/** \brief Instantiation of process_write().
 *
 * This function writes outgoing data to a socket.
 *
 * This function manages our own internal cache, which we use to allow
 * for out of synchronization (non-blocking) output.
 *
 * When the output buffer goes empty, this function calls the
 * process_empty_buffer() callback.
 *
 * \sa write()
 * \sa process_read()
 * \sa process_empty_buffer()
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_write()
{
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(snap_tcp_client_connection::write(&f_output[f_position], f_output.size() - f_position));
        if(r > 0)
        {
            // some data was written
            f_position += r;
            if(f_position >= f_output.size())
            {
                f_output.clear();
                f_position = 0;
                process_empty_buffer();
            }
        }
        else if(r < 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, generate an error
            //
            int const e(errno);
            SNAP_LOG_ERROR("an error occurred while writing to socket of \"")(f_name)("\" (errno: ")(e)(" -- ")(strerror(e))(").");
            process_error();
            return;
        }
    }

    // process next level too
    snap_tcp_client_connection::process_write();
}


/** \brief The hang up event occurred.
 *
 * This function closes the socket and then calls the previous level
 * hang up code which removes this connection from the snap_communicator
 * object it was last added in.
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    // process next level too
    snap_tcp_client_connection::process_hup();
}


/** \fn snap_communicator::snap_tcp_client_buffer_connection::process_line(QString const & line);
 * \brief Process a line of data.
 *
 * This is the default virtual class that can be overridden to implement
 * your own processing. By default this function does nothing.
 *
 * \note
 * At this point I implemented this function so one can instantiate
 * a snap_tcp_server_client_buffer_connection without having to
 * derive it, although I do not think that is 100% proper.
 *
 * \param[in] line  The line of data that was just read from the input
 *                  socket.
 */





///////////////////////////////////////////////
// Snap TCP Server Message Buffer Connection //
///////////////////////////////////////////////

/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \param[in] addr  The address to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  Use this mode to connect as (PLAIN, ALWAYS_SECURE or SECURE).
 * \param[in] blocking  Whether to keep the socket blocking or make it
 *                      non-blocking.
 */
snap_communicator::snap_tcp_client_message_connection::snap_tcp_client_message_connection(std::string const & addr, int const port, mode_t const mode, bool const blocking)
    : snap_tcp_client_buffer_connection(addr, port, mode, blocking)
{
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message (snap_communicator_message)
 * and then calls the process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void snap_communicator::snap_tcp_client_message_connection::process_line(QString const & line)
{
    if(line.isEmpty())
    {
        return;
    }

    snap_communicator_message message;
    if(message.from_message(line))
    {
        dispatch_message(message);
    }
    else
    {
        // TODO: what to do here? This could be that the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR("snap_communicator::snap_tcp_client_message_connection::process_line() was asked to process an invalid message (")(line)(")");
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \exception snap_communicator_runtime_error
 * This function throws this exception if the write() to the pipe
 * fails to write the entire message. This should only happen if
 * the pipe gets severed.
 *
 * \param[in] message  The message to be sent.
 * \param[in] cache  Whether to cache the message if there is no connection.
 *
 * \return Always true, although if an error occurs the function throws.
 */
bool snap_communicator::snap_tcp_client_message_connection::send_message(snap_communicator_message const & message, bool cache)
{
    NOTUSED(cache);

    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    QString const msg(message.to_message());
    QByteArray const utf8(msg.toUtf8());
    std::string buf(utf8.data(), utf8.size());
    buf += "\n";
    return write(buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
}








////////////////////////////////
// Snap TCP Server Connection //
////////////////////////////////


/** \brief Initialize a server connection.
 *
 * This function is used to initialize a server connection, a TCP/IP
 * listener which can accept() new connections.
 *
 * The connection uses a \p mode parameter which can be set to MODE_PLAIN,
 * in which case the \p certificate and \p private_key parameters are
 * ignored, or MODE_SECURE.
 *
 * This connection supports secure SSL communication using a certificate
 * and a private key. These have to be specified as filenames. The
 * `snapcommunicator` daemon makes use of files defined under
 * "/etc/snapwebsites/ssl/..." by default.
 *
 * These files are created using this command line:
 *
 * \code
 * openssl req \
 *     -newkey rsa:2048 -nodes -keyout ssl-test.key \
 *     -x509 -days 3650 -out ssl-test.crt
 * \endcode
 *
 * Then pass "ssl-test.crt" as the certificate and "ssl-test.key"
 * as the private key.
 *
 * \todo
 * Add support for DH connections. Since our snapcommunicator connections
 * are mostly private, it should not be a huge need at this point, though.
 *
 * \todo
 * Add support for verified certificates. Right now we do not create
 * signed certificates. This does not prevent fully secure transactions,
 * it just cannot verify that the computer on the other side is correct.
 *
 * \warning
 * The \p max_connections parameter is currently ignored because the
 * BIO implementation does not give you an API to change that parameter.
 * That being said, they default to the maximum number that the Linux
 * kernel will accept so it should be just fine.
 *
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 * \param[in] certificate  The filename to a .pem file.
 * \param[in] private_key  The filename to a .pem file.
 * \param[in] mode  The mode to use to open the connection (PLAIN or SECURE.)
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 */
snap_communicator::snap_tcp_server_connection::snap_tcp_server_connection(
                  std::string const & addr
                , int port
                , std::string const & certificate
                , std::string const & private_key
                , mode_t mode
                , int max_connections
                , bool reuse_addr)
    : bio_server(addr::string_to_addr(addr, "", port, "tcp"), max_connections, reuse_addr, certificate, private_key, mode)
{
}


/** \brief Reimplement the is_listener() for the snap_tcp_server_connection.
 *
 * A server connection is a listener socket. The library makes
 * use of a completely different callback when a "read" event occurs
 * on these connections.
 *
 * The callback is expected to create the new connection and add
 * it the communicator.
 *
 * \return This version of the function always returns true.
 */
bool snap_communicator::snap_tcp_server_connection::is_listener() const
{
    return true;
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the tcp_server class.
 *
 * \return The socket of this client connection.
 */
int snap_communicator::snap_tcp_server_connection::get_socket() const
{
    return bio_server::get_socket();
}







///////////////////////////////////////
// Snap TCP Server Client Connection //
///////////////////////////////////////


/** \brief Create a client connection created from an accept().
 *
 * This constructor initializes a client connection from a socket
 * that we received from an accept() call.
 *
 * The destructor will automatically close that socket on destruction.
 *
 * \param[in] client  The client that acecpt() returned.
 */
snap_communicator::snap_tcp_server_client_connection::snap_tcp_server_client_connection(tcp_client_server::bio_client::pointer_t client)
    : f_client(client)
{
}


/** \brief Make sure the socket gets released once we are done witht he connection.
 *
 * This destructor makes sure that the socket gets closed.
 */
snap_communicator::snap_tcp_server_client_connection::~snap_tcp_server_client_connection()
{
    close();
}


/** \brief Read data from the TCP server client socket.
 *
 * This function reads as much data up to the specified amount
 * in \p count. The read data is saved in \p buf.
 *
 * \param[in,out] buf  The buffer where the data gets read.
 * \param[in] count  The maximum number of bytes to read in buf.
 *
 * \return The number of bytes read or -1 if an error occurred.
 */
ssize_t snap_communicator::snap_tcp_server_client_connection::read(void * buf, size_t count)
{
    if(!f_client)
    {
        errno = EBADF;
        return -1;
    }
    return f_client->read(reinterpret_cast<char *>(buf), count);
}


/** \brief Write data to this connection's socket.
 *
 * This function writes up to \p count bytes of data from \p buf
 * to this connection's socket.
 *
 * \warning
 * This write function may not always write all the data you are
 * trying to send to the remote connection. If you want to make
 * sure that all your data is written to the other connection,
 * you want to instead use the snap_tcp_server_client_buffer_connection
 * which overloads the write() function and saves the data to be
 * written to the socket in a buffer. The snap communicator run
 * loop is then responsible for sending all the data.
 *
 * \param[in] buf  The buffer of data to be written to the socket.
 * \param[in] count  The number of bytes the caller wants to write to the
 *                   conneciton.
 *
 * \return The number of bytes written to the socket or -1 if an error occurred.
 */
ssize_t snap_communicator::snap_tcp_server_client_connection::write(void const * buf, size_t count)
{
    if(!f_client)
    {
        errno = EBADF;
        return -1;
    }
    return f_client->write(reinterpret_cast<char const *>(buf), count);
}


/** \brief Close the socket of this connection.
 *
 * This function is automatically called whenever the object gets
 * destroyed (see destructor) or detects that the client closed
 * the network connection.
 *
 * Connections cannot be reopened.
 */
void snap_communicator::snap_tcp_server_client_connection::close()
{
    f_client.reset();
}


/** \brief Retrieve the socket of this connection.
 *
 * This function returns the socket defined in this connection.
 */
int snap_communicator::snap_tcp_server_client_connection::get_socket() const
{
    if(f_client == nullptr)
    {
        // client connection was closed
        //
        return -1;
    }
    return f_client->get_socket();
}


/** \brief Tell that we are always a reader.
 *
 * This function always returns true meaning that the connection is
 * always of a reader. In most cases this is safe because if nothing
 * is being written to you then poll() never returns so you do not
 * waste much time in have a TCP connection always marked as a
 * reader.
 *
 * \return The events to listen to for this connection.
 */
bool snap_communicator::snap_tcp_server_client_connection::is_reader() const
{
    return true;
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function makes a copy of the address of this client connection
 * to the \p address parameter and returns the length.
 *
 * If the function returns zero, then the \p address buffer is not
 * modified and no address is defined in this connection.
 *
 * \param[out] address  The reference to an address variable where the
 *                      client's address gets copied.
 *
 * \return Return the length of the address which may be smaller than
 *         sizeof(address). If zero, then no address is defined.
 *
 * \sa get_addr()
 */
size_t snap_communicator::snap_tcp_server_client_connection::get_client_address(struct sockaddr_storage & address) const
{
    // make sure the address is defined and the socket open
    //
    if(const_cast<snap_communicator::snap_tcp_server_client_connection *>(this)->define_address() != 0)
    {
        return 0;
    }

    address = f_address;
    return f_length;
}


/** \brief Retrieve the address in the form of a string.
 *
 * Like the get_addr() of the tcp client and server classes, this
 * function returns the address in the form of a string which can
 * easily be used to log information and other similar tasks.
 *
 * \return The client's address in the form of a string.
 */
std::string snap_communicator::snap_tcp_server_client_connection::get_client_addr() const
{
    // make sure the address is defined and the socket open
    //
    if(!const_cast<snap_communicator::snap_tcp_server_client_connection *>(this)->define_address())
    {
        return std::string();
    }

    size_t const max_length(std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1);

// in release mode this should not be dynamic (although the syntax is so
// the warning would happen), but in debug it is likely an alloca()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
    char buf[max_length];
#pragma GCC diagnostic pop

    char const * r(nullptr);

    if(f_address.ss_family == AF_INET)
    {
        r = inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in const &>(f_address).sin_addr, buf, max_length);
    }
    else
    {
        r = inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 const &>(f_address).sin6_addr, buf, max_length);
    }

    if(r == nullptr)
    {
        int const e(errno);
        SNAP_LOG_FATAL("inet_ntop() could not convert IP address (errno: ")(e)(" -- ")(strerror(e))(").");
        throw snap_communicator_runtime_error("snap_tcp_server_client_connection::get_addr(): inet_ntop() could not convert IP address properly.");
    }

    return buf;
}


/** \brief Retrieve the port.
 *
 * This function returns the port of the socket on our side.
 *
 * If the port is not available (not connected?), then -1 is returned.
 *
 * \return The client's port in host order.
 */
int snap_communicator::snap_tcp_server_client_connection::get_client_port() const
{
    // make sure the address is defined and the socket open
    //
    if(!const_cast<snap_communicator::snap_tcp_server_client_connection *>(this)->define_address())
    {
        return -1;
    }

    if(f_address.ss_family == AF_INET)
    {
        return ntohs(reinterpret_cast<struct sockaddr_in const &>(f_address).sin_port);
    }
    else
    {
        return ntohs(reinterpret_cast<struct sockaddr_in6 const &>(f_address).sin6_port);
    }
}


/** \brief Retrieve the address in the form of a string.
 *
 * Like the get_addr() of the tcp client and server classes, this
 * function returns the address in the form of a string which can
 * easily be used to log information and other similar tasks.
 *
 * \return The client's address in the form of a string.
 */
std::string snap_communicator::snap_tcp_server_client_connection::get_client_addr_port() const
{
    // get the current address and port
    std::string const addr(get_client_addr());
    int const port(get_client_port());

    // make sure they are defined
    if(addr.empty()
    || port < 0)
    {
        return std::string();
    }

    // calculate the result
    std::stringstream buf;
    if(f_address.ss_family == AF_INET)
    {
        buf << addr << ":" << port;
    }
    else
    {
        buf << "[" << addr << "]:" << port;
    }
    return buf.str();
}


/** \brief Retrieve the socket address if we have not done so yet.
 *
 * This function make sure that the f_address and f_length parameters are
 * defined. This is done by calling the getsockname() function.
 *
 * If f_length is still zero, then it is expected that address was not
 * yet read.
 *
 * Note that the function returns -1 if the socket is now -1 (i.e. the
 * connection is closed) whether or not the function worked before.
 *
 * \return false if the address cannot be defined, true otherwise
 */
bool snap_communicator::snap_tcp_server_client_connection::define_address()
{
    int const s(get_socket());
    if(s == -1)
    {
        return false;
    }

    if(f_length == 0)
    {
        // address not defined yet, retrieve with with getsockname()
        //
        f_length = sizeof(f_address);
        if(getsockname(s, reinterpret_cast<struct sockaddr *>(&f_address), &f_length) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("getsockname() failed retrieving IP address (errno: ")(e)(" -- ")(strerror(e))(").");
            f_length = 0;
            return false;
        }
        if(f_address.ss_family != AF_INET
        && f_address.ss_family != AF_INET6)
        {
            SNAP_LOG_ERROR("address family (")(f_address.ss_family)(") returned by getsockname() is not understood, it is neither an IPv4 nor IPv6.");
            f_length = 0;
            return false;
        }
        if(f_length < sizeof(f_address))
        {
            // reset the rest of the structure, just in case
            //
            memset(reinterpret_cast<char *>(&f_address) + f_length, 0, sizeof(f_address) - f_length);
        }
    }

    return true;
}






////////////////////////////////
// Snap TCP Buffer Connection //
////////////////////////////////

/** \brief Initialize a client socket.
 *
 * The client socket gets initialized with the specified 'socket'
 * parameter.
 *
 * If you are a pure client (opposed to a client that was just accepted)
 * you may want to consider using the snap_tcp_client_buffer_connection
 * instead. That gives you a way to open the socket from a set of address
 * and port definitions among other things.
 *
 * This initialization, so things work as expected in our environment,
 * the function marks the socket as non-blocking. This is important for
 * the reader and writer capabilities.
 *
 * \param[in] client  The client to be used for reading and writing.
 */
snap_communicator::snap_tcp_server_client_buffer_connection::snap_tcp_server_client_buffer_connection(tcp_client_server::bio_client::pointer_t client)
    : snap_tcp_server_client_connection(client)
{
    non_blocking();
}


/** \brief Check whether this connection still has some input in its buffer.
 *
 * This function returns true if there is partial incoming data in this
 * object's buffer.
 *
 * \return true if some buffered input is waiting for completion.
 */
bool snap_communicator::snap_tcp_server_client_buffer_connection::has_input() const
{
    return !f_line.empty();
}



/** \brief Check whether this connection still has some output in its buffer.
 *
 * This function returns true if there is still some output in the client
 * buffer. Output is added by the write() function, which is called by
 * the send_message() function.
 *
 * \return true if some buffered output is waiting to be sent out.
 */
bool snap_communicator::snap_tcp_server_client_buffer_connection::has_output() const
{
    return !f_output.empty();
}



/** \brief Tells that this connection is a writer when we have data to write.
 *
 * This function checks to know whether there is data to be writen to
 * this connection socket. If so then the function returns true. Otherwise
 * it just returns false.
 *
 * This happens whenever you called the write() function and our cache
 * is not empty yet.
 *
 * \return true if there is data to write to the socket, false otherwise.
 */
bool snap_communicator::snap_tcp_server_client_buffer_connection::is_writer() const
{
    return get_socket() != -1 && !f_output.empty();
}


/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchroneous.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_signal() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 */
ssize_t snap_communicator::snap_tcp_server_client_buffer_connection::write(void const * data, size_t const length)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }

    if(data != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(data));
        f_output.insert(f_output.end(), d, d + length);
        return length;
    }

    return 0;
}


/** \brief Read and process as much data as possible.
 *
 * This function reads as much incoming data as possible and processes
 * it.
 *
 * If the input includes a newline character ('\n') then this function
 * calls the process_line() callback which can further process that
 * line of data.
 *
 * \todo
 * Look into a way, if possible, to have a single instantiation since
 * as far as I know this code matches the one written in the
 * process_read() of the snap_tcp_client_buffer_connection and
 * the snap_pipe_buffer_connection classes.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    if(get_socket() != -1)
    {
        int count_lines(0);
        int64_t const date_limit(snap_communicator::get_current_date() + f_processing_time_limit);
        std::vector<char> buffer;
        buffer.resize(1024);
        for(;;)
        {
            errno = 0;
            ssize_t const r(read(&buffer[0], buffer.size()));
            if(r > 0)
            {
                for(ssize_t position(0); position < r; )
                {
                    std::vector<char>::const_iterator it(std::find(buffer.begin() + position, buffer.begin() + r, '\n'));
                    if(it == buffer.begin() + r)
                    {
                        // no newline, just add the whole thing
                        f_line += std::string(&buffer[position], r - position);
                        break; // do not waste time, we know we are done
                    }

                    // retrieve the characters up to the newline
                    // character and process the line
                    //
                    f_line += std::string(&buffer[position], it - buffer.begin() - position);
                    process_line(QString::fromUtf8(f_line.c_str()));
                    ++count_lines;

                    // done with that line
                    //
                    f_line.clear();

                    // we had a newline, we may still have some data
                    // in that buffer; (+1 to skip the '\n' itself)
                    //
                    position = it - buffer.begin() + 1;
                }

                // when we reach here all the data read in `buffer` is
                // now either fully processed or in f_line
                //
                // TODO: change the way this works so we can test the
                //       limit after each process_line() call
                //
                if(count_lines >= f_event_limit
                || snap_communicator::get_current_date() >= date_limit)
                {
                    // we reach one or both limits, stop processing so
                    // the other events have a chance to run
                    //
                    break;
                }
            }
            else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                break;
            }
            else //if(r < 0)
            {
                int const e(errno);
                SNAP_LOG_WARNING("an error occurred while reading from socket (errno: ")(e)(" -- ")(strerror(e))(").");
                process_error();
                return;
            }
        }
    }

    // process next level too
    snap_tcp_server_client_connection::process_read();
}


/** \brief Write to the connection's socket.
 *
 * This function implementation writes as much data as possible to the
 * connection's socket.
 *
 * This function calls the process_empty_buffer() callback whenever the
 * output buffer goes empty.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_write()
{
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(snap_tcp_server_client_connection::write(&f_output[f_position], f_output.size() - f_position));
        if(r > 0)
        {
            // some data was written
            f_position += r;
            if(f_position >= f_output.size())
            {
                f_output.clear();
                f_position = 0;
                process_empty_buffer();
            }
        }
        else if(r != 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, get rid of it
            //
            int const e(errno);
            SNAP_LOG_ERROR("an error occurred while writing to socket of \"")(f_name)("\" (errno: ")(e)(" -- ")(strerror(e))(").");
            process_error();
            return;
        }
    }

    // process next level too
    snap_tcp_server_client_connection::process_write();
}


/** \brief The remote used hanged up.
 *
 * This function makes sure that the connection gets closed properly.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    snap_tcp_server_client_connection::process_hup();
}







////////////////////////////////////////
// Snap TCP Server Message Connection //
////////////////////////////////////////

/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \param[in] client  The client representing the in/out socket.
 */
snap_communicator::snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection(tcp_client_server::bio_client::pointer_t client)
    : snap_tcp_server_client_buffer_connection(client)
{
    // TODO: somehow the port seems wrong (i.e. all connections return the same port)

    // make sure the socket is defined and well
    //
    int const socket(client->get_socket());
    if(socket < 0)
    {
        SNAP_LOG_ERROR("called with a closed client connection.");
        throw std::runtime_error("snap_communicator::snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection() called with a closed client connection.");
    }

    struct sockaddr_storage address = sockaddr_storage();
    socklen_t length(sizeof(address));
    if(getpeername(socket, reinterpret_cast<struct sockaddr *>(&address), &length) != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR("getpeername() failed retrieving IP address (errno: ")(e)(" -- ")(strerror(e))(").");
        throw std::runtime_error("getpeername() failed to retrieve IP address in snap_communicator::snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection()");
    }
    if(address.ss_family != AF_INET
    && address.ss_family != AF_INET6)
    {
        SNAP_LOG_ERROR("address family (")(address.ss_family)(") returned by getpeername() is not understood, it is neither an IPv4 nor IPv6.");
        throw std::runtime_error("getpeername() returned an address which is not understood in snap_communicator::snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection()");
    }
    if(length < sizeof(address))
    {
        // reset the rest of the structure, just in case
        memset(reinterpret_cast<char *>(&address) + length, 0, sizeof(address) - length);
    }

    constexpr size_t max_length(std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1);

// in release mode this should not be dynamic (although the syntax is so
// the warning would happen), but in debug it is likely an alloca()
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
    char buf[max_length];
#pragma GCC diagnostic pop

    char const * r(nullptr);

    if(address.ss_family == AF_INET)
    {
        r = inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in const &>(address).sin_addr, buf, max_length);
    }
    else
    {
        r = inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 const &>(address).sin6_addr, buf, max_length);
    }

    if(r == nullptr)
    {
        int const e(errno);
        SNAP_LOG_FATAL("inet_ntop() could not convert IP address (errno: ")(e)(" -- ")(strerror(e))(").");
        throw snap_communicator_runtime_error("snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection(): inet_ntop() could not convert IP address properly.");
    }

    if(address.ss_family == AF_INET)
    {
        f_remote_address = QString("%1:%2").arg(buf).arg(ntohs(reinterpret_cast<struct sockaddr_in const &>(address).sin_port));
    }
    else
    {
        f_remote_address = QString("[%1]:%2").arg(buf).arg(ntohs(reinterpret_cast<struct sockaddr_in6 const &>(address).sin6_port));
    }
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message (snap_communicator_message)
 * and then calls the process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void snap_communicator::snap_tcp_server_client_message_connection::process_line(QString const & line)
{
    // empty lines should not occur, but just in case, just ignore
    if(line.isEmpty())
    {
        return;
    }

    snap_communicator_message message;
    if(message.from_message(line))
    {
        dispatch_message(message);
    }
    else
    {
        // TODO: what to do here? This could because the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR("process_line() was asked to process an invalid message (")(line)(")");
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \exception snap_communicator_runtime_error
 * This function throws this exception if the write() to the pipe
 * fails to write the entire message. This should only happen if
 * the pipe gets severed.
 *
 * \param[in] message  The message to be processed.
 * \param[in] cache  Whether to cache the message if there is no connection.
 *                   (Ignore because a client socket has to be there until
 *                   closed and then it can't be reopened by the server.)
 *
 * \return Always true, although if an error occurs the function throws.
 */
bool snap_communicator::snap_tcp_server_client_message_connection::send_message(snap_communicator_message const & message, bool cache)
{
    NOTUSED(cache);

    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    QString const msg(message.to_message());
    QByteArray const utf8(msg.toUtf8());
    std::string buf(utf8.data(), utf8.size());
    buf += "\n";
    return write(buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
}


/** \brief Retrieve the remote address information.
 *
 * This function can be used to retrieve the remove address and port
 * information as was specified on the constructor. These can be used
 * to find this specific connection at a later time or create another
 * connection.
 *
 * For example, you may get 192.168.2.17:4040.
 *
 * The function works even after the socket gets closed as we save
 * the remote address and port in a string just after the connection
 * was established.
 *
 * \warning
 * This function returns BOTH: the address and the port.
 *
 * \note
 * These parameters are the same as what was passed to the constructor,
 * only both will have been converted to numbers. So for example when
 * you used "localhost", here you get "::1" or "127.0.0.1" for the
 * address.
 *
 * \return The remote host address and connection port.
 */
QString const & snap_communicator::snap_tcp_server_client_message_connection::get_remote_address() const
{
    return f_remote_address;
}




//////////////////////////////////////////////////
// Snap TCP Client Permanent Message Connection //
//////////////////////////////////////////////////

/** \brief Internal implementation of the snap_tcp_client_permanent_message_connection class.
 *
 * This class is used to handle a thread that will process a connection for
 * us. This allows us to connect in any amount of time required by the
 * Unix system to obtain the connection with the remote server.
 *
 * \todo
 * Having threads at the time we do a fork() is not safe. We may
 * want to reconsider offering this functionality here. Because at
 * this time we would have no control of when the thread is created
 * and thus a way to make sure that no such thread is running when
 * we call fork().
 */
class snap_tcp_client_permanent_message_connection_impl
{
public:
    class messenger
        : public snap_communicator::snap_tcp_server_client_message_connection
    {
    public:
        typedef std::shared_ptr<messenger>      pointer_t;

        messenger(snap_communicator::snap_tcp_client_permanent_message_connection * parent, tcp_client_server::bio_client::pointer_t client)
            : snap_tcp_server_client_message_connection(client)
            , f_parent(parent)
        {
            set_name("snap_tcp_client_permanent_message_connection_impl::messenger");
        }

        messenger(messenger const & rhs) = delete;
        messenger & operator = (messenger const & rhs) = delete;

        // snap_connection implementation
        virtual void process_empty_buffer()
        {
            snap_tcp_server_client_message_connection::process_empty_buffer();
            f_parent->process_empty_buffer();
        }

        // snap_connection implementation
        virtual void process_error()
        {
            snap_tcp_server_client_message_connection::process_error();
            f_parent->process_error();
        }

        // snap_connection implementation
        virtual void process_hup()
        {
            snap_tcp_server_client_message_connection::process_hup();
            f_parent->process_hup();
        }

        // snap_connection implementation
        virtual void process_invalid()
        {
            snap_tcp_server_client_message_connection::process_invalid();
            f_parent->process_invalid();
        }

        // snap_tcp_server_client_message_connection implementation
        virtual void process_message(snap_communicator_message const & message)
        {
            // We call the dispatcher from our parent since the child
            // (this messenger) is not given a dispatcher
            //
            snap_communicator_message copy(message);
            f_parent->dispatch_message(copy);
        }

    private:
        snap_communicator::snap_tcp_client_permanent_message_connection *  f_parent = nullptr;
    };

    class thread_done_signal
        : public snap_communicator::snap_thread_done_signal
    {
    public:
        typedef std::shared_ptr<thread_done_signal>   pointer_t;

        thread_done_signal(snap_tcp_client_permanent_message_connection_impl * parent_impl)
            : f_parent_impl(parent_impl)
        {
            set_name("snap_tcp_client_permanent_message_connection_impl::thread_done_signal");
        }

        thread_done_signal(thread_done_signal const & rhs) = delete;
        thread_done_signal & operator = (thread_done_signal const & rhs) = delete;

        /** \brief This signal was emitted.
         *
         * This function gets called whenever the thread is just about to
         * quit. Calling f_thread.is_running() may still return true when
         * you get in the 'thread_done()' callback. However, an
         * f_thread.stop() will return very quickly.
         */
        virtual void process_read()
        {
            snap_thread_done_signal::process_read();

            f_parent_impl->thread_done();
        }

    private:
        snap_tcp_client_permanent_message_connection_impl *  f_parent_impl = nullptr;
    };

    class runner
        : public snap_thread::snap_runner
    {
    public:
        runner(snap_tcp_client_permanent_message_connection_impl * parent_impl, std::string const & address, int port, tcp_client_server::bio_client::mode_t mode)
            : snap_runner("background snap_tcp_client_permanent_message_connection for asynchroneous connections")
            , f_parent_impl(parent_impl)
            , f_address(address)
            , f_port(port)
            , f_mode(mode)
        {
        }


        runner(runner const & rhs) = delete;
        runner & operator = (runner const & rhs) = delete;


        /** \brief This is the actual function run by the thread.
         *
         * This function calls the connect() function and then
         * tells the main thread we are done.
         */
        virtual void run()
        {
            connect();

            // tell the main thread that we are done
            //
            f_parent_impl->trigger_thread_done();
        }


        /** \brief This function attempts to connect.
         *
         * This function attempts a connection to the specified address
         * and port with the specified mode (i.e. plain or encrypted.)
         *
         * The function may take a long time to succeed connecting with
         * the server. The main thread will be awaken whenever this
         * thread dies.
         *
         * If an error occurs, then the f_socket variable member will
         * be set to -1. Otherwise it represents the socket that we
         * just connected with.
         */
        void connect()
        {
            try
            {
                // create a socket using the bio_client class,
                // but then just create a duplicate that we will
                // use in a server-client TCP object (because
                // we cannot directly create the right type of
                // object otherwise...)
                //
                f_tcp_connection.reset(new tcp_client_server::bio_client(f_address, f_port, f_mode));
            }
            catch(tcp_client_server::tcp_client_server_runtime_error const & e)
            {
                // connection failed... we will have to try again later
                //
                // WARNING: our logger is not multi-thread safe
                //SNAP_LOG_ERROR("connection to ")(f_address)(":")(f_port)(" failed with: ")(e.what());
                f_last_error = e.what();
                f_tcp_connection.reset();
            }
        }


        /** \brief Retrieve the address to connect to.
         *
         * This function returns the address passed in on creation.
         *
         * \note
         * Since the variable is constant, it is likely to never change.
         * However, the c_str() function may change the buffer pointer.
         * Hence, to be 100% safe, you cannot call this function until
         * you make sure that the thread is fully stopped.
         */
        std::string const & get_address() const
        {
            return f_address;
        }


        /** \brief Retrieve the port to connect to.
         *
         * This function returns the port passed in on creation.
         *
         * \note
         * Since the variable is constant, it never gets changed
         * which means it is always safe to use it between
         * both threads.
         */
        int get_port() const
        {
            return f_port;
        }


        /** \brief Retrieve the client allocated and connected by the thread.
         *
         * This function returns the TCP connection object resulting from
         * connection attempts of the background thread.
         *
         * If the pointer is null, then you may get the corresponding
         * error message using the get_last_error() function.
         *
         * You can get the client TCP connection pointer once. After that
         * you always get a null pointer.
         *
         * \note
         * This function is guarded so the pointer and the object it
         * points to will be valid in another thread that retrieves it.
         *
         * \return The connection pointer.
         */
        tcp_client_server::bio_client::pointer_t release_client()
        {
            snap_thread::snap_lock lock(f_mutex);
            tcp_client_server::bio_client::pointer_t tcp_connection;
            tcp_connection.swap(f_tcp_connection);
            return tcp_connection;
        }


        /** \brief Retrieve the last error message that happened.
         *
         * This function returns the last error message that was captured
         * when trying to connect to the socket. The message is the
         * e.what() message from the exception we captured.
         *
         * The message does not get cleared so the function can be called
         * any number of times. To know whether an error was generated
         * on the last attempt, make sure to first get the get_socket()
         * and if it returns -1, then this message is significant,
         * otherwise it is from a previous error.
         *
         * \warning
         * Remember that if the background thread was used the error will
         * NOT be available in the main thread until a full memory barrier
         * was executed. For that reason we make sure that the thread
         * was stopped when we detect an error.
         *
         * \return The last error message.
         */
        std::string const & get_last_error() const
        {
            return f_last_error;
        }


        /** \brief Close the connection.
         *
         * This function closes the connection. Since the f_tcp_connection
         * holds the socket to the remote server, we have get this function
         * called in order to completely disconnect.
         *
         * \note
         * This function does not clear the f_last_error parameter so it
         * can be read later.
         */
        void close()
        {
            f_tcp_connection.reset();
        }


    private:
        snap_tcp_client_permanent_message_connection_impl * f_parent_impl = nullptr;
        std::string const                                   f_address;
        int const                                           f_port;
        tcp_client_server::bio_client::mode_t const         f_mode;
        tcp_client_server::bio_client::pointer_t            f_tcp_connection = tcp_client_server::bio_client::pointer_t();
        std::string                                         f_last_error = std::string();
    };


    /** \brief Initialize a permanent message connection implementation object.
     *
     * This object manages the thread used to asynchronically connect to
     * the specified address and port.
     *
     * This class and its sub-classes may end up executing callbacks
     * of the snap_tcp_client_permanent_message_connection object.
     * However, in all cases these are never run from the thread.
     *
     * \param[in] client  A pointer to the owner of this
     *            snap_tcp_client_permanent_message_connection_impl object.
     * \param[in] address  The address we are to connect to.
     * \param[in] port  The port we are to connect to.
     * \param[in] mode  The mode used to connect.
     */
    snap_tcp_client_permanent_message_connection_impl(snap_communicator::snap_tcp_client_permanent_message_connection * parent, std::string const & address, int port, tcp_client_server::bio_client::mode_t mode)
        : f_parent(parent)
        //, f_thread_done() -- auto-init
        , f_thread_runner(this, address, port, mode)
        , f_thread("background connection handler thread", &f_thread_runner)
        //, f_messenger(nullptr) -- auto-init
        //, f_message_cache() -- auto-init
    {
    }


    snap_tcp_client_permanent_message_connection_impl(snap_tcp_client_permanent_message_connection_impl const & rhs) = delete;
    snap_tcp_client_permanent_message_connection_impl & operator = (snap_tcp_client_permanent_message_connection_impl const & rhs) = delete;

    /** \brief Destroy the permanent message connection.
     *
     * This function makes sure that the messenger was lost.
     */
    ~snap_tcp_client_permanent_message_connection_impl()
    {
        // to make sure we can lose the messenger, first we want to be sure
        // that we do not have a thread running
        //
        try
        {
            f_thread.stop();
        }
        catch(snap_thread_exception_mutex_failed_error const &)
        {
        }
        catch(snap_thread_exception_invalid_error const &)
        {
        }

        // in this case we may still have an instance of the f_thread_done
        // which linger around, we want it out
        //
        // Note: the call is safe even if the f_thread_done is null
        //
        snap_communicator::instance()->remove_connection(f_thread_done);

        // although the f_messenger variable gets reset automatically in
        // the destructor, it would not get removed from the snap
        // communicator instance if we were not doing it explicitly
        //
        disconnect();
    }


    /** \brief Direct connect to the messenger.
     *
     * In this case we try to connect without the thread. This allows
     * us to avoid the thread problems, but we are blocked until the
     * OS decides to time out or the connection worked.
     */
    void connect()
    {
        if(f_done)
        {
            SNAP_LOG_ERROR("Permanent connection marked done. Cannot attempt to reconnect.");
            return;
        }

        // call the thread connect() function from the main thread
        //
        f_thread_runner.connect();

        // simulate receiving the thread_done() signal
        //
        thread_done();
    }


    /** \brief Check whether the permanent connection is currently connected.
     *
     * This function returns true if the messenger exists, which means that
     * the connection is up.
     *
     * \return true if the connection is up.
     */
    bool is_connected()
    {
        return f_messenger != nullptr;
    }


    /** \brief Try to start the thread runner.
     *
     * This function tries to start the thread runner in order to initiate
     * a connection in the background. If the thread could not be started,
     * then the function returns false.
     *
     * If the thread started, then the function returns true. This does
     * not mean that the connection was obtained. This is known once
     * the process_connected() function is called.
     *
     * \return true if the thread was successfully started.
     */
    bool background_connect()
    {
        if(f_done)
        {
            SNAP_LOG_ERROR("Permanent connection marked done. Cannot attempt to reconnect.");
            return false;
        }

        if(f_thread.is_running())
        {
            SNAP_LOG_ERROR("A background connection attempt is already in progress. Further requests are ignored.");
            return false;
        }

        // create the f_thread_done only when required
        //
        if(f_thread_done == nullptr)
        {
            f_thread_done.reset(new thread_done_signal(this));
        }

        snap_communicator::instance()->add_connection(f_thread_done);

        if(!f_thread.start())
        {
            SNAP_LOG_ERROR("The thread used to run the background connection process did not start.");
            return false;
        }

        return true;
    }


    /** \brief Tell the main thread that the background thread is done.
     *
     * This function is called by the thread so the thread_done()
     * function of the thread done object gets called. Only the
     * thread should call this function.
     *
     * As a result the thread_done() function of this class will be
     * called by the main thread.
     */
    void trigger_thread_done()
    {
        f_thread_done->thread_done();
    }


    /** \brief Signal that the background thread is done.
     *
     * This callback is called whenever the background thread sends
     * a signal to us. This is used to avoid calling end user functions
     * that would certainly cause a lot of problem if called from the
     * thread.
     *
     * The function calls the process_connection_failed() if the
     * connection did not happen.
     *
     * The function calls the process_connected() if the connection
     * did happen.
     *
     * \note
     * This is used only if the user requested that the connection
     * happen in the background (i.e. use_thread was set to true
     * in the snap_tcp_client_permanent_message_connection object
     * constructor.)
     */
    void thread_done()
    {
        // if we used the thread we have to remove the signal used
        // to know that the thread was done
        //
        snap_communicator::instance()->remove_connection(f_thread_done);

        // we will access the f_last_error member of the thread runner
        // which may not be available to the main thread yet, calling
        // stop forces a memory barrier so we are all good.
        //
        // calling stop() has no effect if we did not use the thread,
        // however, not calling stop() when we did use the thread
        // causes all sorts of other problems (especially, the thread
        // never gets joined)
        //
        f_thread.stop();

        tcp_client_server::bio_client::pointer_t client(f_thread_runner.release_client());
        if(f_done)
        {
            // already marked done, ignore the result and lose the
            // connection immediately
            //
            //f_thread_running.close(); -- not necessary, 'client' is the connection
            return;
        }

        if(client == nullptr)
        {
            // TODO: fix address in error message using a snap::addr so
            //       as to handle IPv6 seemlessly.
            //
            SNAP_LOG_ERROR("connection to ")
                          (f_thread_runner.get_address())
                          (":")
                          (f_thread_runner.get_port())
                          (" failed with: ")
                          (f_thread_runner.get_last_error());

            // signal that an error occurred
            //
            f_parent->process_connection_failed(f_thread_runner.get_last_error());
        }
        else
        {
            f_messenger.reset(new messenger(f_parent, client));

            // add the messenger to the communicator
            //
            snap_communicator::instance()->add_connection(f_messenger);

            // if some messages were cached, process them immediately
            //
            while(!f_message_cache.empty())
            {
                f_messenger->send_message(f_message_cache[0]);
                f_message_cache.erase(f_message_cache.begin());
            }

            // let the client know we are now connected
            //
            f_parent->process_connected();
        }
    }

    /** \brief Send a message to the connection.
     *
     * This implementation function actually sends the message to the
     * connection, assuming that the connection exists. Otherwise, it
     * may cache the message (if cache is true.)
     *
     * Note that the message does not get cached if mark_done() was
     * called earlier since we are trying to close the whole connection.
     *
     * \param[in] message  The message to send.
     * \param[in] cache  Whether to cache the message if the connection is
     *                   currently down.
     *
     * \return true if the message was forwarded, false if the message
     *         was ignored or cached.
     */
    bool send_message(snap_communicator_message const & message, bool cache)
    {
        if(f_messenger != nullptr)
        {
            return f_messenger->send_message(message);
        }

        if(cache && !f_done)
        {
            f_message_cache.push_back(message);
        }

        return false;
    }


    /** \brief Forget about the messenger connection.
     *
     * This function is used to fully disconnect from the messenger.
     *
     * If there is a messenger, this means:
     *
     * \li Removing the messenger from the snap_communicator instance.
     * \li Closing the connection in the thread object.
     *
     * In most cases, it is called when an error occur, also it happens
     * that we call it explicitly through the disconnect() function
     * of the permanent connection class.
     *
     * \note
     * This is safe, even though it is called from the messenger itself
     * because it will not get deleted yet. This is because the run()
     * loop has a copy in its own temporary copy of the vector of
     * connections.
     */
    void disconnect()
    {
        if(f_messenger != nullptr)
        {
            snap_communicator::instance()->remove_connection(f_messenger);
            f_messenger.reset();

            // just the messenger does not close the TCP connection because
            // we may have another in the thread runner
            //
            f_thread_runner.close();
        }
    }


    /** \brief Return the address and size of the remote computer.
     *
     * This function retrieve the socket address.
     *
     * \param[out] address  The binary address of the remote computer.
     *
     * \return The size of the sockaddr structure, 0 if no address is available.
     */
    size_t get_client_address(struct sockaddr_storage & address) const
    {
        if(f_messenger != nullptr)
        {
            return f_messenger->get_client_address(address);
        }
        memset(&address, 0, sizeof(address));
        return 0;
    }


    /** \brief Return the address of the f_message object.
     *
     * This function returns the address of the message object.
     *
     * \return The address of the remote computer.
     */
    std::string get_client_addr() const
    {
        if(f_messenger != nullptr)
        {
            return f_messenger->get_client_addr();
        }
        return std::string();
    }


    /** \brief Mark the messenger as done.
     *
     * This function is used to mark the messenger as done. This means it
     * will get removed from the snap_communicator instance as soon as it
     * is done with its current write buffer if there is one.
     *
     * You may also want to call the disconnection() function to actually
     * reset the pointer along the way.
     */
    void mark_done()
    {
        f_done = true;

        // once done we don't attempt to reconnect so we can as well
        // get rid of our existing cache immediately to save some
        // memory
        //
        f_message_cache.clear();

        if(f_messenger != nullptr)
        {
            f_messenger->mark_done();
        }
    }


private:
    snap_communicator::snap_tcp_client_permanent_message_connection *   f_parent = nullptr;
    thread_done_signal::pointer_t                                       f_thread_done = thread_done_signal::pointer_t();
    runner                                                              f_thread_runner;
    snap::snap_thread                                                   f_thread;
    messenger::pointer_t                                                f_messenger = messenger::pointer_t();
    snap_communicator_message::vector_t                                 f_message_cache = snap_communicator_message::vector_t();
    bool                                                                f_done = false;
};


/** \brief Initializes this TCP client message connection.
 *
 * This implementation creates what we call a permanent connection.
 * Such a connection may fail once in a while. In such circumstances,
 * the class automatically requests for a reconnection (see various
 * parameters in the regard below.) However, this causes one issue:
 * by default, the connection just never ends. When you are about
 * ready to close the connection, you must call the mark_done()
 * function first. This will tell the various error functions to
 * drop this connection instead of restarting it after a small pause.
 *
 * This constructor makes sure to initialize the timer and saves
 * the address, port, mode, pause, and use_thread parameters.
 *
 * The timer is first set to trigger immediately. This means the TCP
 * connection will be attempted as soon as possible (the next time
 * the run() loop is entered, it will time out immediately.) You
 * are free to call set_timeout_date() with a date in the future if
 * you prefer that the connect be attempted a little later.
 *
 * The \p pause parameter is used if the connection is lost and this
 * timer is used again to attempt a new connection. It will be reused
 * as long as the connection fails (as a delay). It has to be at least
 * 10 microseconds, although really you should not use less than 1
 * second (1000000). You may set the pause parameter to 0 in which case
 * you are responsible to set the delay (by default there will be no
 * delay and thus the timer will never time out.)
 *
 * To start with a delay, instead of trying to connect immediately,
 * you may pass a negative pause parameter. So for example to get the
 * first attempt 5 seconds after you created this object, you use
 * -5000000LL as the pause parameter.
 *
 * The \p use_thread parameter determines whether the connection should
 * be attempted in a thread (asynchroneously) or immediately (which means
 * the timeout callback may block for a while.) If the connection is to
 * a local server with an IP address specified as numbers (i.e. 127.0.0.1),
 * the thread is probably not required. For connections to a remote
 * computer, though, it certainly is important.
 *
 * \param[in] address  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 * \param[in] mode  The mode to use to open the connection.
 * \param[in] pause  The amount of time to wait before attempting a new
 *                   connection after a failure, in microseconds, or 0.
 * \param[in] use_thread  Whether a thread is used to connect to the
 *                        server.
 */
snap_communicator::snap_tcp_client_permanent_message_connection::snap_tcp_client_permanent_message_connection
    (   std::string const & address
      , int port
      , tcp_client_server::bio_client::mode_t mode
      , int64_t const pause
      , bool const use_thread
    )
    : snap_timer(pause < 0 ? -pause : 0)
    , f_impl(new snap_tcp_client_permanent_message_connection_impl(this, address, port, mode))
    , f_pause(llabs(pause))
    , f_use_thread(use_thread)
{
}


/** \brief Destroy instance
 */
snap_communicator::snap_tcp_client_permanent_message_connection::~snap_tcp_client_permanent_message_connection()
{
    // Does nothing
}


/** \brief Attempt to send a message to this connection.
 *
 * If the connection is currently enabled, the message is sent immediately.
 * Otherwise, it may be cached if the \p cache parameter is set to true.
 * A cached message is forwarded as soon as a new successful connection
 * happens, which can be a problem if messages need to happen in a very
 * specific order (For example, after a reconnection to snapcommunicator
 * you first need to REGISTER or CONNECT...)
 *
 * \param[in] message  The message to send to the connected server.
 * \param[in] cache  Whether the message should be cached.
 *
 * \return true if the message was sent, false if it was not sent, although
 *         if cache was true, it was cached
 */
bool snap_communicator::snap_tcp_client_permanent_message_connection::send_message(snap_communicator_message const & message, bool cache)
{
    return f_impl->send_message(message, cache);
}


/** \brief Check whether the connection is up.
 *
 * This function returns true if the connection is considered to be up.
 * This means sending messages will work quickly instead of being
 * cached up until an actual TCP/IP connection gets established.
 *
 * Note that the connection may have hanged up since, and the system
 * may not have yet detected the fact (i.e. the connection is going
 * to receive the process_hup() call after the event in which you are
 * working.)
 *
 * \return true if connected
 */
bool snap_communicator::snap_tcp_client_permanent_message_connection::is_connected() const
{
    return f_impl->is_connected();
}


/** \brief Disconnect the messenger now.
 *
 * This function kills the current connection.
 *
 * There are a few cases where two daemons communicate between each others
 * and at some point one of them wants to exit and needs to disconnect. This
 * function can be used in that one situation assuming that you have an
 * acknowledgement from the other daemon.
 *
 * Say you have daemon A and B. B wants to quit and before doing so sends
 * a form of "I'm quitting" message to A. In that situation, B is not closing
 * the messenger connection, A is responsible for that (i.e. A acknowledges
 * receipt of the "I'm quitting" message from B by closing the connection.)
 *
 * B also wants to call the mark_done() function to make sure that it
 * does not reconnected a split second later and instead the permanent
 * connection gets removed from the snap_communicator list of connections.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::disconnect()
{
    f_impl->disconnect();
}


/** \brief Overload so we do not have to use namespace everywhere.
 *
 * This function overloads the snap_connection::mark_done() function so
 * we can call it without the need to use snap_timer::mark_done()
 * everywhere.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::mark_done()
{
    snap_timer::mark_done();
}


/** \brief Mark connection as done.
 *
 * This function allows you to mark the permanent connection and the
 * messenger as done.
 *
 * Note that calling this function with false is the same as calling the
 * base class mark_done() function.
 *
 * If the \p message parameter is set to true, we suggest you also call
 * the disconnect() function. That way the messenger will truly get
 * removed from everyone quickly.
 *
 * \param[in] messenger  If true, also mark the messenger as done.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::mark_done(bool messenger)
{
    snap_timer::mark_done();
    if(messenger)
    {
        f_impl->mark_done();
    }
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function makes a copy of the address of this client connection
 * to the \p address parameter and returns the length.
 *
 * \param[in] address  The reference to an address variable where the
 *                     address gets copied.
 *
 * \return Return the length of the address which may be smaller than
 *         sizeof(struct sockaddr). If zero, then no address is defined.
 *
 * \sa get_addr()
 */
size_t snap_communicator::snap_tcp_client_permanent_message_connection::get_client_address(struct sockaddr_storage & address) const
{
    return f_impl->get_client_address(address);
}


/** \brief Retrieve the remote computer address as a string.
 *
 * This function returns the address of the remote computer as a string.
 * It will be a canonicalized IP address.
 *
 * \return The cacnonicalized IP address.
 */
std::string snap_communicator::snap_tcp_client_permanent_message_connection::get_client_addr() const
{
    return f_impl->get_client_addr();
}


/** \brief Internal timeout callback implementation.
 *
 * This callback implements the guts of this class: it attempts to connect
 * to the specified address and port, optionally after creating a thread
 * so the attempt can happen asynchroneously.
 *
 * When the connection fails, the timer is used to try again pause
 * microseconds later (pause as specified in the constructor).
 *
 * When a connection succeeds, the timer is disabled until you detect
 * an error while using the connection and re-enable the timer.
 *
 * \warning
 * This function changes the timeout delay to the pause amount
 * as defined with the constructor. If you want to change that
 * amount, you can do so an any point after this function call
 * using the set_timeout_delay() function. If the pause parameter
 * was set to -1, then the timeout never gets changed.
 * However, you should not use a permanent message timer as your
 * own or you will interfer with the internal use of the timer.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_timeout()
{
    // got a spurious call when already marked done
    //
    if(is_done())
    {
        return;
    }

    // change the timeout delay although we will not use it immediately
    // if we start the thread or attempt an immediate connection, but
    // that way the user can change it by calling set_timeout_delay()
    // at any time after the first process_timeout() call
    //
    if(f_pause > 0)
    {
        set_timeout_delay(f_pause);
        f_pause = 0;
    }

    if(f_use_thread)
    {
        // in this case we create a thread, run it and know whether the
        // connection succeeded only when the thread tells us it did
        //
        // TODO: the background_connect() may return false in two situations:
        //       1) when the thread is already running and then the behavior
        //          we have below is INCORRECT
        //       2) when the thread cannot be started (i.e. could not
        //          allocate the stack?) in which case the if() below
        //          is the correct behavior
        //
        if(f_impl->background_connect())
        {
            // we started the thread successfully, so block the timer
            //
            set_enable(false);
        }
    }
    else
    {
        // the success is noted when we receive a call to
        // process_connected(); there we do set_enable(false)
        // so the timer stops
        //
        f_impl->connect();
    }
}


/** \brief Process an error.
 *
 * When an error occurs, we restart the timer so we can attempt to reconnect
 * to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the snap_timer::process_error() function
 * which means that this connection is not automatically removed from
 * the snapcommunicator object on failures.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_error()
{
    if(is_done())
    {
        snap_timer::process_error();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Process a hang up.
 *
 * When a hang up occurs, we restart the timer so we can attempt to reconnect
 * to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the snap_timer::process_hup() function
 * which means that this connection is not automatically removed from
 * the snapcommunicator object on failures.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_hup()
{
    if(is_done())
    {
        snap_timer::process_hup();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Process an invalid signal.
 *
 * When an invalid signal occurs, we restart the timer so we can attempt
 * to reconnect to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the snap_timer::process_invalid() function
 * which means that this connection is not automatically removed from
 * the snapcommunicator object on failures.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_invalid()
{
    if(is_done())
    {
        snap_timer::process_invalid();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Make sure that the messenger connection gets removed.
 *
 * This function makes sure that the messenger sub-connection also gets
 * removed from the snap communicator. Otherwise it would lock the system
 * since connections are saved in the snap communicator object as shared
 * pointers.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::connection_removed()
{
    f_impl->disconnect();
}


/** \brief Process a connection failed callback.
 *
 * When a connection attempt fails, we restart the timer so we can
 * attempt to reconnect to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \param[in] error_message  The error message that trigged this callback.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_connection_failed(std::string const & error_message)
{
    NOTUSED(error_message);
    set_enable(true);
}


/** \brief The connection is ready.
 *
 * This callback gets called whenever the connection succeeded and is
 * ready to be used.
 *
 * You should implement this virtual function if you have to initiate
 * the communication. For example, the snapserver has to send a
 * REGISTER to the snapcommunicator system and thus implements this
 * function.
 *
 * The default implementation makes sure that the timer gets turned off
 * so we do not try to reconnect every minute or so.
 */
void snap_communicator::snap_tcp_client_permanent_message_connection::process_connected()
{
    set_enable(false);
}




////////////////////////////////
// Snap UDP Server Connection //
////////////////////////////////


/** \brief Initialize a UDP listener.
 *
 * This function is used to initialize a server connection, a UDP/IP
 * listener which wakes up whenever a send() is sent to this listener
 * address and port.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 */
snap_communicator::snap_udp_server_connection::snap_udp_server_connection(std::string const & addr, int port)
    : udp_server(addr, port)
{
}


/** \brief Check to know whether this UDP connection is a reader.
 *
 * This function returns true to say that this UDP connection is
 * indeed a reader.
 *
 * \return This function already returns true as we are likely to
 *         always want a UDP socket to be listening for incoming
 *         packets.
 */
bool snap_communicator::snap_udp_server_connection::is_reader() const
{
    return true;
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the udp_server class.
 *
 * \return The socket of this client connection.
 */
int snap_communicator::snap_udp_server_connection::get_socket() const
{
    return udp_server::get_socket();
}


/** \brief Define a secret code.
 *
 * When receiving a message through this UDP socket, this secret code must
 * be included in the message. If not present, then the message gets
 * discarded.
 *
 * By default this parameter is an empty string. This means no secret
 * code is required and UDP communication can be done without it.
 *
 * \note
 * Secret codes are expected to be used only on connections between
 * computers. If the IP address is 127.0.0.1, you probably don't need
 * to have a secret code.
 *
 * \warning
 * Remember that UDP messages are limited in size. If too long, the
 * send_message() function throws an error. So your secret code should
 * remain relatively small.
 *
 * \todo
 * The secret_code string must be a valid UTF-8 string. At this point
 * this is not enforced.
 *
 * \param[in] secret_code  The secret code that has to be included in the
 * incoming messages for those to be accepted.
 */
void snap_communicator::snap_udp_server_connection::set_secret_code(std::string const & secret_code)
{
    f_secret_code = secret_code;
}


/** \brief Retrieve the server secret code.
 *
 * This function returns the server secret code as defined with the
 * set_secret_code() function. By default this parameter is set to
 * the empty string.
 *
 * Whenever a message is received, this code is checked. If defined
 * in the server and not equal to the code in the message, then the
 * message is discarded (hackers?)
 *
 * The message is also used when sending a message. It gets added
 * to the message if it is not the empty string.
 *
 * \return The secret code.
 */
std::string const & snap_communicator::snap_udp_server_connection::get_secret_code() const
{
    return f_secret_code;
}




////////////////////////////////
// Snap UDP Server Connection //
////////////////////////////////


/** \brief Initialize a UDP server to send and receive messages.
 *
 * This function initialises a UDP server as a Snap UDP server
 * connection attached to the specified address and port.
 *
 * It is expected to be used to send and receive UDP messages.
 *
 * Note that to send messages, you need the address and port
 * of the destination. In effect, we do not use this server
 * when sending. Instead we create a client that we immediately
 * destruct once the message was sent.
 *
 * \param[in] addr  The address to listen on.
 * \param[in] port  The port to listen on.
 */
snap_communicator::snap_udp_server_message_connection::snap_udp_server_message_connection(std::string const & addr, int port)
    : snap_udp_server_connection(addr, port)
{
    // allow for looping over all the messages in one go
    //
    non_blocking();
}


/** \brief Send a UDP message.
 *
 * This function offers you to send a UDP message to the specified
 * address and port. The message should be small enough to fit in
 * on UDP packet or the call will fail.
 *
 * \note
 * The function return true when the message was successfully sent.
 * This does not mean it was received.
 *
 * \param[in] addr  The destination address for the message.
 * \param[in] port  The destination port for the message.
 * \param[in] message  The message to send to the destination.
 * \param[in] secret_code  The secret code to send along the message.
 *
 * \return true when the message was sent, false otherwise.
 */
bool snap_communicator::snap_udp_server_message_connection::send_message(
                  std::string const & addr
                , int port
                , snap_communicator_message const & message
                , std::string const & secret_code)
{
    // Note: contrary to the TCP version, a UDP message does not
    //       need to include the '\n' character since it is sent
    //       in one UDP packet. However, it has a maximum size
    //       limit which we enforce here.
    //
    udp_client_server::udp_client client(addr, port);
    snap_communicator_message m(message);
    if(!secret_code.empty())
    {
        m.add_parameter("udp_secret", QString::fromUtf8(secret_code.c_str()));
    }
    QString const msg(m.to_message());
    QByteArray const utf8(msg.toUtf8());
    if(static_cast<size_t>(utf8.size()) > DATAGRAM_MAX_SIZE)
    {
        // packet too large for our buffers
        //
        throw snap_communicator_invalid_message("message too large for a UDP server");
    }
    if(client.send(utf8.data(), utf8.size()) != utf8.size()) // we do not send the '\0'
    {
        SNAP_LOG_ERROR("snap_udp_server_message_connection::send_message(): could not send UDP message.");
        return false;
    }

    return true;
}


/** \brief Implementation of the process_read() callback.
 *
 * This function reads the datagram we just received using the
 * recv() function. The size of the datagram cannot be more than
 * DATAGRAM_MAX_SIZE (1Kb at time of writing.)
 *
 * The message is then parsed and further processing is expected
 * to be accomplished in your implementation of process_message().
 *
 * The function actually reads as many pending datagrams as it can.
 */
void snap_communicator::snap_udp_server_message_connection::process_read()
{
    char buf[DATAGRAM_MAX_SIZE];
    for(;;)
    {
        ssize_t const r(recv(buf, sizeof(buf) / sizeof(buf[0]) - 1));
        if(r <= 0)
        {
            break;
        }
        buf[r] = '\0';
        QString const udp_message(QString::fromUtf8(buf));
        snap::snap_communicator_message message;
        if(message.from_message(udp_message))
        {
            QString const expected(QString::fromUtf8(get_secret_code().c_str()));
            if(message.has_parameter("udp_secret"))
            {
                QString const secret(message.get_parameter("udp_secret"));
                if(secret != expected)
                {
                    if(!expected.isEmpty())
                    {
                        // our secret code and the message secret code do not match
                        //
                        SNAP_LOG_ERROR("the incoming message has an unexpected udp_secret code, message ignored");
                        return;
                    }

                    // the sender included a UDP secret code but we don't
                    // require it so we emit a warning but still accept
                    // the message
                    //
                    SNAP_LOG_WARNING("no udp_secret=... parameter was expected (missing secret_code=... settings for this application?)");
                }
            }
            else if(!expected.isEmpty())
            {
                // secret code is missing from incoming message
                //
                SNAP_LOG_ERROR("the incoming message was expected to have udp_secret code, message ignored");
                return;
            }

            // we received a valid message, process it
            //
            dispatch_message(message);
        }
        else
        {
            SNAP_LOG_ERROR("snap_communicator::snap_udp_server_message_connection::process_read() was asked to process an invalid message (")(udp_message)(")");
        }
    }
}




/////////////////////////////////////////////////
// Snap TCP Blocking Client Message Connection //
/////////////////////////////////////////////////

/** \brief Blocking client message connection.
 *
 * This object allows you to create a blocking, generally temporary
 * one message connection client. This is specifically used with
 * the snaplock daemon, but it can be used for other things too as
 * required.
 *
 * The connection is expected to be used as shown in the following
 * example which is how it is used to implement the LOCK through
 * our snaplock daemons.
 *
 * \code
 *      class my_blocking_connection
 *          : public snap_tcp_blocking_client_message_connection
 *      {
 *      public:
 *          my_blocking_connection(std::string const & addr, int port, mode_t mode)
 *              : snap_tcp_blocking_client_message_connection(addr, port, mode)
 *          {
 *              // need to register with snap communicator
 *              snap_communicator_message register_message;
 *              register_message.set_command("REGISTER");
 *              ...
 *              blocking_connection.send_message(register_message);
 *
 *              run();
 *          }
 *
 *          ~my_blocking_connection()
 *          {
 *              // done, send UNLOCK and then make sure to unregister
 *              snap_communicator_message unlock_message;
 *              unlock_message.set_command("UNLOCK");
 *              ...
 *              blocking_connection.send_message(unlock_message);
 *
 *              snap_communicator_message unregister_message;
 *              unregister_message.set_command("UNREGISTER");
 *              ...
 *              blocking_connection.send_message(unregister_message);
 *          }
 *
 *          // now that we have a dispatcher, this  would probably use
 *          // that mechanism instead of a list of if()/else if()
 *          //
 *          // Please, consider using the dispatcher instead
 *          //
 *          virtual void process_message(snap_communicator_message const & message)
 *          {
 *              QString const command(message.get_command());
 *              if(command == "LOCKED")
 *              {
 *                  // the lock worked, release hand back to the user
 *                  done();
 *              }
 *              else if(command == "READY")
 *              {
 *                  // the REGISTER worked
 *                  // send the LOCK now
 *                  snap_communicator_message lock_message;
 *                  lock_message.set_command("LOCK");
 *                  ...
 *                  blocking_connection.send_message(lock_message);
 *              }
 *              else if(command == "HELP")
 *              {
 *                  // snapcommunicator wants us to tell it what commands
 *                  // we accept
 *                  snap_communicator_message commands_message;
 *                  commands_message.set_command("COMMANDS");
 *                  ...
 *                  blocking_connection.send_message(commands_message);
 *              }
 *          }
 *      };
 *      my_blocking_connection blocking_connection("127.0.0.1", 4040);
 *
 *      // then we can send a message to the service we are interested in
 *      blocking_connection.send_message(my_message);
 *
 *      // now we call run() waiting for a reply
 *      blocking_connection.run();
 * \endcode
 */
snap_communicator::snap_tcp_blocking_client_message_connection::snap_tcp_blocking_client_message_connection(std::string const & addr, int const port, mode_t const mode)
    : snap_tcp_client_message_connection(addr, port, mode, true)
{
}


/** \brief Blocking run on the connection.
 *
 * This function reads the incoming messages and calls process_message()
 * on each one of them, in a blocking manner.
 *
 * If you called mark_done() before, the done flag is reset back to false.
 * You will have to call mark_done() again if you again receive a message
 * that is expected to end the loop.
 *
 * \note
 * Internally, the function actually calls process_line() which transforms
 * the line in a message and in turn calls process_message().
 */
void snap_communicator::snap_tcp_blocking_client_message_connection::run()
{
    mark_not_done();

    do
    {
        for(;;)
        {
            // TBD: can the socket become -1 within the read() loop?
            //      (i.e. should not that be just outside of the for(;;)?)
            //
            struct pollfd fd;
            fd.events = POLLIN | POLLPRI | POLLRDHUP;
            fd.fd = get_socket();
            if(fd.fd < 0
            || !is_enabled())
            {
                // invalid socket
                process_error();
                return;
            }

            // at this time, this class is used with the lock and
            // the lock has a timeout so we need to block at most
            // for that amount of time and not forever (presumably
            // the snaplock would send us a LOCKFAILED marked with
            // a "timeout" parameter, but we cannot rely on the
            // snaplock being there and responding as expected.)
            //
            // calculate the number of microseconds and then convert
            // them to milliseconds for poll()
            //
            int64_t const next_timeout_timestamp(save_timeout_timestamp());
            int64_t const now(get_current_date());
            int64_t const timeout((next_timeout_timestamp - now) / 1000);
            if(timeout <= 0)
            {
                // timed out
                //
                process_timeout();
                if(is_done())
                {
                    return;
                }
                SNAP_LOG_FATAL("blocking connection timed out.");
                throw snap_communicator_runtime_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): blocking connection timed out");
            }
            errno = 0;
            fd.revents = 0; // probably useless... (kernel should clear those)
            int const r(::poll(&fd, 1, timeout));
            if(r < 0)
            {
                // r < 0 means an error occurred
                //
                if(errno == EINTR)
                {
                    // Note: if the user wants to prevent this error, he should
                    //       use the snap_signal with the Unix signals that may
                    //       happen while calling poll().
                    //
                    throw snap_communicator_runtime_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): EINTR occurred while in poll() -- interrupts are not supported yet though");
                }
                if(errno == EFAULT)
                {
                    throw snap_communicator_parameter_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): buffer was moved out of our address space?");
                }
                if(errno == EINVAL)
                {
                    // if this is really because nfds is too large then it may be
                    // a "soft" error that can be fixed; that being said, my
                    // current version is 16K files which frankly when we reach
                    // that level we have a problem...
                    //
                    struct rlimit rl;
                    getrlimit(RLIMIT_NOFILE, &rl);
                    throw snap_communicator_parameter_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): too many file fds for poll, limit is currently %1, your kernel top limit is %2")
                                .arg(rl.rlim_cur)
                                .arg(rl.rlim_max).toStdString());
                }
                if(errno == ENOMEM)
                {
                    throw snap_communicator_runtime_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): poll() failed because of memory");
                }
                int const e(errno);
                throw snap_communicator_runtime_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): poll() failed with error %1").arg(e));
            }

            if((fd.revents & (POLLIN | POLLPRI)) != 0)
            {
                // read one character at a time otherwise we would be
                // blocked forever
                //
                char buf[2];
                int const size(::read(fd.fd, buf, 1));
                if(size != 1)
                {
                    // invalid read
                    process_error();
                    throw snap_communicator_runtime_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): read() failed reading data from socket (return value = %1)").arg(size));
                }
                if(buf[0] == '\n')
                {
                    // end of a line, we got a whole message in our buffer
                    // notice that we do not add the '\n' to line
                    break;
                }
                buf[1] = '\0';
                f_line += buf;
            }
            if((fd.revents & POLLERR) != 0)
            {
                process_error();
                return;
            }
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                process_hup();
                return;
            }
            if((fd.revents & POLLNVAL) != 0)
            {
                process_invalid();
                return;
            }
        }
        process_line(QString::fromUtf8(f_line.c_str()));
        f_line.clear();
    }
    while(!is_done());
}


/** \brief Quick peek on the connection.
 *
 * This function checks for incoming messages and calls process_message()
 * on each one of them. If no messages are found on the pipe, then the
 * function returns immediately.
 *
 * \note
 * Internally, the function actually calls process_line() which transforms
 * the line in a message and in turn calls process_message().
 */
void snap_communicator::snap_tcp_blocking_client_message_connection::peek()
{
    do
    {
        for(;;)
        {
            struct pollfd fd;
            fd.events = POLLIN | POLLPRI | POLLRDHUP;
            fd.fd = get_socket();
            if(fd.fd < 0
            || !is_enabled())
            {
                // invalid socket
                process_error();
                return;
            }

            errno = 0;
            fd.revents = 0; // probably useless... (kernel should clear those)
            int const r(::poll(&fd, 1, 0));
            if(r < 0)
            {
                // r < 0 means an error occurred
                //
                if(errno == EINTR)
                {
                    // Note: if the user wants to prevent this error, he should
                    //       use the snap_signal with the Unix signals that may
                    //       happen while calling poll().
                    //
                    throw snap_communicator_runtime_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): EINTR occurred while in poll() -- interrupts are not supported yet though");
                }
                if(errno == EFAULT)
                {
                    throw snap_communicator_parameter_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): buffer was moved out of our address space?");
                }
                if(errno == EINVAL)
                {
                    // if this is really because nfds is too large then it may be
                    // a "soft" error that can be fixed; that being said, my
                    // current version is 16K files which frankly when we reach
                    // that level we have a problem...
                    //
                    struct rlimit rl;
                    getrlimit(RLIMIT_NOFILE, &rl);
                    throw snap_communicator_parameter_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): too many file fds for poll, limit is currently %1, your kernel top limit is %2")
                                .arg(rl.rlim_cur)
                                .arg(rl.rlim_max).toStdString());
                }
                if(errno == ENOMEM)
                {
                    throw snap_communicator_runtime_error("snap_communicator::snap_tcp_blocking_client_message_connection::run(): poll() failed because of memory");
                }
                int const e(errno);
                throw snap_communicator_runtime_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): poll() failed with error %1").arg(e));
            }

            if(r == 0)
            {
                return;
            }

            if((fd.revents & (POLLIN | POLLPRI)) != 0)
            {
                // read one character at a time otherwise we would be
                // blocked forever
                //
                char buf[2];
                int const size(::read(fd.fd, buf, 1));
                if(size != 1)
                {
                    // invalid read
                    process_error();
                    throw snap_communicator_runtime_error(QString("snap_communicator::snap_tcp_blocking_client_message_connection::run(): read() failed reading data from socket (return value = %1)").arg(size));
                }
                if(buf[0] == '\n')
                {
                    // end of a line, we got a whole message in our buffer
                    // notice that we do not add the '\n' to line
                    break;
                }
                buf[1] = '\0';
                f_line += buf;
            }
            if((fd.revents & POLLERR) != 0)
            {
                process_error();
                return;
            }
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                process_hup();
                return;
            }
            if((fd.revents & POLLNVAL) != 0)
            {
                process_invalid();
                return;
            }
        }
        process_line(QString::fromUtf8(f_line.c_str()));
        f_line.clear();
    }
    while(!is_done());
}


/** \brief Send the specified message to the connection on the other end.
 *
 * This function sends the specified message to the other side of the
 * socket connection. If the write somehow fails, then the function
 * returns false.
 *
 * The function blocks until the entire message was written to the
 * socket.
 *
 * \param[in] message  The message to send to the connection.
 * \param[in] cache  Whether to cache the message if it cannot be sent
 *                   immediately (ignored at the moment.)
 *
 * \return true if the message was sent succesfully, false otherwise.
 */
bool snap_communicator::snap_tcp_blocking_client_message_connection::send_message(snap_communicator_message const & message, bool cache)
{
    NOTUSED(cache);

    int const s(get_socket());
    if(s >= 0)
    {
        // transform the message to a string and write to the socket
        // the writing is blocking and thus fully synchronous so the
        // function blocks until the message gets fully sent
        //
        // WARNING: we cannot use f_connection.write() because that one
        //          is asynchronous (at least, it writes to a buffer
        //          and not directly to the socket!)
        //
        QString const msg(message.to_message());
        QByteArray const utf8(msg.toUtf8());
        std::string buf(utf8.data(), utf8.size());
        buf += "\n";
        return ::write(s, buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
    }

    return false;
}


/** \brief Overridden callback.
 *
 * This function is overriding the lower level process_error() to make
 * (mostly) sure that the remove_from_communicator() function does not
 * get called because that would generate the creation of a
 * snap_communicator object which we do not want with blocking
 * clients.
 */
void snap_communicator::snap_tcp_blocking_client_message_connection::process_error()
{
}









///////////////////////
// Snap Communicator //
///////////////////////


/** \brief Initialize a snap communicator object.
 *
 * This function initializes the snap_communicator object.
 */
snap_communicator::snap_communicator()
    //: f_connections() -- auto-init
    //, f_force_sort(true) -- auto-init
{
}


/** \brief Retrieve the instance() of the snap_communicator.
 *
 * This function returns the instance of the snap_communicator.
 * There is really no reason and it could also create all sorts
 * of problems to have more than one instance hence we created
 * the communicator as a singleton. It also means you cannot
 * actually delete the communicator.
 */
snap_communicator::pointer_t snap_communicator::instance()
{
    if(!g_instance)
    {
        g_instance.reset(new snap_communicator);
    }

    return g_instance;
}


/** \brief Retrieve a reference to the vector of connections.
 *
 * This function returns a reference to all the connections that are
 * currently attached to the snap_communicator system.
 *
 * This is useful to search the array.
 *
 * \return The vector of connections.
 */
snap_communicator::snap_connection::vector_t const & snap_communicator::get_connections() const
{
    return f_connections;
}


/** \brief Attach a connection to the communicator.
 *
 * This function attaches a connection to the communicator. This allows
 * us to execute code for that connection by having the process_signal()
 * function called.
 *
 * Connections are kept in the order in which they are added. This may
 * change the order in which connection callbacks are called. However,
 * events are received asynchronously so do not expect callbacks to be
 * called in any specific order.
 *
 * You may call this function with a null pointer. It simply returns
 * false immediately. This makes it easy to eventually allocate a
 * new connection and then use the return value of this function
 * to know whether the two step process worked or not.
 *
 * \note
 * A connection can only be added once to a snap_communicator object.
 * Also it cannot be shared between multiple communicator objects.
 *
 * \param[in] connection  The connection being added.
 *
 * \return true if the connection was added, false if the connection
 *         was already present in the communicator list of connections.
 */
bool snap_communicator::add_connection(snap_connection::pointer_t connection)
{
    if(connection == nullptr)
    {
        return false;
    }

    if(!connection->valid_socket())
    {
        throw snap_communicator_parameter_error("snap_communicator::add_connection(): connection without a socket cannot be added to a snap_communicator object.");
    }

    auto const it(std::find(f_connections.begin(), f_connections.end(), connection));
    if(it != f_connections.end())
    {
        // already added, can be added only once but we allow multiple
        // calls (however, we do not count those calls, so first call
        // to the remove_connection() does remove it!)
        //
        return false;
    }

    f_connections.push_back(connection);

    connection->connection_added();

    return true;
}


/** \brief Remove a connection from a snap_communicator object.
 *
 * This function removes a connection from this snap_communicator object.
 * Note that any one connection can only be added once.
 *
 * \param[in] connection  The connection to remove from this snap_communicator.
 *
 * \return true if the connection was removed, false if it was not found.
 */
bool snap_communicator::remove_connection(snap_connection::pointer_t connection)
{
    auto it(std::find(f_connections.begin(), f_connections.end(), connection));
    if(it == f_connections.end())
    {
        return false;
    }

    SNAP_LOG_TRACE("removing 1 connection, \"")(connection->get_name())("\", of ")(f_connections.size())(" connections (including this one.)");
    f_connections.erase(it);

    connection->connection_removed();

#if 0
#ifdef _DEBUG
std::for_each(
          f_connections.begin()
        , f_connections.end()
        , [](auto const & c)
        {
            SNAP_LOG_TRACE("snap_communicator::remove_connection(): remaining connection: \"")(c->get_name())("\"");
        });
#endif
#endif

    return true;
}


/** \brief Run until all connections are removed.
 *
 * This function "blocks" until all the events added to this
 * snap_communicator instance are removed. Until then, it
 * wakes up and run callback functions whenever an event occurs.
 *
 * In other words, you want to add_connection() before you call
 * this function otherwise the function returns immediately.
 *
 * Note that you can include timeout events so if you need to
 * run some code once in a while, you may just use a timeout
 * event and process your repetitive events that way.
 *
 * \return true if the loop exits because the list of connections is empty.
 */
bool snap_communicator::run()
{
    // the loop promises to exit once the even_base object has no
    // more connections attached to it
    //
    std::vector<bool> enabled;
    std::vector<struct pollfd> fds;
    f_force_sort = true;
    for(;;)
    {
        // any connections?
        if(f_connections.empty())
        {
            return true;
        }

        if(f_force_sort)
        {
            // sort the connections by priority
            //
            std::stable_sort(f_connections.begin(), f_connections.end(), snap_connection::compare);
            f_force_sort = false;
        }

        // make a copy because the callbacks may end up making
        // changes to the main list and we would have problems
        // with that here...
        //
        snap_connection::vector_t connections(f_connections);
        size_t max_connections(connections.size());

        // timeout is do not time out by default
        //
        int64_t next_timeout_timestamp(std::numeric_limits<int64_t>::max());

        // clear() is not supposed to delete the buffer of vectors
        //
        enabled.clear();
        fds.clear();
        fds.reserve(max_connections); // avoid more than 1 allocation
        for(size_t idx(0); idx < max_connections; ++idx)
        {
            snap_connection::pointer_t c(connections[idx]);
            c->f_fds_position = -1;

            // is the connection enabled?
            //
            // note that we save that value for later use in our loop
            // below because otherwise we will miss many events and
            // it tends to break things; that means you may get your
            // callback called even while disabled
            //
            enabled.push_back(c->is_enabled());
            if(!enabled[idx])
            {
                //SNAP_LOG_TRACE("snap_communicator::run(): connection '")(c->get_name())("' has been disabled, so ignored.");
                continue;
            }
//SNAP_LOG_TRACE("snap_communicator::run(): handling connection ")(idx)("/")(max_connections)(". '")(c->get_name())("' since it is enabled...");

            // check whether a timeout is defined in this connection
            //
            int64_t const timestamp(c->save_timeout_timestamp());
            if(timestamp != -1)
            {
                // the timeout event gives us a time when to tick
                //
                if(timestamp < next_timeout_timestamp)
                {
                    next_timeout_timestamp = timestamp;
                }
            }

            // is there any events to listen on?
            int e(0);
            if(c->is_listener() || c->is_signal())
            {
                e |= POLLIN;
            }
            if(c->is_reader())
            {
                e |= POLLIN | POLLPRI | POLLRDHUP;
            }
            if(c->is_writer())
            {
                e |= POLLOUT | POLLRDHUP;
            }
            if(e == 0)
            {
                // this should only happend on snap_timer objects
                //
                continue;
            }

            // do we have a currently valid socket? (i.e. the connection
            // may have been closed or we may be handling a timer or
            // signal object)
            //
            if(c->get_socket() < 0)
            {
                continue;
            }

            // this is considered valid, add this connection to the list
            //
            // save the position since we may skip some entries...
            // (otherwise we would have to use -1 as the socket to
            // allow for such dead entries, but avoiding such entries
            // saves time)
            //
            c->f_fds_position = fds.size();

//SNAP_LOG_ERROR("*** still waiting on \"")(c->get_name())("\".");
            struct pollfd fd;
            fd.fd = c->get_socket();
            fd.events = e;
            fd.revents = 0; // probably useless... (kernel should clear those)
            fds.push_back(fd);
        }

        // compute the right timeout
        int64_t timeout(-1);
        if(next_timeout_timestamp != std::numeric_limits<int64_t>::max())
        {
            int64_t const now(get_current_date());
            timeout = next_timeout_timestamp - now;
            if(timeout < 0)
            {
                // timeout is in the past so timeout immediately, but
                // still check for events if any
                timeout = 0;
            }
            else
            {
                // convert microseconds to milliseconds for poll()
                timeout /= 1000;
                if(timeout == 0)
                {
                    // less than one is a waste of time (CPU intenssive
                    // until the time is reached, we can be 1 ms off
                    // instead...)
                    timeout = 1;
                }
            }
        }
        else if(fds.empty())
        {
            SNAP_LOG_FATAL("snap_communicator::run(): nothing to poll() on. All connections are disabled? (Ignoring ")
                          (max_connections)(" and exiting the run() loop anyway.)");
            return false;
        }

//SNAP_LOG_TRACE("snap_communicator::run(): ")
//              ("count ")(fds.size())
//              ("timeout ")(timeout)
//              (" (next was: ")(next_timeout_timestamp)
//              (", current ~ ")(get_current_date())
//              (")");

        // TODO: add support for ppoll() so we can support signals cleanly
        //       with nearly no additional work from us
        //
        errno = 0;
        int const r(poll(&fds[0], fds.size(), timeout));
        if(r >= 0)
        {
            // quick sanity check
            //
            if(static_cast<size_t>(r) > connections.size())
            {
                throw snap_communicator_runtime_error("snap_communicator::run(): poll() returned a number of events to handle larger than the input allows");
            }
            //SNAP_LOG_TRACE("tid=")(gettid())(", snap_communicator::run(): ------------------- new set of ")(r)(" events to handle");

            // check each connection one by one for:
            //
            // 1) fds events, including signals
            // 2) timeouts
            //
            // and execute the corresponding callbacks
            //
            for(size_t idx(0); idx < connections.size(); ++idx)
            {
                snap_connection::pointer_t c(connections[idx]);

                // is the connection enabled?
                //
                // note that we check whether that connection was enabled
                // before poll() was called; this is very important because
                // the last poll() events must be run even if a previous
                // callback call just disabled this very connection
                // (i.e. at the time we called poll() the connection was
                // still enabled and therefore we are expected to call
                // their callbacks even if it just got disabled by an
                // earlier callback)
                //
                if(!enabled[idx])
                {
                    //SNAP_LOG_TRACE("snap_communicator::run(): in loop, connection '")(c->get_name())("' has been disabled, so ignored!");
                    continue;
                }

                // if we have a valid fds position then an event other
                // than a timeout occurred on that connection
                //
                if(c->f_fds_position >= 0)
                {
                    struct pollfd * fd(&fds[c->f_fds_position]);

                    // if any events were found by poll(), process them now
                    //
                    if(fd->revents != 0)
                    {
                        // an event happened on this one
                        //
                        if((fd->revents & (POLLIN | POLLPRI)) != 0)
                        {
                            // we consider that Unix signals have the greater priority
                            // and thus handle them first
                            //
                            if(c->is_signal())
                            {
                                snap_signal * ss(dynamic_cast<snap_signal *>(c.get()));
                                if(ss)
                                {
                                    ss->process();
                                }
                            }
                            else if(c->is_listener())
                            {
                                // a listener is a special case and we want
                                // to call process_accept() instead
                                //
                                c->process_accept();
                            }
                            else
                            {
                                c->process_read();
                            }
                        }
                        if((fd->revents & POLLOUT) != 0)
                        {
                            c->process_write();
                        }
                        if((fd->revents & POLLERR) != 0)
                        {
                            c->process_error();
                        }
                        if((fd->revents & (POLLHUP | POLLRDHUP)) != 0)
                        {
                            c->process_hup();
                        }
                        if((fd->revents & POLLNVAL) != 0)
                        {
                            c->process_invalid();
                        }
                    }
                }

                // now check whether we have a timeout on this connection
                //
                int64_t const timestamp(c->get_saved_timeout_timestamp());
                if(timestamp != -1)
                {
                    int64_t const now(get_current_date());
                    if(now >= timestamp)
                    {
//SNAP_LOG_TRACE("snap_communicator::run(): timer of connection = '")(c->get_name())
//    ("', timestamp = ")(timestamp)
//    (", now = ")(now)
//    (", now >= timestamp --> ")(now >= timestamp ? "TRUE (timed out!)" : "FALSE");

                        // move the timeout as required first
                        // (because the callback may move it again)
                        //
                        c->calculate_next_tick();

                        // the timeout date needs to be reset if the tick
                        // happened for that date
                        //
                        if(now >= c->get_timeout_date())
                        {
                            c->set_timeout_date(-1);
                        }

                        // then run the callback
                        //
                        c->process_timeout();
                    }
                }
            }
        }
        else
        {
            // r < 0 means an error occurred
            //
            if(errno == EINTR)
            {
                // Note: if the user wants to prevent this error, he should
                //       use the snap_signal with the Unix signals that may
                //       happen while calling poll().
                //
                throw snap_communicator_runtime_error("snap_communicator::run(): EINTR occurred while in poll() -- interrupts are not supported yet though");
            }
            if(errno == EFAULT)
            {
                throw snap_communicator_parameter_error("snap_communicator::run(): buffer was moved out of our address space?");
            }
            if(errno == EINVAL)
            {
                // if this is really because nfds is too large then it may be
                // a "soft" error that can be fixed; that being said, my
                // current version is 16K files which frankly when we reach
                // that level we have a problem...
                //
                struct rlimit rl;
                getrlimit(RLIMIT_NOFILE, &rl);
                throw snap_communicator_parameter_error(QString("snap_communicator::run(): too many file fds for poll, limit is currently %1, your kernel top limit is %2")
                            .arg(rl.rlim_cur)
                            .arg(rl.rlim_max));
            }
            if(errno == ENOMEM)
            {
                throw snap_communicator_runtime_error("snap_communicator::run(): poll() failed because of memory");
            }
            int const e(errno);
            throw snap_communicator_runtime_error(QString("snap_communicator::run(): poll() failed with error %1").arg(e));
        }
    }
}








} // namespace ed
// vim: ts=4 sw=4 et

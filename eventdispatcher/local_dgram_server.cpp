// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


// self
//
#include    "eventdispatcher/local_dgram_server.h"

#include    "eventdispatcher/exception.h"


// snapdev
//
#include    <snapdev/chownnm.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <sys/stat.h>
#include    <poll.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initialize a UDP server object.
 *
 * This function initializes a UDP server object making it ready to
 * receive messages.
 *
 * The server address and port are specified in the constructor so
 * if you need to receive messages from several different addresses
 * and/or port, you'll have to create a server for each.
 *
 * The address is a string and it can represent an IPv4 or IPv6
 * address.
 *
 * Note that this function calls bind() to listen to the socket
 * at the specified address. To accept data on different UDP addresses
 * and ports, multiple UDP servers must be created.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \warning
 * Remember that the multicast feature under Linux is shared by all
 * processes running on that server. Any one process can listen for
 * any and all multicast messages from any other process. Our
 * implementation limits the multicast from a specific IP. However.
 * other processes can also receive your packets and there is nothing
 * you can do to prevent that.
 *
 * \exception udp_client_server_runtime_error
 * The udp_client_server_runtime_error exception is raised when the address
 * and port combination cannot be resolved or if the socket cannot be
 * opened.
 *
 * \param[in] address  The address to connect/listen to.
 * \param[in] sequential  Whether the packets have to be 100% sequential.
 * \param[in] close_on_exec  Whether the socket has to be closed on execve().
 */
local_dgram_server::local_dgram_server(
              addr::addr_unix const & address
            , bool sequential
            , bool close_on_exec
            , bool force_reuse_addr)
    : local_dgram_base(address, sequential, close_on_exec)
{
    if(f_address.is_unnamed())
    {
        // for an unnamed socket, we do not bind at all the user is
        // responsible for knowing where to read and where to write
        //
        return;
    }

    sockaddr_un un;
    f_address.get_un(un);

    // bind to the first address
    //
    int r(-1);
    if(f_address.is_file())
    {
        // TODO: this is common code to the local_stream_server_connection.cpp
        //
        // a Unix file socket must create a new socket file to prove unicity
        // if the file already exists, even if it isn't used, the bind() call
        // will fail; if the file exists and the force_reuse_addr is true this
        // this function attempts to delete the file if it is a socket and we
        // can't connect to it (i.e. "lost file")
        //
        struct stat st = {};
        if(stat(un.sun_path, &st) == 0)
        {
            if(!S_ISSOCK(st.st_mode))
            {
                SNAP_LOG_ERROR
                    << "file \""
                    << un.sun_path
                    << "\" is not a socket; cannot listen on address \""
                    << f_address.to_uri()
                    << "\"."
                    << SNAP_LOG_SEND;
                throw runtime_error("file already exists and it is not a socket, can't create an AF_UNIX server.");
            }

            if(!force_reuse_addr)
            {
                SNAP_LOG_ERROR
                    << "file socket \""
                    << un.sun_path
                    << "\" already in use (errno: "
                    << std::to_string(EADDRINUSE)
                    << " -- "
                    << strerror(EADDRINUSE)
                    << "); cannot listen on address \""
                    << f_address.to_uri()
                    << "\"."
                    << SNAP_LOG_SEND;
                throw runtime_error("socket already exists, can't create an AF_UNIX server.");
            }

            r = f_address.unlink();
            if(r != 0
            && errno != ENOENT)
            {
                SNAP_LOG_ERROR
                    << "not able to delete file socket \""
                    << un.sun_path
                    << "\"; socket already in use (errno: "
                    << std::to_string(EADDRINUSE)
                    << " -- "
                    << strerror(EADDRINUSE)
                    << "); cannot listen on address \""
                    << f_address.to_uri()
                    << "\"."
                    << SNAP_LOG_SEND;
                throw runtime_error("could not unlink socket to reuse it as an AF_UNIX server.");
            }
        }
        r = bind(
                  f_socket.get()
                , reinterpret_cast<sockaddr const *>(&un)
                , sizeof(struct sockaddr_un));

        std::string const group(f_address.get_group());
        if(!group.empty())
        {
            if(snapdev::chownnm(un.sun_path, std::string(), group) != 0)
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "not able to change group ownership of socket file \""
                    << un.sun_path
                    << "\" (errno: "
                    << e
                    << " -- "
                    << strerror(e)
                    << "); cannot listen on address \""
                    << f_address.to_uri()
                    << "\"."
                    << SNAP_LOG_SEND;
                throw runtime_error("could not change group ownership on socket file.");
            }
        }

        // after the bind() we can then set the full permissions the way the
        // user wants them to be (also bind() applies the umask() so doing
        // that with the fchmod() above is likely to fail in many cases).
        //
        // note: we know that the path in un.snn_path is null terminated.
        //
        if(r == 0
        && chmod(un.sun_path, f_address.get_mode()) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "chmod() failed changing permissions after bind() (errno: "
                << e
                << " -- "
                << strerror(e)
                << ") on socket with address \""
                << f_address.to_uri()
                << "\"."
                << SNAP_LOG_SEND;
            throw runtime_error("could not change socket permissions.");
        }
    }
    else
    {
        // we want to limit the size because otherwise it would include
        // the '\0's after the specified name
        //
        std::size_t const size(sizeof(un.sun_family)
                                        + 1 // for the '\0' in sun_path[0]
                                        + strlen(un.sun_path + 1));
        r = bind(
                  f_socket.get()
                , reinterpret_cast<sockaddr const *>(&un)
                , size);
    }

    if(r != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR
                << "the bind() function failed with errno: "
                << e
                << " ("
                << strerror(e)
                << "); Unix address \""
                << f_address.to_uri()
                << "\"."
                << SNAP_LOG_SEND;
        throw runtime_error(
                "could not bind AF_UNIX datagram socket to \""
                + f_address.to_uri()
                + "\"");
    }
}


/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \param[in] msg  The buffer where the message is saved.
 * \param[in] max_size  The maximum size the message (i.e. size of the \p msg buffer.)
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
int local_dgram_server::recv(char * msg, size_t max_size)
{
    return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
}


/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] msg  The buffer where the message will be saved.
 * \param[in] max_size  The size of the \p msg buffer in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
int local_dgram_server::timed_recv(char * msg, size_t const max_size, int const max_wait_ms)
{
    pollfd fd;
    fd.events = POLLIN | POLLPRI | POLLRDHUP;
    fd.fd = f_socket.get();
    int const retval(poll(&fd, 1, max_wait_ms));
    if(retval == -1)
    {
        // poll() sets errno accordingly
        return -1;
    }
    if(retval > 0)
    {
        // our socket has data
        return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
    }

    // our socket has no data
    errno = EAGAIN;
    return -1;
}


/** \brief Wait for data to come in, but return a std::string.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] bufsize  The maximum size of the returned string in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return received string. an empty string if not data received or error.
 *
 * \sa timed_recv()
 */
std::string local_dgram_server::timed_recv(int const bufsize, int const max_wait_ms)
{
    std::vector<char> buf;
    buf.resize( bufsize + 1, '\0' ); // +1 for ending \0
    int const r(timed_recv(&buf[0], bufsize, max_wait_ms));
    if(r <= -1)
    {
        // Timed out, so return empty string.
        // TBD: could std::string() smash errno?
        //
        return std::string();
    }

    // Resize the buffer, then convert to std string
    //
    buf.resize(r + 1, '\0');

    std::string word;
    word.resize(r);
    std::copy(buf.begin(), buf.end(), word.begin());

    return word;
}




} // namespace ed
// vim: ts=4 sw=4 et

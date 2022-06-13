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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Handle an AF_UNIX socket as a server.
 *
 * This class is used to listen on a stream oriented connection using an
 * AF_UNIX type of socket.
 */

// self
//
#include    "eventdispatcher/local_stream_server_connection.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/local_stream_client_connection.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C lib
//
#include    <fcntl.h>
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize the local stream server.
 *
 * The server constructor creates a socket, binds it, and then listen on it.
 *
 * \todo
 * Fix docs.
 *
 * By default the server accepts a maximum of \p max_connections (set to
 * 0 or less to get the default tcp_server::MAX_CONNECTIONS) in its waiting queue.
 * If you use the server and expect a low connection rate, you may want to
 * reduce the count to 5. Although some very busy servers use larger numbers.
 * This value gets clamped to a minimum of 5 and a maximum of 1,000.
 *
 * Note that the maximum number of connections is actually limited to
 * /proc/sys/net/core/somaxconn connections. This number is generally 128 
 * in 2016. So the  super high limit of 1,000 is anyway going to be ignored
 * by the OS.
 *
 * The Unix socket is made non-reusable by default. It is possible to mark
 * the server address as reusable by setting the \p force_reuse_addr to true.
 * If not true, then the fact that the file exists prevents a new server from
 * being created. When true, the function first checks whether it can connect
 * to a server. It a connection succeeds, then the creation of the server
 * fails (i.e. the Unix socket is already in use). If the connection fails,
 * the function makes sure to delete the file if it exists.
 *
 * By default the server is marked as "keepalive". You can turn it off
 * using the keepalive() function with false.
 *
 * \note
 * The code here handles the file based sockets by attempting to delete
 * the file if it already exists. If you create services that could end
 * up using the exact same socket name, then it could be an issue. If
 * not then the code will do exactly what you expects. That being said.
 * If you want to increase your chance of proper clean up of a Unix
 * socket file, you would have to create a parent process which deletes
 * the file whenever the child dies. Then it can either restart the child
 * (on a crash) or just quit.
 *
 * \exception runtime_error
 * This exception is raised if the socket cannot be created, bound to
 * the specified Unix address, or listen() fails on the socket.
 *
 * \param[in] address  The Unix address to listen on.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] force_reuse_addr  Whether to allow the deletion of a file
 * before the bind() call.
 * \param[in] close_on_exec  Whether to close the server & client sockets
 * on an execve().
 */
local_stream_server_connection::local_stream_server_connection(
              addr::unix const & address
            , int max_connections
            , bool force_reuse_addr
            , bool close_on_exec)
    : f_address(address)
    , f_max_connections(max_connections)
    , f_close_on_exec(close_on_exec)
{
    if(f_max_connections < 5)
    {
        f_max_connections = 5;
    }
    else if(f_max_connections > 1000)
    {
        f_max_connections = 1000;
    }

    sockaddr_un un;
    f_address.get_un(un);
    f_socket.reset(socket(
                  un.sun_family
                , SOCK_STREAM | SOCK_NONBLOCK | (close_on_exec ? SOCK_CLOEXEC : 0)
                , 0));
    if(f_socket == nullptr)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "socket() failed creating a socket descriptor (errno: "
            << std::to_string(e)
            << " -- "
            << strerror(e)
            << "); cannot listen on address \""
            << f_address.to_uri()
            << "\"."
            << SNAP_LOG_SEND;
        throw runtime_error("could not create socket for AF_UNIX server");
    }

    if(f_address.is_unnamed())
    {
        // for an unnamed socket, we do not bind at all the user is
        // responsible for knowing where to read and where to write
        //
        return;
    }

    int r(-1);
    if(f_address.is_file())
    {
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
                throw runtime_error("file already exists and it is not a socket, can't create an AF_UNIX server");
            }

            bool available(false);
            if(force_reuse_addr)
            {
                SNAP_LOG_WARNING
                    << "attempting a contection to "
                    << f_address.to_uri()
                    << " as a client to see that the address is available for this server;"
                    << " on success this generates an expected fatal error which we catch here."
                    << SNAP_LOG_SEND;
                try
                {
                    // TODO: this generates a fatal error in the log which can
                    //       be disturbing... we could look at adding a tag on
                    //       that error and here mark the messages associated
                    //       with that tag as hidden so that way we do not get
                    //       the error output.
                    //
                    local_stream_client_connection test_connection(f_address);
                }
                catch(runtime_error const & e)
                {
                    // note: in Linux we can distinguish between a full
                    //       backlog (EAGAIN) and a disconnected socket
                    //       (ECONNREFUSED); we should not set available
                    //       to true on EAGAIN...
                    //
                    available = true;
                }
            }
            if(!available)
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
                throw runtime_error("socket already exists, can't create an AF_UNIX server");
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
                throw runtime_error("could not unlink socket to reuse it as an AF_UNIX server");
            }
        }
        r = bind(
                  f_socket.get()
                , reinterpret_cast<sockaddr const *>(&un)
                , sizeof(struct sockaddr_un));
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

    if(r < 0)
    {
        throw runtime_error(
                  "could not bind the socket to \""
                + f_address.to_uri()
                + "\"");
    }

    // start listening, we expect the caller to then call accept() to
    // acquire connections
    //
    if(listen(f_socket.get(), f_max_connections) < 0)
    {
        throw runtime_error(
                  "could not listen to the socket bound to \""
                + f_address.to_uri()
                + "\"");
    }
}


/** \brief Clean up the server socket.
 *
 * This function deletes the socket file if this service used such a socket.
 *
 * \note
 * If the server crashes, that delete may not happen. In order to allow
 * for a restart, calling the constructor and setting the force_reuse_addr
 * to true is what will generally work best.
 */
local_stream_server_connection::~local_stream_server_connection()
{
    f_address.unlink();
}


/** \brief Check whether this connection is a listener.
 *
 * This function already returns true since a server is a listener.
 * This allows us to have our process_accept() function called instead
 * of the process_read().
 *
 * \return Always true since this is a listening server.
 */
bool local_stream_server_connection::is_listener() const
{
    return true;
}


/** \brief Retrieve the socket descriptor.
 *
 * This function returns the socket descriptor. It can be used to
 * tweak things on the socket such as making it non-blocking or
 * directly accessing the data.
 *
 * \return The socket descriptor.
 */
int local_stream_server_connection::get_socket() const
{
    return f_socket.get();
}


/** \brief Retrieve the maximum number of connections.
 *
 * This function returns the maximum number of connections that can
 * be accepted by the socket. This was set by the constructor and
 * it cannot be changed later.
 *
 * \return The maximum number of incoming connections.
 */
int local_stream_server_connection::get_max_connections() const
{
    return f_max_connections;
}


/** \brief Retrieve one new connection.
 *
 * This function will wait until a new connection arrives and returns a
 * new bio_client object for each new connection.
 *
 * If the socket is made non-blocking then the function may return without
 * a bio_client object (i.e. a null pointer instead.)
 *
 * \return A file descriptor representing the new connection socket.
 */
snapdev::raii_fd_t local_stream_server_connection::accept()
{
    struct sockaddr_un un;
    socklen_t len(sizeof(un));
    snapdev::raii_fd_t r(::accept(
              f_socket.get()
            , reinterpret_cast<sockaddr *>(&un)
            , &len));
    if(r == nullptr)
    {
        throw runtime_error("failed accepting a new AF_UNIX client");
    }

    // force a close on execve() to avoid sharing the socket in child
    // processes
    //
    if(f_close_on_exec)
    {
        // if this call fails, we ignore the error, but still log the event
        //
        if(fcntl(r.get(), F_SETFD, FD_CLOEXEC) != 0)
        {
            SNAP_LOG_WARNING
                << "::accept(): an error occurred trying"
                   " to mark accepted AF_UNIX socket with FD_CLOEXEC."
                << SNAP_LOG_SEND;
        }
    }

    return r;
}


/** \brief Return the current state of the close-on-exec flag.
 *
 * This function returns the current state of the close-on-exec flag. This
 * is the flag as defined in the contructor or by the set_close_on_exec()
 * function. It does not represent the status of the server socket nor
 * of the clients that were accept()'ed by this class.
 *
 * It will, however, be used whenever the accept() is called in the future.
 *
 * \return The current status of the close-on-exec flag.
 */
bool local_stream_server_connection::get_close_on_exec() const
{
    return f_close_on_exec;
}


/** \brief Change the close-on-exec flag for future accept() calls.
 *
 * This function allows you to change the close-on-exec flag after
 * you created a Unix server. This means you may say that the server
 * needs to be closed, but not the connections or vice versa.
 *
 * \param[in] yes  Whether the close-on-exec will be set on sockets
 * returned by the accept() function.
 */
void local_stream_server_connection::set_close_on_exec(bool yes)
{
    f_close_on_exec = yes;
}


/** \brief Retrieve the server IP address.
 *
 * This function returns the IP address used to bind the socket. This
 * is the address clients have to use to connect to the server unless
 * the address was set to all zeroes (0.0.0.0) in which case any user
 * can connect.
 *
 * The IP address cannot be changed.
 *
 * \return The server IP address.
 */
addr::unix local_stream_server_connection::get_addr() const
{
    return f_address;
}





} // namespace ed
// vim: ts=4 sw=4 et

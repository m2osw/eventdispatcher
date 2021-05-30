// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/tcp_blocking_client_message_connection.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// C++ lib
//
#include    <cstring>


// C lib
//
#include    <poll.h>
#include    <sys/resource.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



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
 *          : public ed::tcp_blocking_client_message_connection
 *      {
 *      public:
 *          my_blocking_connection(std::string const & addr, int port, mode_t mode)
 *              : tcp_blocking_client_message_connection(addr, port, mode)
 *          {
 *              // need to register with communicator
 *              message register_message;
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
 *              message unlock_message;
 *              unlock_message.set_command("UNLOCK");
 *              ...
 *              blocking_connection.send_message(unlock_message);
 *
 *              message unregister_message;
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
 *          virtual void process_message(message const & message)
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
 *                  message lock_message;
 *                  lock_message.set_command("LOCK");
 *                  ...
 *                  blocking_connection.send_message(lock_message);
 *              }
 *              else if(command == "HELP")
 *              {
 *                  // snapcommunicator wants us to tell it what commands
 *                  // we accept
 *                  message commands_message;
 *                  commands_message.set_command("COMMANDS");
 *                  ...
 *                  blocking_connection.send_message(commands_message);
 *              }
 *          }
 *      };
 *      my_blocking_connection blocking_connection("127.0.0.1", 4040);
 *
 *      // then we can send a message to the service we are interested in
 *      my_blocking_connection.send_message(my_message);
 *
 *      // now we call run() waiting for a reply
 *      my_blocking_connection.run();
 * \endcode
 *
 * \param[in] addr  The address to connect to.
 * \param[in] port  The port to connect at.
 * \param[in] mode  The mode used to connect.
 */
tcp_blocking_client_message_connection::tcp_blocking_client_message_connection(
              std::string const & addr
            , int const port
            , mode_t const mode)
    : tcp_client_message_connection(addr, port, mode, true)
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
void tcp_blocking_client_message_connection::run()
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
            std::int64_t const next_timeout_timestamp(save_timeout_timestamp());
            std::int64_t const now(get_current_date());
            std::int64_t const timeout((next_timeout_timestamp - now) / 1000);
            if(timeout <= 0)
            {
                // timed out
                //
                process_timeout();
                if(is_done())
                {
                    return;
                }
                SNAP_LOG_FATAL
                    << "blocking connection timed out."
                    << SNAP_LOG_SEND;
                throw event_dispatcher_runtime_error(
                    "tcp_blocking_client_message_connection::run(): blocking"
                    " connection timed out.");
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
                    //       use the signal with the Unix signals that may
                    //       happen while calling poll().
                    //
                    throw event_dispatcher_runtime_error(
                            "tcp_blocking_client_message_connection::run():"
                            " EINTR occurred while in poll() -- interrupts"
                            " are not supported yet though.");
                }
                if(errno == EFAULT)
                {
                    throw event_dispatcher_runtime_error(
                            "tcp_blocking_client_message_connection::run():"
                            " buffer was moved out of our address space?");
                }
                if(errno == EINVAL)
                {
                    // if this is really because nfds is too large then it may be
                    // a "soft" error that can be fixed; that being said, my
                    // current version is 16K files which frankly when we reach
                    // that level we have a problem...
                    //
                    rlimit rl;
                    getrlimit(RLIMIT_NOFILE, &rl);
                    throw event_dispatcher_invalid_parameter(
                              "tcp_blocking_client_message_connection::run():"
                              " too many file fds for poll, limit is"
                              " currently "
                            + std::to_string(rl.rlim_cur)
                            + ", your kernel top limit is "
                            + std::to_string(rl.rlim_max)
                            + ".");
                }
                if(errno == ENOMEM)
                {
                    throw event_dispatcher_runtime_error(
                            "tcp_blocking_client_message_connection::run():"
                            " poll() failed because of memory.");
                }
                int const e(errno);
                throw event_dispatcher_invalid_parameter(
                          "tcp_blocking_client_message_connection::run():"
                          " poll() failed with error "
                        + std::to_string(e)
                        + " -- "
                        + strerror(e));
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
                    //
                    process_error();
                    throw event_dispatcher_invalid_parameter(
                              "tcp_blocking_client_message_connection::run():"
                              " read() failed reading data from socket"
                              " (return value = "
                            + std::to_string(size)
                            + ").");
                }
                if(buf[0] == '\n')
                {
                    // end of a line, we got a whole message in our buffer
                    // notice that we do not add the '\n' to line
                    //
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
        process_line(f_line);
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
void tcp_blocking_client_message_connection::peek()
{
    do
    {
        for(;;)
        {
            pollfd fd;
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
                    //       use the signal with the Unix signals that may
                    //       happen while calling poll().
                    //
                    throw event_dispatcher_runtime_error(
                            "tcp_blocking_client_message_connection::run():"
                            " EINTR occurred while in poll() -- interrupts"
                            " are not supported yet though");
                }
                if(errno == EFAULT)
                {
                    throw event_dispatcher_invalid_parameter(
                            "tcp_blocking_client_message_connection::run():"
                            " buffer was moved out of our address space?");
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
                    throw event_dispatcher_invalid_parameter(
                              "tcp_blocking_client_message_connection::run():"
                              " too many file fds for poll, limit is currently "
                            + std::to_string(rl.rlim_cur)
                            + ", your kernel top limit is "
                            + std::to_string(rl.rlim_max));
                }
                if(errno == ENOMEM)
                {
                    throw event_dispatcher_runtime_error(
                            "tcp_blocking_client_message_connection::run():"
                            " poll() failed because of memory");
                }
                int const e(errno);
                throw event_dispatcher_runtime_error(
                          "tcp_blocking_client_message_connection::run():"
                          " poll() failed with error "
                        + std::to_string(e)
                        + " -- "
                        + strerror(e));
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
                    throw event_dispatcher_runtime_error(
                              "tcp_blocking_client_message_connection::run():"
                              " read() failed reading data from socket (return"
                              " value = "
                            + std::to_string(size)
                            + ")");
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
        process_line(f_line);
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
 * \return true if the message was sent successfully, false otherwise.
 */
bool tcp_blocking_client_message_connection::send_message(message const & msg, bool cache)
{
    snap::NOTUSED(cache);

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
        std::string buf(msg.to_message());
        buf += '\n';
        return ::write(s, buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
    }

    return false;
}


/** \brief Overridden callback.
 *
 * This function is overriding the lower level process_error() to make
 * (mostly) sure that the remove_from_communicator() function does not
 * get called because that would generate the creation of a
 * communicator object which we do not want with blocking
 * clients.
 */
void tcp_blocking_client_message_connection::process_error()
{
}



} // namespace ed
// vim: ts=4 sw=4 et

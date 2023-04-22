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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

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
#include    "eventdispatcher/inter_thread_message_connection.h"

#include    "eventdispatcher/exception.h"


// cppthread
//
#include    <cppthread/thread.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C
//
#include    <poll.h>
#include    <sys/eventfd.h>
#include    <sys/resource.h>
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initializes the inter-thread connection.
 *
 * This function creates two queues to communicate between two threads.
 * At this point, we expect such connections to only be used between
 * two threads because we cannot listen on more than one socket.
 *
 * The connection is expected to be created by "thread A". This means
 * the send_message() for "thread A" adds messages to the queue of
 * "thread B" and the process_message() for "thread A" reads
 * messages from the "thread A" queue, and vice versa.
 *
 * In order to know whether a queue has data in it, we use an eventfd().
 * One of them is for "thread A" and the other is for "thread B".
 *
 * \todo
 * To support all the features of a connection on both sides
 * we would have to allocate a sub-connection object for thread B.
 * That sub-connection object would then be used just like a full
 * regular connection with all of its own parameters. Actually the
 * FIFO of messages could then clearly be segregated in each object.
 *
 * \exception initialization_error
 * This exception is raised if the pipes (socketpair) cannot be created.
 */
inter_thread_message_connection::inter_thread_message_connection()
{
    f_creator_id = cppthread::gettid();

    f_thread_a.reset(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE));
    if(!f_thread_a)
    {
        // eventfd could not be created
        //
        throw initialization_error("could not create eventfd for thread A");
    }

    f_thread_b.reset(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE));
    if(!f_thread_b)
    {
        f_thread_a.reset();

        // eventfd could not be created
        //
        throw initialization_error("could not create eventfd for thread B");
    }
}


/** \brief Make sure to close the eventfd objects.
 *
 * The destructor ensures that the eventfd objects allocated by the
 * constructor get closed.
 */
inter_thread_message_connection::~inter_thread_message_connection()
{
}


/** \brief Close the thread communication early.
 *
 * This function closes the pair of eventfd managed by this
 * inter-thread connection object.
 *
 * After this call, the inter-thread connection is closed and cannot be
 * used anymore. The read and write functions will return immediately
 * if called.
 */
void inter_thread_message_connection::close()
{
    f_thread_a.reset();
    f_thread_b.reset();
}


/** \brief Poll the connection in the child.
 *
 * There can be only one communicator, therefore, the thread
 * cannot make use of it since it is only for the main application.
 * This poll() function can be used by the child to wait on the
 * connection.
 *
 * You may specify a timeout as usual.
 *
 * \exception runtime_error
 * If an interrupt happens and stops the poll() then this exception is
 * raised. If not enough memory is available to run the poll() function,
 * this errors is raised.
 *
 * \exception parameter_error
 * Somehow a buffer was moved out of our client's space (really that one
 * is not likely to happen...). Too many file descriptors in the list of
 * fds (not likely to happen since we just have one!)
 *
 * \param[in] timeout  The maximum amount of time to wait in microseconds.
 *                     Use zero (0) to not block at all.
 *
 * \return -1 if an error occurs, 0 on success
 */
int inter_thread_message_connection::poll(int timeout)
{
    for(;;)
    {
        // are we even enabled?
        //
        struct pollfd fd;
        fd.events = POLLIN | POLLPRI | POLLRDHUP;
        fd.fd = get_socket();

        if(fd.fd < 0
        || !is_enabled())
        {
            return -1;
        }

        // we cannot use this connection timeout information; it would
        // otherwise be common to both threads; so instead we have
        // a parameter which is used by the caller to tell us how long
        // we have to wait
        //
        // convert microseconds to milliseconds for poll()
        //
        if(timeout > 0)
        {
            timeout /= 1000;
            if(timeout == 0)
            {
                // less than one is a waste of time (CPU intensive
                // until the time is reached, we can be 1 ms off
                // instead...)
                //
                timeout = 1;
            }
        }
        else
        {
            // negative numbers are adjusted to zero.
            //
            timeout = 0;
        }

        int const r(::poll(&fd, 1, timeout));
        if(r < 0)
        {
            // r < 0 means an error occurred
            //
            int const e(errno);

            if(e == EINTR)
            {
                // Note: if the user wants to prevent this error, he should
                //       use the signal with the Unix signals that may
                //       happen while calling poll().
                //
                throw runtime_error("EINTR occurred while in poll() -- interrupts are not supported yet though");
            }
            if(e == EFAULT)
            {
                throw parameter_error("buffer was moved out of our address space?");
            }
            if(e == EINVAL)
            {
                // if this is really because nfds is too large then it may be
                // a "soft" error that can be fixed; that being said, my
                // current version is 16K files which frankly when we reach
                // that level we have a problem...
                //
                struct rlimit rl;
                getrlimit(RLIMIT_NOFILE, &rl);
                throw parameter_error(
                            "too many file fds for poll, limit is currently "
                            + std::to_string(rl.rlim_cur)
                            + ", your kernel top limit is "
                            + std::to_string(rl.rlim_max));
            }
            if(e == ENOMEM)
            {
                throw runtime_error("poll() failed because of memory");
            }
            throw runtime_error(
                        "poll() failed with error: "
                        + std::to_string(e)
                        + " -- "
                        + strerror(e));
        }

        if(r == 0)
        {
            // poll() timed out, just return so the thread can do some
            // additional work
            //
            return 0;
        }

        // we reach here when there is something to read
        //
        if((fd.revents & (POLLIN | POLLPRI)) != 0)
        {
            process_read();
        }
        // at this point we do not request POLLOUT and assume that the
        // write() function will never fail
        //
        //if((fd.revents & POLLOUT) != 0)
        //{
        //    process_write();
        //}
        if((fd.revents & POLLERR) != 0)
        {
            process_error();
        }
        if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
        {
            process_hup();
        }
        if((fd.revents & POLLNVAL) != 0)
        {
            process_invalid();
        }
    }
    snapdev::NOT_REACHED();
}


/** \brief Pipe connections accept reads.
 *
 * This function returns true meaning that the pipe connection can be
 * used to read data.
 *
 * \return true since a pipe connection is a reader.
 */
bool inter_thread_message_connection::is_reader() const
{
    return true;
}


/** \brief This function returns the pipe we want to listen on.
 *
 * This function returns the file descriptor of one of the two
 * sockets. The parent process returns the descriptor of socket
 * number 0. The child process returns the descriptor of socket
 * number 1.
 *
 * \note
 * If the close() function was called, this function returns -1.
 *
 * \return A pipe descriptor to listen on with poll().
 */
int inter_thread_message_connection::get_socket() const
{
    if(f_creator_id == cppthread::gettid())
    {
        return f_thread_a.get();
    }

    return f_thread_b.get();
}


/** \brief Read one message from the FIFO.
 *
 * This function reads one message from the FIFO specific to this
 * thread. If the FIFO is empty, 
 *
 * The function makes sure to use the correct socket for the calling
 * process (i.e. depending on whether this is the parent or child.)
 *
 * Just like the system write(2) function, errno is set to the error
 * that happened when the function returns -1.
 *
 * \warning
 * At the moment this class does not support the dispatcher
 * extension.
 *
 * \return The number of bytes written to this pipe socket, or -1 on errors.
 */
void inter_thread_message_connection::process_read()
{
    message msg;

    bool const is_thread_a(f_creator_id == cppthread::gettid());

    // retrieve the message
    //
    bool const got_message((is_thread_a ? f_message_a : f_message_b).pop_front(msg, 0));

    // "remove" that one object from the semaphore counter
    //
    uint64_t value(1);
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wunused-result"
    if(read(is_thread_a ? f_thread_a.get() : f_thread_b.get(), &value, sizeof(value)) != sizeof(value))
    {
        throw runtime_error("an error occurred while reading from inter-thread eventfd description.");
    }
//#pragma GCC diagnostic pop

    // send the message for processing
    // got_message should always be true, but just in case...
    //
    if(got_message)
    {
        if(is_thread_a)
        {
            process_message_a(msg);
        }
        else
        {
            process_message_b(msg);
        }
    }
}


/** \brief Send a message to the other end of this connection.
 *
 * This function sends the specified \p message to the thread
 * on the other side of the connection.
 *
 * \note
 * We are not a writer. We directly write to the corresponding
 * thread eventfd() so it can wake up and read the message we
 * just sent. There is only one reason for which the write
 * would not be available, we already sent 2^64-2 messages,
 * which is not likely to happen since memory would not support
 * that many messages.
 *
 * \todo
 * One day we probably will want to be able to have support for a
 * process_write() callback... Maybe we should do the write there.
 * Only we need to know where the write() would have to happen...
 * That's a bit complicated right now for a feature that would not
 * get tested well...
 *
 * \param[in] msg  The message to send to the other side.
 * \param[in] cache  These messages are always cached so this is ignored.
 *
 * \return true of the message was sent, false if it was cached or failed.
 */
bool inter_thread_message_connection::send_message(
      message & msg
    , bool cache)
{
    snapdev::NOT_USED(cache);

    if(f_creator_id == cppthread::gettid())
    {
        f_message_b.push_back(msg);
        uint64_t const value(1);
        return write(f_thread_b.get(), &value, sizeof(value)) == sizeof(value);
    }
    else
    {
        f_message_a.push_back(msg);
        uint64_t const value(1);
        return write(f_thread_a.get(), &value, sizeof(value)) == sizeof(value);
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

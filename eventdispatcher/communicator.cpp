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
#include    "eventdispatcher/communicator.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/signal.h"
#include    "eventdispatcher/utils.h"


// cppthread lib
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <algorithm>


// C lib
//
#include    <poll.h>
#include    <sys/resource.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{
namespace
{


/** \brief The instance of the communicator singleton.
 *
 * This pointer is the one instance of the communicator
 * we create to run an event loop.
 */
communicator::pointer_t *           g_instance = nullptr;


} // no name namespace





/** \brief Initialize a snap communicator object.
 *
 * This function initializes the communicator object.
 */
communicator::communicator()
    //: f_connections() -- auto-init
    //, f_force_sort(true) -- auto-init
{
}


/** \brief Retrieve the instance() of the communicator.
 *
 * This function returns the instance of the communicator.
 * There is really no reason and it could also create all sorts
 * of problems to have more than one instance hence we created
 * the communicator as a singleton. It also means you cannot
 * actually delete the communicator.
 *
 * The initialization fo the communicator instance is thread
 * safe.
 */
communicator::pointer_t communicator::instance()
{
    cppthread::guard g(*cppthread::g_system_mutex);

    if(g_instance == nullptr)
    {
        g_instance = new communicator::pointer_t();
        g_instance->reset(new communicator());
    }

    return *g_instance;
}


/** \brief Retrieve a reference to the vector of connections.
 *
 * This function returns a reference to all the connections that are
 * currently attached to the communicator system.
 *
 * This is useful to search the array.
 *
 * \return The vector of connections.
 */
connection::vector_t const & communicator::get_connections() const
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
 * A connection can only be added once to a communicator object.
 * Also it cannot be shared between multiple communicator objects.
 *
 * \param[in] connection  The connection being added.
 *
 * \return true if the connection was added, false if the connection
 *         was already present in the communicator list of connections.
 */
bool communicator::add_connection(connection::pointer_t connection)
{
    if(connection == nullptr)
    {
        return false;
    }

    if(!connection->valid_socket())
    {
        throw event_dispatcher_invalid_parameter(
            "communicator::add_connection(): connection without a socket"
            " cannot be added to a communicator object.");
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


/** \brief Remove a connection from a communicator object.
 *
 * This function removes a connection from this communicator object.
 * Note that any one connection can only be added once.
 *
 * \param[in] connection  The connection to remove from this communicator.
 *
 * \return true if the connection was removed, false if it was not found.
 */
bool communicator::remove_connection(connection::pointer_t connection)
{
    auto it(std::find(f_connections.begin(), f_connections.end(), connection));
    if(it == f_connections.end())
    {
        return false;
    }

    SNAP_LOG_TRACE
        << "removing 1 connection, \""
        << connection->get_name()
        << "\", of "
        << f_connections.size()
        << " connections (including this one.)"
        << SNAP_LOG_SEND;

    f_connections.erase(it);

    connection->connection_removed();

#if 0
#ifdef _DEBUG
std::for_each(
          f_connections.begin()
        , f_connections.end()
        , [](auto const & c)
        {
            SNAP_LOG_TRACE
                << "communicator::remove_connection(): remaining connection: \""
                << c->get_name()
                << "\""
                << SNAP_LOG_SEND;
        });
#endif
#endif

    return true;
}


/** \brief Set the Force Sort flag to \p status.
 *
 * This function can be called to force the run() function to sort (or not
 * sort) the list of connections.
 *
 * Since the sort function is somewhat expensive, the sort changes the
 * vector of connections in place. Then only a change of priority
 * triggers a request for the vector to be sorted again.
 *
 * This function can be used in the event you need to force a trigger.
 * It would be unlikely that you would call this function with false.
 *
 * \param[in] status  The new status of the force sort flag.
 */
void communicator::set_force_sort(bool status)
{
    f_force_sort = status;
}


/** \brief Run until all connections are removed.
 *
 * This function "blocks" until all the events added to this
 * communicator instance are removed. Until then, it
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
bool communicator::run()
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
            std::stable_sort(f_connections.begin(), f_connections.end(), connection::compare);
            f_force_sort = false;
        }

        // make a copy because the callbacks may end up making
        // changes to the main list and we would have problems
        // with that here...
        //
        connection::vector_t connections(f_connections);
        size_t max_connections(connections.size());

        // timeout is do not time out by default
        //
        std::int64_t next_timeout_timestamp(std::numeric_limits<std::int64_t>::max());

        // clear() is not supposed to delete the buffer of vectors
        //
        enabled.clear();
        fds.clear();
        fds.reserve(max_connections); // avoid more than 1 allocation
        for(size_t idx(0); idx < max_connections; ++idx)
        {
            connection::pointer_t c(connections[idx]);
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
                //SNAP_LOG_TRACE
                //    << "communicator::run(): connection '"
                //    << c->get_name()
                //    << "' has been disabled, so ignored."
                //    << SNAP_LOG_SEND;
                continue;
            }
//SNAP_LOG_TRACE
//    << "communicator::run(): handling connection "
//    << idx
//    << "/"
//    << max_connections
//    << ". '"
//    << c->get_name()
//    << "' since it is enabled..."
//    << SNAP_LOG_SEND;

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

//SNAP_LOG_ERROR
//    << "*** still waiting on \""
//    << c->get_name()
//    << "\"."
//    << SNAP_LOG_SEND;

            struct pollfd fd;
            fd.fd = c->get_socket();
            fd.events = e;
            fd.revents = 0; // probably useless... (kernel should clear those)
            fds.push_back(fd);
        }

        // compute the right timeout
        std::int64_t timeout(-1);
        if(next_timeout_timestamp != std::numeric_limits<int64_t>::max())
        {
            std::int64_t const now(get_current_date());
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
            SNAP_LOG_FATAL
                << "communicator::run(): nothing to poll() on. All connections are disabled? (Ignoring "
                << max_connections
                << " and exiting the run() loop anyway.)"
                << SNAP_LOG_SEND;
            return false;
        }

//SNAP_LOG_TRACE << "communicator::run(): "
//               << "count " << fds.size()
//               << "timeout " << timeout
//               << " (next was: " << next_timeout_timestamp
//               << ", current ~ " << get_current_date()
//               << ")"
//               << SNAP_LOG_SEND;

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
                throw event_dispatcher_runtime_error("communicator::run(): poll() returned a number of events to handle larger than the input allows");
            }
            //SNAP_LOG_TRACE
            //    <<"tid="
            //    << gettid()
            //    << ", communicator::run(): ------------------- new set of "
            //    << r
            //    << " events to handle"
            //    << SNAP_LOG_SEND;

            // check each connection one by one for:
            //
            // 1) fds events, including signals
            // 2) timeouts
            //
            // and execute the corresponding callbacks
            //
            for(size_t idx(0); idx < connections.size(); ++idx)
            {
                connection::pointer_t c(connections[idx]);

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
                    //SNAP_LOG_TRACE
                    //    << "communicator::run(): in loop, connection '"
                    //    << c->get_name()
                    //    << "' has been disabled, so ignored!"
                    //    << SNAP_LOG_SEND;
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
                                signal * ss(dynamic_cast<signal *>(c.get()));
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
//SNAP_LOG_TRACE
//    << "communicator::run(): timer of connection = '"<< c->get_name()
//    << "', timestamp = " << timestamp
//    << ", now = " << now
//    << ", now >= timestamp --> " << (now >= timestamp ? "TRUE (timed out!)" : "FALSE")
//    << SNAP_LOG_SEND;

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
                throw event_dispatcher_runtime_error("communicator::run(): EINTR occurred while in poll() -- interrupts are not supported yet though");
            }
            if(errno == EFAULT)
            {
                throw event_dispatcher_invalid_parameter("communicator::run(): buffer was moved out of our address space?");
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
                            "communicator::run(): too many file fds for poll, limit is currently "
                          + std::to_string(rl.rlim_cur)
                          + ", your kernel top limit is "
                          + std::to_string(rl.rlim_max));
            }
            if(errno == ENOMEM)
            {
                throw event_dispatcher_runtime_error("communicator::run(): poll() failed because of memory");
            }
            int const e(errno);
            throw event_dispatcher_runtime_error(
                      "communicator::run(): poll() failed with error "
                      + std::to_string(e)
                      + " -- "
                      + strerror(e));
        }
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

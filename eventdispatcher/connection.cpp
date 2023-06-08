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
 * \brief Implementation of the connection base class.
 *
 * All the connection classes must derive from the connection base
 * class.
 *
 * Connections are able to handle TCP, UDP, Unix signals, etc. The
 * base class give us all the necessary defaults for all the connections.
 */


// self
//
#include    "eventdispatcher/connection.h"

#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/utils.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <cstring>


// C
//
#include    <sys/ioctl.h>
#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initializes the connection.
 *
 * This function initializes a base connection object.
 */
connection::connection()
{
}


/** \brief Proceed with the cleanup of the connection.
 *
 * This function cleans up a connection object.
 */
connection::~connection()
{
}


/** \brief Remove this connection from the communicator it was added in.
 *
 * This function removes the connection from the communicator that
 * it was created in.
 *
 * This happens in several circumstances:
 *
 * \li When the connection is not necessary anymore
 * \li When the connection receives a message saying it should close
 * \li When the connection receives a Hang Up event
 * \li When the connection looks erroneous
 * \li When the connection looks invalid
 *
 * If the connection is not currently connected to a communicator
 * object, then nothing happens.
 */
void connection::remove_from_communicator()
{
    communicator::instance()->remove_connection(shared_from_this());
}


/** \brief Retrieve the name of the connection.
 *
 * When generating an error or a log the library makes use of this name
 * so we actually know which type of socket generated a problem.
 *
 * \return A constant reference to the connection name.
 */
std::string const & connection::get_name() const
{
    return f_name;
}


/** \brief Change the name of the connection.
 *
 * A connection can be given a name. This is mainly for debug purposes.
 * We will be adding this name in errors and exceptions as they occur.
 *
 * The connection makes a copy of \p name.
 *
 * \param[in] name  The name to give this connection.
 */
void connection::set_name(std::string const & name)
{
    f_name = name;
}


/** \brief Tell us whether this socket is a listener or not.
 *
 * By default a connection object does not represent a listener
 * object.
 *
 * \return The base implementation returns false. Override this
 *         virtual function if your connection is a listener.
 */
bool connection::is_listener() const
{
    return false;
}


/** \brief Tell us whether this connection is listening on a Unix signal.
 *
 * By default a connection object does not represent a Unix signal.
 * See the signal implementation for further information about
 * Unix signal handling in this library.
 *
 * \return The base implementation returns false.
 */
bool connection::is_signal() const
{
    return false;
}


/** \brief Tell us whether this socket is used to receive data.
 *
 * If you expect to receive data on this connection, then mark it
 * as a reader by returning true in an overridden version of this
 * function.
 *
 * \return By default this function returns false (nothing to read).
 */
bool connection::is_reader() const
{
    return false;
}


/** \brief Tell us whether this socket is used to send data.
 *
 * If you expect to send data on this connection, then mark it
 * as a writer by returning true in an overridden version of
 * this function.
 *
 * \return By default this function returns false (nothing to write).
 */
bool connection::is_writer() const
{
    return false;
}


/** \brief Check whether the socket is valid for this connection.
 *
 * Some connections do not make use of a socket so just checking
 * whether the socket is -1 is not a good way to know whether the
 * socket is valid.
 *
 * The default function assumes that a socket has to be 0 or more
 * to be valid. Other connection implementations may overload this
 * function to allow other values.
 *
 * \return true if the socket is valid.
 */
bool connection::valid_socket() const
{
    return get_socket() >= 0;
}


/** \brief Check whether this connection is enabled.
 *
 * It is possible to turn a connection ON or OFF using the set_enable()
 * function. This function returns the current value. If true, which
 * is the default, the connection is considered enabled and will get
 * its callbacks called.
 *
 * \return true if the connection is currently enabled.
 */
bool connection::is_enabled() const
{
    return f_enabled;
}


/** \brief Change the status of a connection.
 *
 * This function let you change the status of a connection from
 * enabled (true) to disabled (false) and vice versa.
 *
 * A disabled connection is not listened on at all. This is similar
 * to returning false in all three functions is_listener(),
 * is_reader(), and is_writer().
 *
 * \param[in] enabled  The new status of the connection.
 */
void connection::set_enable(bool enabled)
{
    f_enabled = enabled;
}


/** \brief Define the priority of this connection object.
 *
 * By default connection objects have a priority of 100.
 *
 * You may also use the set_priority() to change the priority of a
 * connection at any time.
 *
 * \return The current priority of this connection.
 *
 * \sa set_priority()
 */
int connection::get_priority() const
{
    return f_priority;
}


/** \brief Change this event priority.
 *
 * This function can be used to change the default priority (which is
 * 100) to a larger or smaller number. A larger number makes the connection
 * less important and callbacks get called later. A smaller number makes
 * the connection more important and callbacks get called sooner.
 *
 * Note that the priority of a connection can modified at any time.
 * It is not guaranteed to be taken in account immediately, though.
 *
 * \exception parameter_error
 * The priority of the event is out of range when this exception is raised.
 * The value must between between 0 and EVENT_MAX_PRIORITY. Any
 * other value raises this exception.
 *
 * \param[in] priority  Priority of the event.
 */
void connection::set_priority(priority_t priority)
{
    if(priority < 0 || priority > EVENT_MAX_PRIORITY)
    {
        std::string err("connection::set_priority(): priority out of range,"
                        " this instance of connection accepts priorities between 0 and ");
        err += std::to_string(EVENT_MAX_PRIORITY);
        err += ".";
        throw parameter_error(err);
    }

    f_priority = priority;

    // make sure that the new order is calculated when we execute
    // the next loop
    //
    communicator::instance()->set_force_sort();
}


/** \brief Less than operator to sort connections by priority.
 *
 * This function is used to know whether a connection has a higher or lower
 * priority. This is used when one adds, removes, or changes the priority
 * of a connection. The sorting itself happens in the
 * communicator::run() which knows that something changed whenever
 * it checks the data.
 *
 * The result of the priority mechanism is that callbacks of items with
 * a smaller priority will be called first.
 *
 * \param[in] lhs  The left hand side connection.
 * \param[in] rhs  The right hand side connection.
 *
 * \return true if lhs has a smaller priority than rhs.
 */
bool connection::compare(pointer_t const & lhs, pointer_t const & rhs)
{
    return lhs->get_priority() < rhs->get_priority();
}


/** \brief Get the number of events a connection will process in a row.
 *
 * Depending on the connection, their events may get processed within
 * a loop. If a new event is received before the current event being
 * processed is done, then the system generally processes that new event
 * before exiting the loop.
 *
 * This count limit specifies that a certain amount of events can be
 * processed in a row. After that many events were processed, the loop
 * exits.
 *
 * Some loops may not allow for us to immediately quit that function. In
 * that case we go on until a breaking point is allowed.
 *
 * \return The total amount of microsecond allowed before a connection
 * processing returns even if additional events are already available
 * in connection.
 *
 * \sa set_event_limit()
 */
uint16_t connection::get_event_limit() const
{
    return f_event_limit;
}


/** \brief Set the number of events a connection will process in a row.
 *
 * Depending on the connection, their events may get processed within
 * a loop. If a new event is received before the current event being
 * processed is done, then the system generally processes that new event
 * before exiting the loop.
 *
 * This count limit specifies that a certain amount of events can be
 * processed in a row. After that many events were processed, the loop
 * exits.
 *
 * Some loops may not allow for us to immediately quit that function. In
 * that case we go on until a breaking point is allowed.
 *
 * \param[in] event_limit  Number of events to process in a row.
 *
 * \sa get_event_limit()
 */
void connection::set_event_limit(uint16_t event_limit)
{
    f_event_limit = event_limit;
}


/** \brief Get the processing time limit while processing a connection events.
 *
 * Depending on the connection, their events may get processed within
 * a loop. If a new event is received before the current event being
 * processed is done, then the system generally processes that new event
 * before exiting the loop.
 *
 * This count limit specifies that a certain amount of events can be
 * processed in a row. After that many events were processed, the loop
 * exits.
 *
 * Some loops may not allow for us to immediately quit that function. In
 * that case we go on until a breaking point is allowed.
 *
 * \return The total amount of microsecond allowed before a connection
 * processing returns even if additional events are already available
 * in connection.
 *
 * \sa set_processing_time_limit()
 */
uint16_t connection::get_processing_time_limit() const
{
    return f_processing_time_limit;
}


/** \brief Set the processing time limit while processing a connection events.
 *
 * Depending on the connection, their events may get processed within
 * a loop. If a new event is received before the current event being
 * processed is done, then the system generally processes that new event
 * before exiting the loop.
 *
 * This time limit gives a certain amount of time for a set of events
 * to get processed. The default is 0.5 seconds. Note that the system
 * won't stop the current event after 0.5 seconds, however, if it
 * takes that long or more, then it will not try to process another
 * event within that loop before it checks all the connections that
 * exist in your process.
 *
 * Some loops may not allow for us to immediately quit that function. In
 * that case we go on until a breaking point is allowed.
 *
 * \param[in] processing_time_limit  The total amount of microsecond
 *            allowed before a connection processing returns even if
 *            additional events are already available in connection.
 *
 * \sa get_processing_time_limit()
 */
void connection::set_processing_time_limit(std::int32_t processing_time_limit)
{
    // in microseconds.
    //
    f_processing_time_limit = processing_time_limit;
}


/** \brief Return the delay between ticks when this connection times out.
 *
 * All connections can include a timeout delay in microseconds which is
 * used to know when the wait on that specific connection times out.
 *
 * By default connections do not time out. This function returns -1
 * to indicate that this connection does not ever time out. To
 * change the timeout delay use the set_timeout_delay() function.
 *
 * \return This function returns the current timeout delay.
 */
int64_t connection::get_timeout_delay() const
{
    return f_timeout_delay;
}


/** \brief Change the timeout of this connection.
 *
 * Each connection can be setup with a timeout in microseconds.
 * When that delay is past, the callback function of the connection
 * is called with the EVENT_TIMEOUT flag set (note that the callback
 * may happen along other events.)
 *
 * The current date when this function gets called is the starting
 * point for each following trigger. Because many other callbacks
 * get called, it is not very likely that you will be called
 * exactly on time, but the ticks are guaranteed to be requested
 * on a non moving schedule defined as:
 *
 * \f[
 * \large tick_i = start-time + k \times delay
 * \f]
 *
 * In other words the time and date when ticks happen does not slip
 * with time. However, this implementation may skip one or more
 * ticks at any time (especially if the delay is very small).
 *
 * When a tick triggers an EVENT_TIMEOUT, the communicator::run()
 * function calls calculate_next_tick() to calculate the time when
 * the next tick will occur which will always be in the function.
 *
 * \exception parameter_error
 * This exception is raised if the timeout_us parameter is not considered
 * valid. The minimum value is 10 and microseconds. You may use -1 to turn
 * off the timeout delay feature.
 *
 * \param[in] timeout_us  The new time out in microseconds.
 */
void connection::set_timeout_delay(std::int64_t timeout_us)
{
    if(timeout_us != -1
    && timeout_us < 10)
    {
        throw parameter_error(
                      "connection::set_timeout_delay():"
                      " timeout_us parameter cannot be less than 10"
                      " unless it is exactly -1, "
                    + std::to_string(timeout_us)
                    + " is not valid.");
    }

    f_timeout_delay = timeout_us;

    // immediately calculate the next timeout date
    //
    f_timeout_next_date = get_current_date() + f_timeout_delay;
}


void connection::set_timeout_delay(snapdev::timespec_ex const & timeout_ns)
{
    set_timeout_delay(timeout_ns.to_usec());
}


/** \brief Calculate when the next tick shall occur.
 *
 * This function calculates the date and time when the next tick
 * has to be triggered. This function is called after the
 * last time the EVENT_TIMEOUT callback was called.
 */
void connection::calculate_next_tick()
{
    if(f_timeout_delay == -1)
    {
        // no delay based timeout so forget about it
        //
        return;
    }

    // what is now?
    //
    int64_t const now(get_current_date());

    // gap between now and the last time we triggered this timeout
    //
    int64_t const gap(now - f_timeout_next_date);
    if(gap < 0)
    {
        // somehow we got called even though now is still larger
        // than f_timeout_next_date
        //
        // This message happens all the time, it is not helpful at the moment
        // so commenting out.
        //
        //SNAP_LOG_DEBUG
        //        << "connection::calculate_next_tick()"
        //           " called even though the next date is still larger than 'now'."
        //        << SNAP_LOG_SEND;
        return;
    }

    // number of ticks in that gap, rounded up
    int64_t const ticks((gap + f_timeout_delay - 1) / f_timeout_delay);

    // the next date may be equal to now, however, since it is very
    // unlikely that the tick has happened right on time, and took
    // less than 1ms, this is rather unlikely all around...
    //
    f_timeout_next_date += ticks * f_timeout_delay;
}


/** \brief Return when this connection times out.
 *
 * All connections can include a timeout in microseconds which is
 * used to know when the wait on that specific connection times out.
 *
 * By default connections do not time out. This function returns -1
 * to indicate that this connection does not ever time out. You
 * may overload this function to return a different value so your
 * version can time out.
 *
 * \return This function returns the timeout date in microseconds.
 */
int64_t connection::get_timeout_date() const
{
    return f_timeout_date;
}


/** \brief Change the date at which you want a timeout event.
 *
 * This function can be used to setup one specific date and time
 * at which this connection should timeout. This specific date
 * is used internally to calculate the amount of time the poll()
 * will have to wait, not including the time it will take
 * to execute other callbacks if any needs to be run (i.e. the
 * timeout is executed last, after all other events, and also
 * priority is used to know which other connections are parsed
 * first.)
 *
 * \exception parameter_error
 * If the date_us is too small (less than -1) then this exception
 * is raised.
 *
 * \param[in] date_us  The new time out in micro seconds.
 */
void connection::set_timeout_date(std::int64_t date_us)
{
    if(date_us < -1)
    {
        throw parameter_error(
                      "connection::set_timeout_date():"
                      " date_us parameter cannot be less than -1, "
                    + std::to_string(date_us)
                    + " is not valid.");
    }

    f_timeout_date = date_us;
}


void connection::set_timeout_date(snapdev::timespec_ex const & date)
{
    set_timeout_date(date.to_usec());
}


/** \brief Return when this connection expects a timeout.
 *
 * All connections can include a timeout specification which is
 * either a specific day and time set with set_timeout_date()
 * or an repetitive timeout which is defined with the
 * set_timeout_delay().
 *
 * If neither timeout is set the function returns -1. Otherwise
 * the function will calculate when the connection is to time
 * out and return that date.
 *
 * If the date is already in the past then the callback
 * is called immediately with the EVENT_TIMEOUT flag set.
 *
 * \note
 * If the timeout date is triggered, then the loop calls
 * set_timeout_date(-1) because the date timeout is expected
 * to only be triggered once. This resetting is done before
 * calling the user callback which can in turn set a new
 * value back in the connection object.
 *
 * \return This function returns -1 when no timers are set
 *         or a timestamp in microseconds when the timer is
 *         expected to trigger.
 */
int64_t connection::get_timeout_timestamp() const
{
    if(f_timeout_date != -1)
    {
        // this one is easy, it is already defined as expected
        //
        return f_timeout_date;
    }

    if(f_timeout_delay != -1)
    {
        // this one makes use of the calculated next date
        //
        return f_timeout_next_date;
    }

    // no timeout defined
    //
    return -1;
}


/** \brief Save the timeout stamp just before calling poll().
 *
 * This function is called by the run() function before the poll()
 * gets called. It makes sure to save the timeout timestamp so
 * when we check the connections again after poll() returns and
 * any number of callbacks were called, the timeout does or does
 * not happen as expected.
 *
 * \return The timeout timestamp as returned by get_timeout_timestamp().
 *
 * \sa get_saved_timeout_timestamp()
 * \sa run()
 */
int64_t connection::save_timeout_timestamp()
{
    f_saved_timeout_stamp = get_timeout_timestamp();
    return f_saved_timeout_stamp;
}


/** \brief Get the saved timeout timestamp.
 *
 * This function returns the timeout as saved by the
 * save_timeout_timestamp() function. The timestamp returned by
 * this function was frozen so if the user calls various timeout
 * functions that could completely change the timeout stamp that
 * the get_timeout_timestamp() would return just at the time we
 * want to know whether th timeout callback needs to be called
 * will be ignored by the loop.
 *
 * \return The saved timeout stamp as returned by save_timeout_timestamp().
 *
 * \sa save_timeout_timestamp()
 * \sa run()
 */
int64_t connection::get_saved_timeout_timestamp() const
{
    return f_saved_timeout_stamp;
}


/** \brief Make this connection socket a non-blocking socket.
 *
 * For the read and write to work as expected we generally need
 * to make those sockets non-blocking.
 *
 * For accept(), you do just one call and return and it will not
 * block on you. It is important to not setup a socket you
 * listen on as non-blocking if you do not want to risk having the
 * accepted sockets non-blocking.
 */
void connection::non_blocking() const
{
    if(valid_socket()
    && get_socket() >= 0)
    {
        int optval(1);
        if(ioctl(get_socket(), FIONBIO, &optval) == -1)
        {
            int const e(errno);
            SNAP_LOG_WARNING
                << "connection::non_blocking(): error "
                << e
                << " ("
                << strerror(e)
                << ") occurred trying to mark socket as non-blocking."
                << SNAP_LOG_SEND;
        }
    }
}


/** \brief Ask the OS to keep the socket alive.
 *
 * This function marks the socket with the SO_KEEPALIVE flag. This means
 * the OS implementation of the network stack should regularly send
 * small messages over the network to keep the connection alive.
 *
 * The function returns whether the function works or not. If the function
 * fails, it logs a warning and returns.
 */
void connection::keep_alive() const
{
    if(get_socket() != -1)
    {
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        if(setsockopt(get_socket(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
        {
            int const e(errno);
            SNAP_LOG_WARNING
                << "connection::keep_alive(): error "
                << e
                << " ("
                << strerror(e)
                << ") occurred trying to mark socket with SO_KEEPALIVE."
                << SNAP_LOG_SEND;
        }
    }
}


/** \brief Lets you know whether mark_done() was called.
 *
 * This function returns true if mark_done() was called on this connection.
 *
 * \return true if the connection is done (we want it to stop sending/receiving
 * as soon as possible).
 */
bool connection::is_done() const
{
    return f_done;
}


/** \brief Call once you are done with a connection.
 *
 * This function lets the connection know that you are done with it.
 * It is very important to call this function before you send the last
 * message. For example, with its permanent connection the snapbackend
 * tool does this:
 *
 * \code
 *      f_messenger->mark_done();
 *      f_messenger->send_message(stop_message);
 * \endcode
 *
 * The f_done flag is currently used in two situations by the main
 * system:
 *
 * \li write buffer is empty
 *
 * There are times when you send one or more last messages to a connection.
 * The write is generally buffered and will be processed whenever you
 * next come back in the run() loop.
 *
 * So one knows that the write (output) buffer is empty whenever one gets
 * its process_empty_buffer() callback called. At that point, the connection
 * can be removed from the communicator instance since we are done with
 * it. The default process_empty_buffer() does that for us whenever the
 * mark_done() function was called.
 *
 * \li HUP of a permanent connection
 *
 * When the f_done flag is set, the next HUP error is properly interpreted
 * as "we are done". Otherwise, a HUP is interpreted as a lost connection
 * and since a permanent connection is... permanent, it simply restarts the
 * connect process to reconnect to the server.
 *
 * \todo
 * Since we remove the connection on a process_empty_buffer(), maybe we
 * should have the has_output() as a virtual function and if it returns
 * true (which would be the default,) then we should remove the connection
 * immediately since it is already done? This may require a quick review
 * since in some cases we are not to remove the connection at all. But
 * this function could call the process_empty_buffer(), which can be
 * overridden so the developer can add its own callback to avoid the
 * potential side effect.
 *
 * \sa is_done()
 * \sa mark_not_done()
 */
void connection::mark_done()
{
    f_done = true;

    //if(!has_output())
    //{
    //    process_empty_buffer();
    //}
}


/** \brief Mark this connection as not done.
 *
 * In some cases you may want to mark a connection as done and later
 * restore it as not done.
 *
 * Specifically, this is used by the
 * tcp_blocking_client_message_connection class. When you call the
 * run() function, this mark_not_done() function gets called so in
 * effect you can re-enter the run() loop multiple times. Each time,
 * you have to call the mark_done() function to exit the loop.
 *
 * \sa is_done()
 * \sa mark_done()
 */
void connection::mark_not_done()
{
    f_done = false;
}


/** \brief This callback gets called whenever the connection times out.
 *
 * This function is called whenever a timeout is detected on this
 * connection. It is expected to be overwritten by your class if
 * you expect to use the timeout feature.
 *
 * The timer class is expected to always have a timer (although
 * the connection can temporarily be disabled) which triggers this
 * callback on a given periodicity.
 */
void connection::process_timeout()
{
}


/** \brief This callback gets called whenever the signal happened.
 *
 * This function is called whenever a certain signal (as defined in
 * your signal object) was detected while waiting for an
 * event.
 */
void connection::process_signal()
{
}


/** \brief This callback gets called whenever data can be read.
 *
 * This function is called whenever a socket has data that can be
 * read. For UDP, this means reading one packet. For TCP, it means
 * you can read at least one byte. To avoid blocking in TCP,
 * you must have called the non_blocking() function on that
 * connection, then you can attempt to read as much data as you
 * want.
 */
void connection::process_read()
{
}


/** \brief This callback gets called whenever data can be written.
 *
 * This function is called whenever a socket has space in its output
 * buffers to write data there.
 *
 * For UDP, this means writing one packet.
 *
 * For TCP, it means you can write at least one byte. To be able to
 * write as many bytes as you want, you must make sure to make the
 * socket non_blocking() first, then you can write as many bytes as
 * you want, although all those bytes may not get written in one
 * go (you may need to wait for the next call to this function to
 * finish up your write.)
 */
void connection::process_write()
{
}


/** \brief Sent all data to the other end.
 *
 * This function is called whenever a connection bufferized data
 * to be sent to the other end of the connection and that buffer
 * just went empty.
 *
 * Just at the time the function gets called, the buffer is empty.
 * You may refill it at that time.
 *
 * The callback is often used to remove a connection from the
 * communicator instance (i.e. just after we sent a last
 * message to the other end.)
 *
 * By default this function removes the connection from the
 * communicator instance if the mark_done() function was
 * called. Otherwise, it just ignores the message.
 */
void connection::process_empty_buffer()
{
    if(f_done)
    {
        SNAP_LOG_DEBUG
            << "socket "
            << get_socket()
            << " of connection \""
            << f_name
            << "\" was marked as done, removing in process_empty_buffer()."
            << SNAP_LOG_SEND;

        remove_from_communicator();
    }
}


/** \brief This callback gets called whenever a connection is made.
 *
 * A listening server receiving a new connection gets this function
 * called. The function is expected to create a new connection object
 * and add it to the communicator.
 *
 * \code
 *      // get the socket from the accept() function
 *      int const client_socket(accept());
 *      client_impl::pointer_t connection(std::make_shared<client_impl>(get_communicator(), client_socket));
 *      connection->set_name("connection created by server on accept()");
 *      get_communicator()->add_connection(connection);
 * \endcode
 */
void connection::process_accept()
{
}


/** \brief This callback gets called whenever an error is detected.
 *
 * If an error is detected on a socket, this callback function gets
 * called. By default the function removes the connection from
 * the communicator because such errors are generally non-recoverable.
 *
 * The function also logs an error message.
 */
void connection::process_error()
{
    // TBD: should we offer a virtual close() function to handle this
    //      case? because the get_socket() function will not return
    //      -1 after such errors...

    if(get_socket() == -1)
    {
        SNAP_LOG_DEBUG
            << "socket "
            << get_socket()
            << " of connection \""
            << f_name
            << "\" was marked as erroneous by the kernel or was closed (-1)."
            << SNAP_LOG_SEND;
    }
    else
    {
        // this happens all the time, so we changed the WARNING into a
        // DEBUG, too much logs by default otherwise...
        //
        SNAP_LOG_DEBUG
            << "socket "
            << get_socket()
            << " of connection \""
            << f_name
            << "\" was marked as erroneous by the kernel."
            << SNAP_LOG_SEND;
    }

    remove_from_communicator();
}


/** \brief This callback gets called whenever a hang up is detected.
 *
 * When the remote connection (client or server) closes a socket
 * on their end, then the other end is signaled by getting this
 * callback called.
 *
 * Note that this callback will be called after the process_read()
 * and process_write() callbacks. The process_write() is unlikely
 * to work at all. However, the process_read() may be able to get
 * a few more bytes from the remove connection and act on it.
 *
 * By default a connection gets removed from the communicator
 * when the hang up even occurs.
 */
void connection::process_hup()
{
    // TBD: should we offer a virtual close() function to handle this
    //      case? because the get_socket() function will not return
    //      -1 after such errors...

    SNAP_LOG_DEBUG
        << "socket "
        << get_socket()
        << " of connection \""
        << f_name
        << "\" hang up."
        << SNAP_LOG_SEND;

    remove_from_communicator();
}


/** \brief This callback gets called whenever an invalid socket is detected.
 *
 * I am not too sure at the moment when we are expected to really receive
 * this call. How does a socket become invalid (i.e. does it get closed
 * and then the user still attempts to use it)? In most cases, this should
 * probably never happen.
 *
 * By default a connection gets removed from the communicator
 * when the invalid even occurs.
 *
 * This function also logs the error.
 */
void connection::process_invalid()
{
    // TBD: should we offer a virtual close() function to handle this
    //      case? because the get_socket() function will not return
    //      -1 after such errors...

    SNAP_LOG_ERROR
        << "socket of connection \""
        << f_name
        << "\" was marked as invalid by the kernel."
        << SNAP_LOG_SEND;

    remove_from_communicator();
}


/** \brief Callback called whenever this connection gets added.
 *
 * This function gets called whenever this connection is added to
 * the communicator object. This gives you the opportunity
 * to do additional initialization before the run() loop gets
 * called or re-entered.
 */
void connection::connection_added()
{
}


/** \brief Callback called whenever this connection gets removed.
 *
 * This callback gets called after it got removed from the
 * communicator object. This gives you the opportunity
 * to do additional clean ups before the run() loop gets
 * re-entered.
 */
void connection::connection_removed()
{
}


/** \fn int connection::get_socket() const = 0
 * \brief Retrieve this connection socket.
 *
 * This function returns the socket of the connection which is a file
 * descriptor.
 *
 * A connection is expected to create a socket at the time it gets created.
 * It can use that socket until it gets closed. After it gets closed, the
 * function returns -1.
 *
 * \return The socket connection or -1 if the connection is closed.
 */



} // namespace ed
// vim: ts=4 sw=4 et

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
 * \brief Implementation of the Thread Done Signal class.
 *
 * When you create threads, it is often useful to know once a thread
 * is done via a signal (i.e. without having to be blocked joining
 * the thread).
 */


// self
//
#include    "eventdispatcher/thread_done_signal.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <cstring>


// C lib
//
#include    <fcntl.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initializes the "thread done signal" object.
 *
 * To know that a thread is done, we need some form of signal that the
 * poll() can wake up on. For the purpose we currently use a pipe because
 * a full socket is rather slow to setup compare to a simple pipe.
 *
 * To use this signal, one creates a Thread Done Signal and adds the
 * new connection to the Snap Communicator object. Then when the thread
 * is done, the thread calls the thread_done() function. That will wake
 * up the main process.
 *
 * The same thread_done_signal class can be used multiple times,
 * but only by one thread at a time. Otherwise you cannot know which
 * thread sent the message and by the time you attempt a join, you may
 * be testing the wrong thread (either that or you need another type
 * of synchronization mechanism.)
 *
 * \code
 *      class thread_done_impl
 *          : ed::thread_done_signal::thread_done_signal
 *      {
 *          ...
 *          void process_read()
 *          {
 *              // this function gets called when the thread is about
 *              // to exit or has exited; since the write to the pipe
 *              // happens before the thread really exited, but should
 *              // be near the very end, you should be fine calling the
 *              // cppthread::stop() function to join with it very
 *              // quickly.
 *              ...
 *          }
 *          ...
 *      };
 *
 *      // in the main thread
 *      ed::thread_done_signal::pointer_t s(std::shared_ptr<thread_done_impl>());
 *      ed::communicator::instance()->add_connection(s);
 *
 *      // create thread... and make sure the thread has access to 's'
 *      ...
 *
 *      // in the thread, before exiting we do:
 *      s->thread_done();
 *
 *      // around here, in the timeline, the process_read() function
 *      // gets called
 * \endcode
 *
 * \todo
 * Change the implementation to use eventfd() instead of pipe2().
 * Pipes are using more resources and are slower to use than
 * an eventfd.
 */
thread_done_signal::thread_done_signal()
{
    if(pipe2(f_pipe, O_NONBLOCK | O_CLOEXEC) != 0)
    {
        // pipe could not be created
        //
        throw event_dispatcher_initialization_error("somehow the pipes used to detect the death of a thread could not be created.");
    }
}


/** \brief Close the pipe used to detect the thread death.
 *
 * The destructor is expected to close the pipe opned in the constructor.
 */
thread_done_signal::~thread_done_signal()
{
    close(f_pipe[0]);
    close(f_pipe[1]);
}


/** \brief Tell that this connection expects incoming data.
 *
 * The thread_done_signal implements a signal that a secondary
 * thread can trigger before it quits, hence waking up the main
 * thread immediately instead of polling.
 *
 * \return The function returns true.
 */
bool thread_done_signal::is_reader() const
{
    return true;
}


/** \brief Retrieve the "socket" of the thread done signal object.
 *
 * The Thread Done Signal is implemented using a pair of pipes.
 * One of the pipes is returned as the "socket" and the other is
 * used to "write the signal".
 *
 * \return The signal "socket" to listen on with poll().
 */
int thread_done_signal::get_socket() const
{
    return f_pipe[0];
}


/** \brief Read the byte that was written in the thread_done().
 *
 * This function implementation reads one byte that was written by
 * thread_done() so the pipes can be reused multiple times.
 */
void thread_done_signal::process_read()
{
    char c(0);
    if(read(f_pipe[0], &c, sizeof(char)) != sizeof(char))
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "an error occurred while reading from a pipe used to know whether a thread is done (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;
    }
}


/** \brief Send the signal from the secondary thread.
 *
 * This function writes one byte in the pipe, which has the effect of
 * waking up the poll() of the main thread. This way we avoid having
 * to lock the file.
 *
 * The thread is expected to call this function just before it returns.
 */
void thread_done_signal::thread_done()
{
    char c(1);
    if(write(f_pipe[1], &c, sizeof(char)) != sizeof(char))
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "an error occurred while writing to a pipe used to know whether a thread is done (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

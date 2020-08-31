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
 * \brief Implementation of the Signal class.
 *
 * The Signal class listens for Unix signals to happen. This wakes us
 * up when the signal happens.
 */

// self
//
#include    "eventdispatcher/signal.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <iostream>


// C lib
//
#include    <string.h>
#include    <signal.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{


namespace
{



/** \brief The array of signals handled by snap_signal objects.
 *
 * This map holds a list of signal handlers. You cannot register
 * the same signal more than once so this map is used to make
 * sure that each signal is unique.
 *
 * \todo
 * We may actually want to use a sigset_t object and just set
 * bits and remove 
 *
 * \note
 * The pointer to the signal object is a bare pointer
 * for in part because we cannot use a smart pointer in
 * a constructor where we add the signal to this map. Also
 * at this time that pointer does not get used so it could
 * as well have been a boolean.
 *
 * \bug
 * Having a global list this means signal objects can't safely
 * be created before main() gets called.
 */
sigset_t                            g_signal_handlers = sigset_t();



}
// no name namespace




/** \brief Initializes the signal object.
 *
 * This function initializes the signal object with the specified
 * \p posix_signal which represents a POSIX signal such as SIGHUP,
 * SIGTERM, SIGUSR1, SIGUSR2, etc.
 *
 * The signal automatically gets masked out. This allows us to
 * unmask the signal only when we are ready to call ppoll() and
 * thus not have the signal break any of our normal user code.
 *
 * The ppoll() function unblocks all the signals that you listen
 * to (i.e. for each snap_signal object you created.) The run()
 * loop ends up calling your process_signal() callback function.
 *
 * Note that the snap_signal callback is called from the normal user
 * environment and not directly from the POSIX signal handler.
 * This means you can call any function from your callback.
 *
 * \note
 * IMPORTANT: Remember that POSIX signals stop your code at a 'breakable'
 * point which in many circumstances can create many problems unless
 * you make sure to mask signals while doing work. For example, you
 * could end up with a read() returning an error when the file you
 * are reading has absolutely no error but a dude decided to signal
 * you with a 'kill -HUP 123'...
 *
 * \code
 *      {
 *          // use an RAII masking mechanism
 *          mask_posix_signal mask();
 *
 *          // do your work (i.e. read/write/etc.)
 *          ...
 *      }
 * \endcode
 *
 * \par
 * The best way in our processes will be to block all signals except
 * while poll() is called (using ppoll() for the feat.)
 *
 * \note
 * By default the constructor masks the specified \p posix_signal and
 * it does not restore the signal on destruction. If you want the
 * signal to be unmasked on destruction (say to restore the default
 * functioning of the SIGINT signal,) then make sure to call the
 * unblock_signal() function right after you create your connection.
 *
 * \warning
 * The the signal gets masked by this constructor. If you want to make
 * sure that most of your code does not get affected by said signal,
 * make sure to create your snap_signal object early on or mask those
 * signals beforehand. Otherwise the signal could happen before it
 * gets masked. Initialization of your process may not require
 * protection anyway.
 *
 * \bug
 * You should not use signal() and setup a handler for the same signal.
 * It will not play nice to have both types of signal handlers. That
 * being said, we my current testing (as of Ubuntu 16.04), it seems
 * to work just fine..
 *
 * \exception snap_communicator_initialization_error
 * Create multiple snap_signal() with the same posix_signal parameter
 * is not supported and this exception is raised whenever you attempt
 * to do that. Remember that you can have at most one snap_communicator
 * object (hence the singleton.)
 *
 * \exception snap_communicator_runtime_error
 * The signalfd() function is expected to create a "socket" (file
 * descriptor) listening for incoming signals. If it fails, this
 * exception is raised (which is very similar to other socket
 * based connections which throw whenever a connection cannot
 * be achieved.)
 *
 * \param[in] posix_signal  The signal to be managed by this snap_signal.
 */
signal::signal(int posix_signal)
    : f_signal(posix_signal)
{
    int const r(sigismember(&g_signal_handlers, f_signal));
    if(r != 0)
    {
        if(r == 1)
        {
            // this could be fixed, but probably not worth the trouble...
            //
            throw event_dispatcher_initialization_error("the same signal cannot be created more than once in your entire process.");
        }

        // f_signal is not considered valid by this OS
        //
        throw event_dispatcher_initialization_error("posix_signal (f_signal) is not a valid/recognized signal number.");
    }

    // create a mask for that signal
    //
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, f_signal); // ignore error, we already know f_signal is valid

    // first we block the signal
    //
    if(sigprocmask(SIG_BLOCK, &set, nullptr) != 0)
    {
        throw event_dispatcher_runtime_error("sigprocmask() failed to block signal.");
    }

    // second we create a "socket" for the signal (really it is a file
    // descriptor manager by the kernel)
    //
    f_socket = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
    if(f_socket == -1)
    {
        int const e(errno);
        std::string err("signalfd() failed to create a signal listener for signal ");
        err += std::to_string(f_signal);
        err += " (errno: ";
        err += std::to_string(e);
        err += " -- ";
        err += strerror(e);
        err += ").";
        SNAP_LOG_ERROR << err << SNAP_LOG_SEND;
        throw event_dispatcher_runtime_error(err);
    }

    // mark this signal as in use
    //
    sigaddset(&g_signal_handlers, f_signal); // ignore error, we already know f_signal is valid
}


/** \brief Restore the signal as it was before you created a snap_signal.
 *
 * The destructor is expected to restore the signal to what it was
 * before you create this snap_signal. Of course, if you created
 * other signal handlers in between, it will not work right since
 * this function will destroy your handler pointer.
 *
 * To do it right, it has to be done in order (i.e. set handler 1, set
 * handler 2, set handler 3, remove handler 3, remove handler 2, remove
 * handler 1.) We do not guarantee anything at this level!
 */
signal::~signal()
{
    close();
}


/** \brief Tell that this connection is listening on a Unix signal.
 *
 * The snap_signal implements the signal listening feature. We use
 * a simple flag in the virtual table to avoid a more expansive
 * dynamic_cast<>() is a loop that goes over all the connections
 * you have defined.
 *
 * \return The base implementation returns false.
 */
bool signal::is_signal() const
{
    return true;
}


/** \brief Retrieve the "socket" of the signal object.
 *
 * Signal objects have a socket (file descriptor) assigned to them
 * using the signalfd() function.
 *
 * \note
 * You should not override this function since there is no other
 * value it can return.
 *
 * \return The signal socket to listen on with poll().
 */
int signal::get_socket() const
{
    return f_socket;
}


/** \brief Retrieve the PID of the child process that just emitted SIGCHLD.
 *
 * This function returns the process identifier (pid_t) of the child that
 * just sent us a SIGCHLD Unix signal.
 *
 * \exception snap_communicator_runtime_error
 * This exception is raised if the function gets called before any signal
 * ever occurred.
 *
 * \return The process identifier (pid_t) of the child that died.
 */
pid_t signal::get_child_pid() const
{
    if(f_signal_info.ssi_signo == 0)
    {
        throw event_dispatcher_runtime_error("snap_signal::get_child_pid() called before any signal ever occurred.");
    }

    return f_signal_info.ssi_pid;
}


/** \brief Get a copy of the current signal.
 *
 * Whenever we read a signal, the data is saved in the internal f_signal_info
 * structure. You can access that structure and its content using this
 * function.
 *
 * The structure is return as read-only. You should not modify it. If you
 * need to do so, make a copy.
 *
 * The structre remains valid until your process_signal() function returns.
 * If you need the info after, make sure to make a copy.
 *
 * \return A direct pointer to the signal info in the object.
 */
signalfd_siginfo const * signal::get_signal_info() const
{
    return &f_signal_info;
}


/** \brief Processes this signal.
 *
 * This function reads the signal "socket" for all the signal received
 * so far.
 *
 * For each instance found in the signal queue, the process_signal() gets
 * called.
 */
void signal::process()
{
    // loop any number of times as required
    // (or can we receive a maximum of 1 such signal at a time?)
    //
    while(f_socket != -1)
    {
        int const r(read(f_socket, &f_signal_info, sizeof(f_signal_info)));
        if(r == sizeof(f_signal_info))
        {
            process_signal();
        }
        else
        {
            if(r == -1)
            {
                // if EAGAIN then we are done as expected, any other error
                // is logged
                //
                if(errno != EAGAIN)
                {
                    int const e(errno);
                    SNAP_LOG_ERROR
                        << "an error occurred while reading from the signalfd() file descriptor. (errno: "
                        << e
                        << " -- "
                        << strerror(e)
                        << ")."
                        << SNAP_LOG_SEND;
                }
            }
            else
            {
                // what to do? what to do?
                SNAP_LOG_ERROR
                    << "reading from the signalfd() file descriptor did not return the expected size. (got "
                    << r
                    << ", expected "
                    << sizeof(f_signal_info)
                    << ")"
                    << SNAP_LOG_SEND;
            }
            break;
        }
    }
}


/** \brief Close the signal file descriptor.
 *
 * This function closes the file descriptor and, if you called the
 * unblock_signal_on_destruction() function, it also restores the
 * signal (unblocks it.)
 *
 * After this call, the connection is pretty much useless (although
 * you could still use it as a timer.) You cannot reopen the signal
 * file descriptor once closed. Instead, you have to create a new
 * connection.
 */
void signal::close()
{
    if(f_socket != -1)
    {
        ::close(f_socket);
        f_socket = -1;

        sigdelset(&g_signal_handlers, f_signal);     // ignore error, we already know f_signal is valid

        if(f_unblock)
        {
            // also unblock the signal
            //
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, f_signal); // ignore error, we already know f_signal is valid
            if(sigprocmask(SIG_UNBLOCK, &set, nullptr) != 0)
            {
                // we cannot throw in a destructor and in most cases this
                // happens in the destructor...
                //throw snap_communicator_runtime_error("sigprocmask() failed to block signal.");

                int const e(errno);
                SNAP_LOG_FATAL
                    << "an error occurred while unblocking signal "
                    << f_signal
                    << " with sigprocmask(). (errno: "
                    << e
                    << " -- "
                    << strerror(e)
                    << SNAP_LOG_SEND;
                std::cerr << "sigprocmask() failed to unblock signal." << std::endl;

                std::terminate();
            }
        }
    }
}


/** \brief Unmask a signal that was part of a connection.
 *
 * If you remove a snap_signal connection, you may want to restore
 * the mask functionality. By default the signal gets masked but
 * it does not get unmasked.
 *
 * By calling this function just after creation, the signal gets restored
 * (unblocked) whenever the snap_signal object gets destroyed.
 */
void signal::unblock_signal_on_destruction()
{
    f_unblock = true;
}



} // namespace ed
// vim: ts=4 sw=4 et

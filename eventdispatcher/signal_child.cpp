// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the Signal class.
 *
 * The Signal class listens for Unix signals to happen. This wakes us
 * up when the signal happens.
 */

// self
//
#include    "eventdispatcher/signal_child.h"

#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// cppthread
//
#include    <cppthread/guard.h>


// snapdev
//
#include    <snapdev/not_used.h>
#include    <snapdev/safe_variable.h>


// C++
//
#include    <iostream>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{


namespace
{



/** \brief This singleton is saved here once created.
 *
 * The signal_child object is a singleton. It is created the first time
 * you call the get_instance() function. It is used to handle the SIGCHLD
 * signal with any number of children from any library or function you
 * are running. This allows for one location to manage that specific
 * signal, but many location to handle the death of a child process.
 */
signal_child::pointer_t    g_signal_child = signal_child::pointer_t();



}
// no name namespace




/** \brief Initialize a child_status object.
 *
 * This constructor saves the wait info of the last waitid() call to this
 * object. It represents the current status of the child process.
 *
 * You can listen to changes to the status of a process. If the process
 * is still running, then you get a reply which says the child process
 * is not exited, signaled, or stopped. You can decide on which signal
 * your callback gets called.
 *
 * \param[in] info  The signal information as received from the system.
 */
child_status::child_status(siginfo_t const & info)
    : f_info(info)
{
}


/** \brief Return the PID of the concerned child.
 *
 * This function gives you the PID of the child that died. This is
 * particularly useful if you handle multiple children in the
 * same callback.
 *
 * \return The child that just received a status change.
 */
pid_t child_status::child_pid() const
{
    return f_info.si_pid;
}


/** \brief Return the UID of the user that was running the child.
 *
 * This function returns the user identifier of the real user who
 * was running the child process.
 *
 * \return The child process user identifier.
 */
uid_t child_status::child_uid() const
{
    return f_info.si_uid;
}


/** \brief Whether the status means the process is still up and running.
 *
 * This function returns true if the process did not exit, was not
 * signaled, and was not stopped. In all other circumstances, this
 * function returns false.
 *
 * \return true if the process is still running.
 */
bool child_status::is_running() const
{
    return !is_exited()
        && !is_signaled()
        && !is_stopped();
}


/** \brief The process terminated cleanly, with a call to exit().
 *
 * This function returns true if the child process ended by calling
 * the exit() function.
 *
 * You can further call the exit_code() function to retrieve the
 * exit code returned by that process (a number between 0 and 255).
 *
 * \return true if the process cleanly exited.
 */
bool child_status::is_exited() const
{
    return f_info.si_code == CLD_EXITED;
}


/** \brief Whether the process terminated because of a signal.
 *
 * After receiving a signal which kills the process, this function returns
 * true. In most cases, we do not expect children to exit with a signal,
 * but if that happens, this is how you can detect the matter.
 *
 * You can further call the is_core_dumped() to detect whether a coredump
 * was generated and terminate_signal() to get the signal number that
 * terminated this process.
 *
 * \return true if the process was terminated because of a signal.
 */
bool child_status::is_signaled() const
{
    return f_info.si_code == CLD_KILLED || f_info.si_code == CLD_DUMPED;
}


/** \brief Whether a core dumped was generated.
 *
 * When a process receives a signal, it can be asked to generate a core
 * dump. In most cases, the core dump size is set to 0 so nothing actually
 * gets saved to disk. So this flag may be true, but it doesn't mean you
 * will find an actual coredump file in your folder.
 *
 * \return true if the process was signaled and a core dump generated.
 */
bool child_status::is_core_dumped() const
{
    return f_info.si_code == CLD_DUMPED;
}


/** \brief The process received a signal to stop.
 *
 * A SIGSTOP or a trace signal (i.e. as in a debugger). The process is
 * still in memory but it is not currently running.
 *
 * You can further call the stop_signal() to know the signal used to
 * stop this process.
 *
 * \return true if the process stopped.
 */
bool child_status::is_stopped() const
{
    // TODO: have a separate is_trapped()
    return f_info.si_code == CLD_STOPPED || f_info.si_code == CLD_TRAPPED;
}


/** \brief The process was sent the SIGCONT signal.
 *
 * The process was previously stopped by a SIGSTOP or a trap or some other
 * similar signal. It was then continued. This signals the continuation.
 *
 * \return true if the process was signaled to continue.
 */
bool child_status::is_continued() const
{
    return f_info.si_code == CLD_CONTINUED;
}


/** \brief Transform the status in a mask.
 *
 * This function transforms this status in a mask. This is used to know
 * which callback to call whenever an event occurs.
 *
 * The function returns 0 if the current status is not properly understood.
 *
 * \return The mask representing this child status.
 */
flag_t child_status::status_mask() const
{
    if(is_running())
    {
        return SIGNAL_CHILD_FLAG_RUNNING;
    }

    if(is_exited())
    {
        return SIGNAL_CHILD_FLAG_EXITED;
    }

    if(is_signaled())
    {
        return SIGNAL_CHILD_FLAG_SIGNALED;
    }

    if(is_stopped())
    {
        return SIGNAL_CHILD_FLAG_STOPPED;
    }

    if(is_continued())
    {
        return SIGNAL_CHILD_FLAG_CONTINUED;
    }

    // invalid / unknown / not understood status
    //
    return 0;
}


/** \brief The exit code of the child process.
 *
 * This function return the exit code as returned by the child process by
 * returning from the main() function or explicitly calling one of the
 * exit() functions.
 *
 * The function returns -1 if the process did not exit normally or is
 * still running. Note that the exit() function can only return a number
 * between 0 and 255.
 *
 * \return The exit code.
 */
int child_status::exit_code() const
{
    if(is_exited())
    {
        return f_info.si_status;
    }
    return -1;
}


/** \brief The signal that terminated the process.
 *
 * This function returns the signal that terminated this process if
 * it was terminated by a signal.
 *
 * If the process was not terminated by a signal (or is still running)
 * then the function returns -1.
 *
 * \return The signal that terminated the concerned child process.
 */
int child_status::terminate_signal() const
{
    if(is_signaled())
    {
        return f_info.si_status;
    }
    return -1;
}


/** \brief Return the signal used to stop the process.
 *
 * This function returns the signal number used to stop the process.
 *
 * If the process is not stopped, then the function returns -1.
 *
 * \return The signal used to stop the process.
 */
int child_status::stop_signal() const
{
    if(is_stopped())
    {
        return f_info.si_status;
    }

    return -1;
}









/** \brief Initializes the signal_child object.
 *
 * This signal_child object is a singleton. It is used to listen on the
 * SIGCHLD signal via a ed::signal connection. You can listen for the
 * death of your child by listening for its pid_t. It will get called
 * on various events (running, exited, signaled, stopped, continued).
 */
signal_child::signal_child()
    : signal(SIGCHLD)
{
}


/** \brief Restore the SIGCHLD signal as it was.
 *
 * Since the signal_child is a singleton, this function only gets called
 * when you exit your process. It is expected that all the children you
 * were listening on died before you call this function.
 */
signal_child::~signal_child()
{
}


/** \brief Get the pointer to the signal_child singleton.
 *
 * This function retrieves the pointer to the signal_child singleton.
 *
 * The first time you call the function, the singleton gets created.
 *
 * \return The singleton pointer.
 */
signal_child::pointer_t signal_child::get_instance()
{
    cppthread::guard lock(*cppthread::g_system_mutex);

    if(g_signal_child == nullptr)
    {
        // WARNING: can't use std::make_shared<> because constructor is
        //          private (since we have a singleton)
        //
        g_signal_child.reset(new signal_child());
    }

    return g_signal_child;
}


/** \brief Add this connection to the communicator.
 *
 * \note
 * You should not call this function. It automatically gets called when
 * you add a listener (see add_listener() in this class). After all, you
 * do not need to listen to anything until you ask for it and similarly
 * the remove gets called automatically when the listener gets removed
 * (which again is automatic once the child dies).
 *
 * This function adds this connection to the communicator. This function can
 * be called any number of times. It will increase a counter which will
 * then be decremented by the remove_connection().
 *
 * This is used because the communicator::add_connection() will not add the
 * signal_child connection more than once because many different functions
 * and libraries may need to add it and these would not know whether to
 * add or remove the connection and in the end we want it to be properly
 * accounted for.
 *
 * You actually will not be able to add it directly using the
 * communicator::add_connection(). It will throw if you try to do that.
 * Instead, you must call this function.
 */
void signal_child::add_connection()
{
    if(f_count == 0)
    {
        // add the connection to the communicator
        //
        snapdev::safe_variable safe(f_adding_to_communicator, true, false);
        ed::communicator::instance()->add_connection(shared_from_this());
    }
    ++f_count;
}


/** \brief Remove the connection from the communicator.
 *
 * \note
 * You do not need to call this function. The listener callback function
 * gets called and assuming the child died (i.e. a child that received a
 * signal that killed it or one that called _exit() to terminate) this
 * function gets called automatically.
 *
 * You must call this function to remove the signal child for each time you
 * added it with the corresponding add_connection().
 *
 * This function is used along the add_connection() because the basic
 * add & remove functions of the communicator do not allow you to add
 * the same connection more than once (which makes sense), yet the signal
 * may be added and removed by many different systems. The means it would
 * be unlikely that you would know of all the adds and all the removes in
 * one place.
 *
 * \exception count_mismatch
 * The remove_connection() function must be called exactly once for each
 * call to the add_connection() function. If called more than this many
 * times, then this exception is raised.
 */
void signal_child::remove_connection()
{
    if(f_count == 0)
    {
        throw count_mismatch(
            "the signal_child::remove_connection() was called more times"
            " than the add_connection()");
    }

    --f_count;
    if(f_count == 0)
    {
        // remove the connection to the communicator
        //
        snapdev::safe_variable safe(f_removing_to_communicator, true, false);
        ed::communicator::instance()->remove_connection(shared_from_this());
    }
}


/** \brief Process the SIGCHLD signal.
 *
 * This function process the SIGCHLD signal. Note that the function is
 * expected to be called once per SIGCHLD signaled, however, if several
 * children die \em simultaneously, then it would not work to process
 * only one child at a time. For this reason, we instead process all the
 * children that have died in one go and if we get called additional times
 * nothing happens.
 */
void signal_child::process_signal()
{
    for(;;)
    {
        // Note: to retrieve the rusage() of the process, we could use our
        //       process_info, however, that has to be done while the
        //       process is still a zombie... if the callback wants to
        //       do that, then it is possible since the call here uses
        //       the WNOWAIT (which means the zombie stays until later)
        //
        siginfo_t info = {};
        int const r(waitid(
                  P_ALL
                , 0
                , &info
                , WEXITED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT));
        if(r != 0)
        {
            // if there are no more children, we get an ECHILD error
            // and we can ignore those
            //
            if(errno != ECHILD)
            {
                int const e(errno);
                SNAP_LOG_ERROR
                    << "waitid() failed to wait for a child: "
                    << e
                    << ", "
                    << strerror(e)
                    << SNAP_LOG_SEND;
            }
            return;
        }

        child_status const status(info);
        if(status.child_pid() == 0)
        {
            // no more state changes
            //
            break;
        }

        callback_t::list_t listeners;
        {
            cppthread::guard lock(f_mutex);
            listeners = f_listeners;
        }

        flag_t const mask(status.status_mask());
        for(auto & listener : listeners)
        {
            if(listener.f_child == status.child_pid()
            && (listener.f_flags & mask) != 0)
            {
                listener.f_callback(status);
            }
        }

        if(status.is_exited()
        || status.is_signaled())
        {
            // release the zombie, we're done
            //
            siginfo_t ignore = {};
            snapdev::NOT_USED(waitid(P_PID, status.child_pid(), &ignore, WEXITED));

            remove_listener(status.child_pid());
        }
    }
}


/** \brief The connection was added to the communicator.
 *
 * The connection was added, make sure it was by us (through our own
 * add_connection() function).
 *
 * \warning
 * The test in this function works only for the very first connection.
 * After that, the communicator prevents this callback from happening.
 *
 * \exception runtime_error
 * The signal_child connection must be added by the add_connection() function.
 * If you directly call the communicator::add_connection(), then this
 * exception is raised.
 */
void signal_child::connection_added()
{
    if(!f_adding_to_communicator)
    {
        throw runtime_error(
            "it looks like you directly called communicator::add_connection()"
            " with the signal_child connection. This is not allowed. Make sure"
            " to call the signal_child::add_connection() instead.");
    }
}


/** \brief The connection was removed from the communicator.
 *
 * The connection was removed, make sure it was by us (through our own
 * remove_connection() function).
 *
 * \exception runtime_error
 * The signal_child connection must be added by the add_connection() function.
 * If you directly call the communicator::add_connection(), then this
 * exception is raised.
 */
void signal_child::connection_removed()
{
    if(!f_removing_to_communicator)
    {
        throw runtime_error(
            "it looks like you directly called communicator::remove_connection()"
            " with the signal_child connection. This is not allowed. Make sure"
            " to call the signal_child::remove_connection() instead.");
    }
}


/** \brief Add a listener function.
 *
 * This function adds a listener so when a SIGCHLD occurs with the specified
 * \p child, the \p callback gets called.
 *
 * You can further define which signals you are interested in. In most
 * likelihood, only the ed::SIGNAL_CHILD_FLAG_EXITED and the
 * ed::SIGNAL_CHILD_FLAG_SIGNALED are going to be useful (i.e. to get
 * called when the process dies).
 *
 * At the time your \p callback function is called, the process is still
 * up (as a zombie). This gives you the opportunity to gather information
 * about the process. You can do so with the process_info class using
 * the callback child_info::child_pid() function to get the necessary
 * process identifier. (TODO: test that this is indeed true)
 *
 * The function can be called multiple times with the same child PID to
 * add multiple callbacks (useful if you vary the mask parameter).
 *
 * This function automatically calls the add_connection() function any
 * time it succeeeds in adding a new child/callback listener.
 *
 * \exception invalid_parameter
 * The mask cannot be set to zero, the child identifier must be positive,
 * the callback pointer cannot be nullptr.
 *
 * \param[in] child  The process identifier (pid_t) of the child to listen on.
 * \param[in] callback  A function pointer to call when the event occurs.
 * \param[in] mask  A mask representing the events you are interested in.
 */
void signal_child::add_listener(
          pid_t child
        , func_t callback
        , flag_t mask)
{
    if(child <= 0)
    {
        throw invalid_parameter(
              "the child parameter must be a valid pid_t (not "
            + std::to_string(child)
            + ")");
    }
    if(callback == nullptr)
    {
        throw invalid_parameter("callback cannot be nullptr");
    }
    if(mask == 0)
    {
        throw invalid_parameter("mask cannot be set to zero");
    }

    cppthread::guard lock(f_mutex);
    f_listeners.emplace(f_listeners.end(), callback_t{ child, callback, mask });
    add_connection();
}


/** \brief Remove all the listeners for a specific child.
 *
 * This function is the converse of the add_listener(). It is used to
 * remove a listener from the list maintained by the signal_child
 * singleton.
 *
 * This function automatically gets called whenever the signal_child
 * detects the death of a child and finds a corresponding listener.
 *
 * Further, this function automatically calls the remove_connection()
 * function when it indeeds removes the specifed \p child. If found
 * more than once, then it gets called once for each instance.
 *
 * \warning
 * All the listener that use the specified \p child parameter are
 * removed from the list of listeners.
 *
 * \note
 * Whenever you create a child with fork(), make sure to add a listener
 * right then before returning to the communicator::run() loop. That
 * way everything happens in the right order. Although the functions
 * handling the listener are thread safe, a fork() is not. So you
 * should not use both together making everything very safe.
 *
 * \param[in] child  The child for which the listener has to be removed.
 */
void signal_child::remove_listener(pid_t child)
{
    cppthread::guard lock(f_mutex);

    for(auto it(f_listeners.begin()); it != f_listeners.end(); )
    {
        if(it->f_child == child)
        {
            it = f_listeners.erase(it);
            remove_connection();
        }
        else
        {
            ++it;
        }
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

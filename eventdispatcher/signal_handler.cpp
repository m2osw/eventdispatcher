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
 * \brief Implementation of the Signal Handler class.
 *
 * The Signal Handler class is used to make sure that we get a log entry in
 * case a terminal signal happens. We can also use this class to ignore
 * certain signals and get callbacks called. In many cases, our services want
 * to do that with many signals and that code is pretty much always the same.
 *
 * The best is to add the signal_handler to your main() function like
 * so:
 *
 * \code
 *     #include <eventdispatcher/signal_handler.h>
 *
 *     int main(int argc, char * argv[])
 *     {
 *         ed::signal_handler::create_instance();
 *         ...
 *     }
 * \endcode
 *
 * This is sufficient to get all the events of interest captured and
 * reported with a stack trace in your logs (the eventdispatcher makes
 * use of the snaplogger for that purpose). Note that the function returns
 * a pointer to the signal_handler object, so you can save that pointer
 * and make tweaks immediately after (see examples of tweaks below).
 *
 * Note that it is possible to call ed::signal_handler::get_instance()
 * and never call the ed::signal_handler::create_instance() function.
 * However, the create function will setup defaults in the handler which
 * makes it easy to start with the expected state.
 *
 * In sub-functions, you may tweak the setup by doing various calls such as:
 *
 * \code
 *     ed::signal_handler::get_instance()->add_terminal_signals(ed::signal_handler::SIGNAL_CHILD);
 * \endcode
 *
 * \note
 * You can also add that terminal signal to the mask of the create_handler()
 * function. For example, you may want to add SIGNAL_TERMINATE (SIGTERM),
 * SIGNAL_INTERRUPT (Ctrl-C) and SIGNAL_QUIT to the list of terminal signals.
 * At the same time, those are expected termination signals but if you have
 * a TCP controller connection and can send a QUIT message, then that message
 * should be used and receiving those additional signals could be viewed as
 * an unexpected event. For that reason, we have the EXTENDED_SIGNAL_TERMINAL
 * which includes those additional three signals.
 *
 * Now if one of your process children dies, you will die too.
 *
 * Also, if you have an object that deals with pipes or sockets and you do
 * not want to receive the SIGPIPE, you can do:
 *
 * \code
 *     ed::signal_handler::get_instance()->add_ignored_signals(ed::signal_handler::SIGNAL_PIPE);
 * \endcode
 *
 * \note
 * You can also add that ignored signal to the mask of the create_handler()
 * function.
 *
 * Finally, you may be interested to capture a signal such as the SIGUSR1
 * signal. You do that by first adding the signal as a terminal signal and
 * then by adding a callback which will return true (i.e. signal handled).
 *
 * \note
 * You may want to consider using a signal connection object instead
 * of a callback for such a flag. You can wait on those signals with a
 * poll() and you avoid the EINTR errors which are so difficult to deal
 * with in a very large piece of software (see eventdispatcher/signal.h).
 *
 * \code
 * bool handle_usr1(
 *            callback_id_t callback_id
 *          , int callback_sig
 *          , siginfo_t const * info
 *          , ucontext_t const * ucontext)
 * {
 *     ...handle USR1 signal...
 *     return true;
 * }
 *
 * ...
 *     signal_handler::pointer_t sh(ed::signal_handler::get_instance());
 *     sh->add_terminal_signals(ed::signal_handler::SIGNAL_USR1);
 *     sh->add_callback(1, ed::signal_handler::SIGNAL_USR1, &handle_usr1);
 * ...
 * \endcode
 *
 * \note
 * This class is thread safe. It will lock its own mutex before running
 * functions dealing with this class parameters. However, this may also
 * results in a deadlock whenever a signal occurs.
 */

// self
//
#include    "eventdispatcher/signal_handler.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// cppthread
//
#include    <cppthread/mutex.h>
#include    <cppthread/guard.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


// C++
//
#include    <iostream>


// C
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


/** \brief Signal number to signal name table.
 *
 * This table is used to transform a signal number in a name.
 *
 * \sa signal_handler::get_signal_name()
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr char const * g_signal_names[64] =
{
    [0]         = nullptr,
    [SIGHUP]    = "SIGHUP",
    [SIGINT]    = "SIGINT",
    [SIGQUIT]   = "SIGQUIT",
    [SIGILL]    = "SIGILL",
    [SIGTRAP]   = "SIGTRAP",
    [SIGABRT]   = "SIGABRT",
    [SIGBUS]    = "SIGBUS",
    [SIGFPE]    = "SIGFPE",
    [SIGKILL]   = "SIGKILL",
    [SIGUSR1]   = "SIGUSR1",
    [SIGSEGV]   = "SIGSEGV",
    [SIGUSR2]   = "SIGUSR2",
    [SIGPIPE]   = "SIGPIPE",
    [SIGALRM]   = "SIGALRM",
    [SIGTERM]   = "SIGTERM",
    [SIGSTKFLT] = "SIGSTKFLT",
    [SIGCHLD]   = "SIGCHLD",
    [SIGCONT]   = "SIGCONT",
    [SIGSTOP]   = "SIGSTOP",
    [SIGTSTP]   = "SIGTSTP",
    [SIGTTIN]   = "SIGTTIN",
    [SIGTTOU]   = "SIGTTOU",
    [SIGURG]    = "SIGURG",
    [SIGXCPU]   = "SIGXCPU",
    [SIGXFSZ]   = "SIGXFSZ",
    [SIGVTALRM] = "SIGVTALRM",
    [SIGPROF]   = "SIGPROF",
    [SIGWINCH]  = "SIGWINCH",
    [SIGPOLL]   = "SIGPOLL",
    [SIGPWR]    = "SIGPWR",
    [SIGSYS]    = "SIGSYS",
};
#pragma GCC diagnostic pop


/** \brief Set of signals for which we want to log a stack trace.
 *
 * Just knowing where a signal occurred is often a bit limited. Knowing
 * the call stack for 10 to 20 items is much more helpful. However, for
 * some signals, it's generally totally useless so we use a mask to know
 * which signals to log the stack trace for.
 */
signal_handler::signal_mask_t       g_show_stack = signal_handler::DEFAULT_SHOW_STACK;


/** \brief The allocated signal_handler instance.
 *
 * The get_instance() allocates this handler.
 *
 * \warning
 * If you want to call the create_instance() function, then it has to be
 * called before get_instance().
 */
signal_handler::pointer_t           g_signal_handler = signal_handler::pointer_t();



}
// no name namespace




/** \brief Initialize the signal handler class.
 *
 * This function sets all the signal action structures to a nullptr
 * and then it sets up the \p terminal and \p ignored signals as
 * defined by the corresponding masks.
 *
 * You add to the set of signals that are terminal and ignored
 * later with the add_terminal_signals() and the add_ignored_signals().
 *
 * You can remove from the set of signals that are terminal or ignored
 * by calling the remove_signals() function.
 *
 * This function is private. It gets called by the get_instance() function.
 * You may also want to use the create_instance() function the first time
 * you create an instance.
 */
signal_handler::signal_handler()
{
}


/** \brief Restore the signals.
 *
 * The signal handler destructor restores all the signals that it changed.
 *
 * \note
 * At this point, the destructor is never called since we use an instance
 * and we do not give a way to destroy it. Unloading the library would have
 * that effect, but that generally doesn't happen.
 */
signal_handler::~signal_handler()
{
    remove_all_signals();
    g_signal_handler = nullptr;
}


/** \brief Handy function used to create the signal handler instance.
 *
 * In many cases, you want to create the signal handler and then setup
 * the terminal signal, the ignored signals, and a callback. This function
 * does all of that for you in one go.
 *
 * \code
 *     int main(int argc, char * argv[])
 *     {
 *         ed::signal_handler::create_instance(
 *                     ed::signal_mask_t terminal = DEFAULT_SIGNAL_TERMINAL
 *                   , ed::signal_mask_t ignored = DEFAULT_SIGNAL_IGNORE
 *                   , SIGFPE
 *                   , handle_floating_point_errors);
 *         ...
 *     }
 * \endcode
 *
 * This function automatically calls the add_terminal_signals()
 * with the specified \p terminal parameter.
 *
 * It then calls the add_ignored_signals with the \p ignored parameter.
 *
 * If the \p sig parameter is set to a non-zero value then the
 * add_callback() gets called. In that case, the \p callback must
 * be properly defined.
 *
 * \param[in] terminal  Mask with the set of terminal signals.
 * \param[in] ignored  Mask with set of signals to be ignored.
 * \param[in] callback_id  An identifier to attach the callback with.
 * \param[in] callback_sig  Signal for which you want a callback.
 * \param[in] callback  The callback function to call.
 *
 * \return The pointer to the signal handler.
 */
signal_handler::pointer_t signal_handler::create_instance(
      signal_mask_t terminal
    , signal_mask_t ignored
    , id_t callback_id
    , int callback_sig
    , callback_t callback)
{
    cppthread::guard g(*cppthread::g_system_mutex);

    if(g_signal_handler != nullptr)
    {
        throw invalid_callback("signal_handler::create_instance() must be called once before signal_handler::get_instance() ever gets called.");
    }

    pointer_t handler(get_instance());

    handler->add_terminal_signals(terminal);
    handler->add_ignored_signals(ignored);

    if(callback_sig > 0)
    {
        handler->add_callback(callback_id, callback_sig, callback);
    }

    return handler;
}


/** \brief Returns the signal handler instance.
 *
 * This function creates an instance of the signal handler and returns
 * the pointer. The very first time, though, you probably want to call
 * the create_instance() function so as to automatically initialize the
 * class. You can also reprogram your own initialization.
 * This function can be called any number of times.
 *
 * \warning
 * If you have threads, make sure to call this function at least once
 * before you create a thread since it is not otherwise thread safe.
 * Actually, the whole class is not considered thread safe so you should
 * create and initialize it.
 *
 * \return A pointer to the signal handler.
 */
signal_handler::pointer_t signal_handler::get_instance()
{
    cppthread::guard g(*cppthread::g_system_mutex);

    if(g_signal_handler == nullptr)
    {
        g_signal_handler.reset(new signal_handler());
    }
    return g_signal_handler;
}


/** \brief Add a callback to the signal handler.
 *
 * This function adds a callback to the signal_handler object. Callbacks
 * get called whenever the specified \p sig is received.
 *
 * You can add any number of callbacks per signal.
 *
 * The \p id parameter is a number you define. It is useful really only
 * if you add the same callback multiple times with different identifiers
 * and in case you want to be able to call the remove_callback() function.
 * You can always use `0` in all other cases.
 *
 * If you set the \p sig parameter to 0, then it will match all the
 * signals received. In other words, that callback will be called whatever
 * the received signal is (i.e. _match any_).
 *
 * \param[in] id  The callback identifier.
 * \param[in] sig  The signal (i.e. SIGPIPE) to assign a callback to.
 * \param[in] callback  The user callback to call on \p sig signal.
 */
void signal_handler::add_callback(callback_id_t id, int sig, callback_t callback)
{
    if(static_cast<std::size_t>(sig) >= sizeof(f_signal_actions) / sizeof(f_signal_actions[0]))
    {
        throw invalid_signal(
                  "signal_handler::add_callback() called with invalid signal number "
                + std::to_string(sig));
    }

    if(callback == nullptr)
    {
        throw invalid_callback("signal_handler::add_callback() called with nullptr as the callback.");
    }

    cppthread::guard g(f_mutex);

    f_callbacks.push_back(signal_callback_t{id, sig, callback});
}


/** \brief Remove a user callback.
 *
 * This function searches for the specified callback using its identifier
 * and removes it from the list of callbacks of the signal_handler object.
 *
 * If more than one callback is assigned the same identifier, then all
 * those callbacks are removed at once.
 *
 * \note
 * To be able to remove your callback, you must keep a reference to it.
 * So if you use std::bind() to call add_callback(), you need to keep
 * a reference to that std::bind().
 *
 * \param[in] id  The callback identifier.
 */
void signal_handler::remove_callback(id_t id)
{
    cppthread::guard g(f_mutex);

    for(auto it(f_callbacks.begin()); it != f_callbacks.end();)
    {
        if(it->f_id == id)
        {
            it = f_callbacks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


/** \brief Set signals that generate a stack trace.
 *
 * Whenever a signal happens, this class can automatically log a stack
 * trace of location of the event. By default the mask is set to
 * DEFAULT_SHOW_STACK.
 *
 * You may add signals to the list by doing:
 *
 * \code
 *     set_show_stack(get_show_stack() | ed::signal_handler::SIGNAL_TRAP);
 * \endcode
 *
 * \param[in] sigs  The mask of signals which will generate a stack trace.
 */
void signal_handler::set_show_stack(signal_mask_t sigs)
{
    cppthread::guard g(f_mutex);

    g_show_stack = sigs;
}


/** \brief Get list of signals that generate a stack trace.
 *
 * This function retrieves the current list of signal that request the
 * class to generate a stack trace when they happen.
 *
 * It can be used with the set_show_stack() function in order to add or
 * remove some signals from that list.
 *
 * \return The mask of signals that generate a stack trace.
 */
signal_handler::signal_mask_t signal_handler::get_show_stack() const
{
    cppthread::guard g(f_mutex);

    return g_show_stack;
}


/** \brief Add signals that terminate the process.
 *
 * Any signal that you consider terminal should be added using this function.
 * Whenever that signal is raised by the system, the process_signal() function
 * gets called. If the corresponding bit is set in the show stack mask, then
 * the function first sends the stack trace to the logs, then it terminates
 * the process with a log specifying which signal terminated the process.
 *
 * \note
 * Some signals can't be caught (i.e. SIGKILL). It is useless to add those
 * to this list.
 *
 * \param[in] sigs  The mask of signals that are expected to terminate your
 * process.
 */
void signal_handler::add_terminal_signals(signal_mask_t sigs)
{
    cppthread::guard g(f_mutex);

    for(size_t i(1); i < std::size(f_signal_actions); ++i)
    {
        if((sigs & (1L << i)) != 0 && f_signal_actions[i] == nullptr)
        {
            sigaction_t action = sigaction_t();
            action.sa_sigaction = signal_handler_func;
            action.sa_flags = SA_SIGINFO;

            f_signal_actions[i] = std::make_shared<sigaction_t>();
            sigaction(i, &action, f_signal_actions[i].get());
        }
    }
}


/** \brief The class allows you to ignore some signals.
 *
 * This function allows you to add a list of signals you want to ignore.
 * For example, it is often that you want to ignore SIGPIPE signals when
 * you deal with sockets, otherwise, reading or writing to a closed socket
 * generates that signal instead of just returns a -1.
 *
 * \note
 * Trying to ignore signals such as SIGSEGV and SIGBUS is not a good idea.
 *
 * \param[in] sigs  The mask of signals you want to ignore.
 */
void signal_handler::add_ignored_signals(signal_mask_t sigs)
{
    cppthread::guard g(f_mutex);

    for(size_t i(1); i < std::size(f_signal_actions); ++i)
    {
        if((sigs & (1L << i)) != 0 && f_signal_actions[i] == nullptr)
        {
            sigaction_t action = sigaction_t();
            action.sa_handler = SIG_IGN;

            f_signal_actions[i] = std::make_shared<sigaction_t>();
            sigaction(i, &action, f_signal_actions[i].get());
        }
    }
}


/** \brief Remove a terminal or ignored signal.
 *
 * This function removes the callback for the specified signals. The function
 * has no effect is you did not first add the signal with one of the
 * add_terminal_signals() or add_ignored_signals() functions.
 *
 * \param[in] sigs  The mask of signals to remove from the list of signals
 * we manage through the signal handler.
 */
void signal_handler::remove_signals(signal_mask_t sigs)
{
    cppthread::guard g(f_mutex);

    for(size_t i(1); i < std::size(f_signal_actions); ++i)
    {
        if((sigs & (1L << i)) != 0 && f_signal_actions[i] != nullptr)
        {
            sigaction(i, f_signal_actions[i].get(), nullptr);
            f_signal_actions[i].reset();
        }
    }
}


/** \brief Remove all the signals.
 *
 * Remove all the signals at once.
 *
 * This function is primarily used when the signal_handler is deleted to
 * restore the state to normal. It should be the very last thing you want
 * to do. You are welcome to call this function at any time, of course,
 * with the consequence that none of the signals will now be handled by
 * this handler.
 *
 * This is equivalent to:
 *
 * \code
 *     remove_signals(ed::signal_handler::ALL_SIGNALS);
 * \endcode
 */
void signal_handler::remove_all_signals()
{
    remove_signals(ALL_SIGNALS);
}


/** \brief Get the name of the signal.
 *
 * This function converts the signal \p sig to a name one can use to print
 * the name in a log or a console.
 *
 * \param[in] sig  The signal number.
 *
 * \return The name of the specified signal or nullptr if \p sig is invalid.
 */
char const * signal_handler::get_signal_name(int sig)
{
    if(static_cast<std::size_t>(sig) >= std::size(g_signal_names))
    {
        return nullptr;
    }
    return g_signal_names[sig];
}


/** \brief This is out handler.
 *
 * This function is the handler that gets called whenever a signal is
 * raised.
 *
 * \param[in] sig  The signal that generated this handler.
 * \param[in] info  Information about the signal handler.
 * \param[in] context  The context from when the interrupt was generated.
 */
void signal_handler::signal_handler_func(
          int sig
        , siginfo_t * info
        , void * context)
{
    // if we are called, g_signal_handler can't be nullptr
    //
    g_signal_handler->process_signal(sig, info, reinterpret_cast<ucontext_t *>(context));
}


/** \brief Callback to process a signal we just received.
 *
 * This function is the one called whenever a signal is received by your
 * process. It includes the signal number (\p sig), the signal information
 * as defined by the kernel (\p info) and the user context when the signal
 * happened (\p ucontext).
 *
 * By default, the function prints out the stack trace if requested for that
 * signal and then print a log message about the signal that generated
 * this call. Finally, it calls `std::terminate()` to terminate the process.
 *
 * However, you can add callbacks to capture the signals in your own handler.
 * When doing so, your callback can return true, meaning that you handled the
 * signal and you do not want the default process to take over. See the
 * add_callback() for additional details.
 *
 * \note
 * An `exit(1)` could be very problematic, so would raising an exception in
 * a thread at an impromptus moment (especially in a signal handler). So here
 * we use `std::terminate()`.
 *
 * \param[in] sig  The signal being processed.
 * \param[in] info  Information about the signal handler.
 * \param[in] context  The context from when the interrupt was generated.
 */
void signal_handler::process_signal(
          int sig
        , siginfo_t * info
        , ucontext_t * ucontext)
{
    callback_list_t callbacks;
    bool show_stack(false);

    // here we lock as little as possible
    {
        cppthread::guard g(f_mutex);
        callbacks = f_callbacks;
        show_stack = (g_show_stack & (1UL << sig)) != 0;
    }

    bool handled(false);
    for(auto it(callbacks.begin()); it != callbacks.end(); ++it)
    {
        if(it->f_sig == sig)
        {
            if((it->f_callback)(it->f_id, sig, info, ucontext))
            {
                handled = true;
            }
        }
    }
    if(handled)
    {
        // user said it was handled, leave it to that...
        //
        return;
    }

    if(show_stack)
    {
        auto const trace(libexcept::collect_stack_trace());
        for(auto const & stack_line : trace)
        {
            SNAP_LOG_ERROR
                << "signal_handler(): backtrace="
                << stack_line
                << SNAP_LOG_SEND;
        }
    }

    char const * n(get_signal_name(sig));
    std::string signame;
    if(n == nullptr)
    {
        signame = "UNKNOWN";
    }
    else
    {
        signame = n;
    }

    SNAP_LOG_FATAL
        << "Fatal signal caught: "
        << signame
        << SNAP_LOG_SEND;

    // Abort
    //
    std::terminate();
    snapdev::NOT_REACHED();
}




} // namespace ed
// vim: ts=4 sw=4 et

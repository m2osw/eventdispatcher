// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the Signal Handler class.
 *
 * The Signal Handler class is used to make sure that we an get a log in
 * case a terminal signal happens. We can also use the class to ignore
 * certain flags. In many cases, services want to do that with many flags
 * and that code is pretty much always the same.
 */

// self
//
#include    "eventdispatcher/signal_handler.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>


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


/** \brief Set of signals for which we want to show the stack.
 *
 * Just knowing where a signal occurred is often a bit limited. Knowing
 * the call stack for 10 to 20 items is mych more helpful. However, for
 * some signals, it's generally totally useless so we use a mask to know
 * which signals to write events for.
 */
signal_handler::signal_mask_t       g_show_stack = signal_handler::DEFAULT_SHOW_STACK;


/** \brief The signal_handler allocated.
 *
 * The get_instance() allocates this handler.
 *
 * \warning
 * This function is not thread safe. Please make sure to get the first
 * instance before creating a thread or make sure only one of your
 * threads makes use of this function. It is very likely that you
 * want to initialize this class before most everything else anyway.
 */
signal_handler::pointer_t           g_signal_handler = signal_handler::pointer_t();


/** \brief This is out handler.
 *
 * This function is the handler that gets called whenever a signal is
 * raised.
 *
 * \param[in] sig  The signal thatgenerated this handler.
 * \param[in] info  Information about the signal handler.
 * \param[in] context  The context from when the interrupt was generated.
 */
void signal_handler_func(int sig, siginfo_t * info, void * context)
{
    // if we are called, g_signal_handler can't be nullptr
    //
    g_signal_handler->process_signal(sig, info, reinterpret_cast<ucontext_t *>(context));
}



}
// no name namespace




/** \brief Initialize the signal handler class.
 *
 * This function sets all the signal action structure to a nullptr.
 */
signal_handler::signal_handler()
{
}


/** \brief Restore the signals.
 *
 * Tbe signal destructor restores all the signals that it changed.
 *
 * \note
 * At this point, the destructor is never called since we use an instance
 * and we do not give a way to destroy it at this point.
 */
signal_handler::~signal_handler()
{
    remove_all_signals();
}


signal_handler::pointer_t signal_handler::get_instance()
{
    if(g_signal_handler == nullptr)
    {
        g_signal_handler.reset(new signal_handler());
    }
    return g_signal_handler;
}


void signal_handler::set_show_stack(signal_mask_t sigs)
{
    g_show_stack = sigs;
}


uint64_t signal_handler::get_show_stack() const
{
    return g_show_stack;
}


void signal_handler::add_terminal_signals(signal_mask_t sigs)
{
    for(size_t i = 0; i < sizeof(f_signal_actions) / sizeof(f_signal_actions[0]); ++i)
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


void signal_handler::add_ignore_signals(signal_mask_t sigs)
{
    for(size_t i = 0; i < sizeof(f_signal_actions) / sizeof(f_signal_actions[0]); ++i)
    {
        if((sigs & (1L << i)) != 0 && f_signal_actions[i] == nullptr)
        {
            sigaction_t action = sigaction_t();
            action.sa_handler = SIG_IGN;
            action.sa_sigaction = signal_handler_func;

            f_signal_actions[i] = std::make_shared<sigaction_t>();
            sigaction(i, &action, f_signal_actions[i].get());
        }
    }
}


void signal_handler::remove_signal(signal_mask_t sigs)
{
    for(size_t i = 0; i < sizeof(f_signal_actions) / sizeof(f_signal_actions[0]); ++i)
    {
        if((sigs & (1L << i)) != 0 && f_signal_actions[i] != nullptr)
        {
            sigaction(i, f_signal_actions[i].get(), nullptr);
            f_signal_actions[i].reset();
        }
    }
}


void signal_handler::remove_all_signals()
{
    signal_mask_t sigs(0);
    for(size_t i = 0; i < sizeof(f_signal_actions) / sizeof(f_signal_actions[0]); ++i)
    {
        sigs |= 1L << i;
    }
    remove_signal(sigs);
}


char const * signal_handler::get_signal_name(int sig)
{
    return g_signal_names[sig];
}


void signal_handler::process_signal(int sig, siginfo_t * info, ucontext_t * ucontext)
{
    snap::NOTUSED(info);
    snap::NOTUSED(ucontext);

    if((g_show_stack & (1UL << sig)) != 0)
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

    // Exit with error status
    //
    ::exit(1);
    snap::NOTREACHED();
}




} // namespace ed
// vim: ts=4 sw=4 et

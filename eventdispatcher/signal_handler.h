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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Functions used to setup how unrecoverable Unix signals should be
 * handled.
 */

// self
//
//#include    "eventdispatcher/connection.h"


// C++ lib
//
#include    <memory>


// C lib
//
#include    <signal.h>



namespace ed
{



class signal_handler
{
public:
    typedef std::shared_ptr<signal_handler>     pointer_t;
    typedef uint64_t                            signal_mask_t;

    static constexpr signal_mask_t const        SIGNAL_HANGUP           = 1UL << SIGHUP;
    static constexpr signal_mask_t const        SIGNAL_INTERRUPT        = 1UL << SIGINT;
    static constexpr signal_mask_t const        SIGNAL_QUIT             = 1UL << SIGQUIT;
    static constexpr signal_mask_t const        SIGNAL_ILLEGAL          = 1UL << SIGILL;
    static constexpr signal_mask_t const        SIGNAL_TRAP             = 1UL << SIGTRAP;
    static constexpr signal_mask_t const        SIGNAL_ABORT            = 1UL << SIGABRT; // SIGIOT
    static constexpr signal_mask_t const        SIGNAL_BUS              = 1UL << SIGBUS;
    static constexpr signal_mask_t const        SIGNAL_FLOATPOINTERROR  = 1UL << SIGFPE;
    static constexpr signal_mask_t const        SIGNAL_KILL             = 1UL << SIGKILL;
    static constexpr signal_mask_t const        SIGNAL_USR1             = 1UL << SIGUSR1;
    static constexpr signal_mask_t const        SIGNAL_SEGMENTVIOLATION = 1UL << SIGSEGV;
    static constexpr signal_mask_t const        SIGNAL_USR2             = 1UL << SIGUSR2;
    static constexpr signal_mask_t const        SIGNAL_PIPE             = 1UL << SIGPIPE;
    static constexpr signal_mask_t const        SIGNAL_ALARM            = 1UL << SIGALRM;
    static constexpr signal_mask_t const        SIGNAL_TERMINATE        = 1UL << SIGTERM;  // Ctrl-C
    static constexpr signal_mask_t const        SIGNAL_STACK_FAULT      = 1UL << SIGSTKFLT;
    static constexpr signal_mask_t const        SIGNAL_CHILD            = 1UL << SIGCHLD;
    static constexpr signal_mask_t const        SIGNAL_CONTINUE         = 1UL << SIGCONT;  // Ctrl-Q
    static constexpr signal_mask_t const        SIGNAL_STOP             = 1UL << SIGSTOP;  // Ctrl-S
    static constexpr signal_mask_t const        SIGNAL_INTERACTIVE_STOP = 1UL << SIGTSTP;
    static constexpr signal_mask_t const        SIGNAL_TERMINAL_IN      = 1UL << SIGTTIN;  // Ctrl-Z
    static constexpr signal_mask_t const        SIGNAL_TERMINAL_OUT     = 1UL << SIGTTOU;
    static constexpr signal_mask_t const        SIGNAL_URGENT           = 1UL << SIGURG;
    static constexpr signal_mask_t const        SIGNAL_XCPU             = 1UL << SIGXCPU;
    static constexpr signal_mask_t const        SIGNAL_FILE_SIZE        = 1UL << SIGXFSZ;
    static constexpr signal_mask_t const        SIGNAL_VIRTUAL_ALARM    = 1UL << SIGVTALRM;
    static constexpr signal_mask_t const        SIGNAL_PROFILING        = 1UL << SIGPROF;
    static constexpr signal_mask_t const        SIGNAL_WINDOW_CHANGE    = 1UL << SIGWINCH;
    static constexpr signal_mask_t const        SIGNAL_POLL             = 1UL << SIGPOLL; // SIGIO/SIGLOST
    static constexpr signal_mask_t const        SIGNAL_POWER            = 1UL << SIGPWR;
    static constexpr signal_mask_t const        SIGNAL_SYSTEM           = 1UL << SIGSYS;

    static constexpr signal_mask_t const        DEFAULT_SIGNAL_TERMINAL =
                                                      SIGNAL_INTERRUPT
                                                    | SIGNAL_QUIT
                                                    | SIGNAL_ILLEGAL
                                                    | SIGNAL_BUS
                                                    | SIGNAL_FLOATPOINTERROR
                                                    | SIGNAL_SEGMENTVIOLATION
                                                    | SIGNAL_TERMINATE;
    static constexpr signal_mask_t const        DEFAULT_SIGNAL_IGNORE =
                                                      SIGNAL_INTERACTIVE_STOP
                                                    | SIGNAL_TERMINAL_IN
                                                    | SIGNAL_TERMINAL_OUT;

    static constexpr signal_mask_t const        DEFAULT_SHOW_STACK =
                                                      SIGNAL_INTERRUPT
                                                    | SIGNAL_QUIT
                                                    | SIGNAL_TERMINATE;

                                signal_handler(signal_handler const &) = delete;
    virtual                     ~signal_handler();

    signal_handler              operator = (signal_handler const &) = delete;

    pointer_t                   get_instance();

    void                        set_show_stack(signal_mask_t sigs);
    signal_mask_t               get_show_stack() const;
    void                        add_terminal_signals(signal_mask_t sigs);
    void                        add_ignore_signals(signal_mask_t sigs);
    void                        remove_signal(signal_mask_t sigs);
    void                        remove_all_signals();
    static char const *         get_signal_name(int sig);

    virtual void                process_signal(int sig, siginfo_t * info, ucontext_t * ucontext);

private:
                                signal_handler();

    typedef struct sigaction                sigaction_t;
    typedef std::shared_ptr<sigaction_t>    sigaction_ptr_t;

    sigaction_ptr_t             f_signal_actions[64] = { sigaction_ptr_t() };
};



} // namespace ed
// vim: ts=4 sw=4 et
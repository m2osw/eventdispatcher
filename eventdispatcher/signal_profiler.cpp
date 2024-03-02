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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the Signal Profiler class.
 *
 * The Signal Profiler class is at attempt to implement the SIGPROF without
 * stopping system functions such as poll(). At this point, this doesn't
 * work.
 *
 * To test with the profiler, use the `-pg` command line option on your g++
 * command line. See the g++ docs for more info.
 */

// self
//
#include    "eventdispatcher/signal_profiler.h"


// C
//
#include    <ucontext.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{

/** \brief Initialize the profiler signal objects.
 *
 * This constructor sets up the signal within the event dispatcher
 * environment and then it retrieves the pointer to the gcc SIGPROF
 * function which is used to collect the necessary data.
 *
 * \todo
 * Make this whole thing work. It is very likely that the stack trace
 * is going to be wrong if the handler doesn't use the newer scheme
 * (i.e. the one where the context gets saved at the time the event
 * occurs and not at the time we're handling it here). At the moment,
 * Linux does it properly, so no worries here. That being said, so
 * far I've not been able to make it work properly.
 */
signal_profiler::signal_profiler()
    : signal(SIGPROF)
{
    // retrieve the handler so we can call it whenever the signal occurs
    //
    sigaction(SIGPROF, nullptr, &f_action);
}

void signal_profiler::process_signal()
{
    if((f_action.sa_flags & SA_SIGINFO) != 0)
    {
        if(f_action.sa_handler != SIG_IGN
        && f_action.sa_handler != SIG_DFL
        && f_action.sa_handler != nullptr)
        {
            (*f_action.sa_handler)(SIGPROF);
        }
    }
    else
    {
        if(f_action.sa_sigaction != nullptr)
        {
            signalfd_siginfo const * fdinfo(get_signal_info());
            siginfo_t info = siginfo_t();
            info.si_signo       = fdinfo->ssi_signo;
            info.si_errno       = fdinfo->ssi_errno;
            info.si_code        = fdinfo->ssi_code;
            //info.si_trapno      = fdinfo->ssi_trapno; // not defined on all machines
            info.si_pid         = fdinfo->ssi_pid;
            info.si_uid         = fdinfo->ssi_uid;
            info.si_status      = fdinfo->ssi_status;
            info.si_utime       = fdinfo->ssi_utime;
            info.si_stime       = fdinfo->ssi_stime;
            info.si_value.sival_int = 0;
            info.si_int         = fdinfo->ssi_int;
            info.si_ptr         = reinterpret_cast<void *>(fdinfo->ssi_ptr);
            info.si_overrun     = fdinfo->ssi_overrun;
            info.si_timerid     = 0;
            info.si_addr        = reinterpret_cast<void *>(fdinfo->ssi_addr);
            info.si_band        = fdinfo->ssi_band;
            info.si_fd          = fdinfo->ssi_fd;
            info.si_addr_lsb    = 0; //fdinfo->ssi_addr_lsb; // not on arch64 and doesn't help at this point

            // the following fields don't exist in 16.04, keep commented until we stop support
            //info.si_lower       = 0;
            //info.si_upper       = 0;
            //info.si_pkey        = 0;

            info.si_call_addr   = 0;
            info.si_syscall     = 0;
            info.si_arch        = 0;

            ucontext_t uc;
            getcontext(&uc);

            (*f_action.sa_sigaction)(SIGPROF, &info, &uc);
        }
    }
}


} // namespace ed
// vim: ts=4 sw=4 et

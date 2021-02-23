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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    "eventdispatcher/connection.h"


// C lib
//
#include    <sys/signalfd.h>



namespace ed
{



class signal
    : public connection
{
public:
    typedef std::shared_ptr<signal>     pointer_t;

                                signal(int posix_signal);
    virtual                     ~signal() override;

    void                        close();
    void                        unblock_signal_on_destruction();

    // connection implementation
    virtual bool                is_signal() const override;
    virtual int                 get_socket() const override;

    pid_t                       get_child_pid() const;
    signalfd_siginfo const *    get_signal_info() const;

private:
    friend communicator;

    void                        process();

    int                         f_signal = 0;   // i.e. SIGHUP, SIGTERM...
    int                         f_socket = -1;  // output of signalfd()
    signalfd_siginfo            f_signal_info = signalfd_siginfo();
    bool                        f_unblock = false;
};



} // namespace ed
// vim: ts=4 sw=4 et

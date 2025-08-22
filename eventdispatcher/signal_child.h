// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

/** \file
 * \brief Handle the SIGCHLD specifically.
 *
 * This class is used to capture the SIGCHLD signal from the OS and call
 * a corresponding callback function.
 *
 * Whenever you want to knwow whether a child you created died, you can
 * use this class. Get the instance (it's a singleton) and then use the
 * add_listener().
 *
 * Once you get called with a child that exited or was signaled, that
 * listener is automatically removed from the list of listeners (since
 * the child is gone, there is really no need for that listener).
 */

// self
//
#include    <eventdispatcher/signal.h>


// cppthread
//
#include    <cppthread/mutex.h>


// C++
//
#include    <functional>
#include    <list>


// C
//
#include    <sys/resource.h>
#include    <sys/signalfd.h>
#include    <sys/wait.h>



namespace ed
{



typedef std::uint32_t           flag_t;

constexpr flag_t const          SIGNAL_CHILD_FLAG_RUNNING       = 0x0001;
constexpr flag_t const          SIGNAL_CHILD_FLAG_EXITED        = 0x0002;
constexpr flag_t const          SIGNAL_CHILD_FLAG_SIGNALED      = 0x0004;
constexpr flag_t const          SIGNAL_CHILD_FLAG_STOPPED       = 0x0008;
constexpr flag_t const          SIGNAL_CHILD_FLAG_CONTINUED     = 0x0010;


class child_status
{
public:
                        child_status(siginfo_t const & info);

    pid_t               child_pid() const;
    uid_t               child_uid() const;
    bool                is_running() const;
    bool                is_exited() const;
    bool                is_signaled() const;
    bool                is_core_dumped() const;
    bool                is_stopped() const;
    bool                is_continued() const;
    flag_t              status_mask() const;

    int                 exit_code() const;
    int                 terminate_signal() const;
    int                 stop_signal() const;

private:
    siginfo_t           f_info = siginfo_t();
};


class signal_child
    : public signal
{
public:
    typedef std::shared_ptr<signal_child>                       pointer_t;
    typedef std::function<void(child_status const & status)>    func_t;

    virtual                     ~signal_child() override;

    static pointer_t            get_instance();

    // signal implementation
    //
    virtual void                process_signal() override;
    virtual void                connection_added() override;
    virtual void                connection_removed() override;

    void                        add_listener(
                                      pid_t child
                                    , func_t callback
                                    , flag_t mask = SIGNAL_CHILD_FLAG_EXITED | SIGNAL_CHILD_FLAG_SIGNALED);
    void                        remove_listener(pid_t child);

private:
    struct callback_t
    {
        typedef std::list<callback_t>     list_t;

        pid_t           f_child = 0;
        func_t          f_callback = 0;
        flag_t          f_flags = 0;
    };

    explicit                    signal_child();

    void                        add_connection();
    void                        remove_connection();

    callback_t::list_t          f_listeners = callback_t::list_t();
    cppthread::mutex            f_mutex = cppthread::mutex();
    std::uint32_t               f_count = 0;
    bool                        f_adding_to_communicator = false;
    bool                        f_removing_to_communicator = false;
};



} // namespace ed
// vim: ts=4 sw=4 et

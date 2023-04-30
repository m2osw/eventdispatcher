// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Process changed class.
 *
 * Class used to listen on process events. This connection is useful to
 * know each time a process is created or stopped. Note, however, that your
 * process needs to be root at the time you create the connection.
 */

// self
//
#include    <eventdispatcher/connection.h>


// snapdev
//
#include    <snapdev/chownnm.h>
#include    <snapdev/raii_generic_deleter.h>



namespace ed
{



enum class process_event_t
{
    PROCESS_EVENT_UNKNOWN,  // kernel sent us an event we do not know about
    PROCESS_EVENT_NONE,     // used to acknowledge our send() calls
    PROCESS_EVENT_FORK,     // new task created
    PROCESS_EVENT_EXEC,     // new task started execution
    PROCESS_EVENT_UID,      // ruid/euid changed
    PROCESS_EVENT_GID,      // rgid/egid changed
    PROCESS_EVENT_SESSION,  // process is a session (child detached from parent)
    PROCESS_EVENT_PTRACE,   // entered process tracing mode (parent is tracer)
    PROCESS_EVENT_COMMAND,  // /proc/###/comm changed (i.e. renamed thread)
    PROCESS_EVENT_COREDUMP, // task exited with a coredump
    PROCESS_EVENT_EXIT,     // task exited
};


char const * process_event_to_string(process_event_t event);

inline std::ostream & operator << (std::ostream & out, process_event_t event)
{
    return out << process_event_to_string(event);
}



class process_changed_event
{
public:
    process_event_t         get_event() const;
    void                    set_event(process_event_t event);

    std::uint32_t           get_cpu() const;
    void                    set_cpu(std::uint32_t timestamp);
    std::uint64_t           get_timestamp() const;
    snapdev::timespec_ex    get_realtime() const;
    void                    set_timestamp(std::uint64_t timestamp);

    pid_t                   get_pid() const;
    void                    set_pid(pid_t pid);
    pid_t                   get_tgid() const;
    void                    set_tgid(pid_t tgid);

    pid_t                   get_parent_pid() const;
    void                    set_parent_pid(pid_t pid);
    pid_t                   get_parent_tgid() const;
    void                    set_parent_tgid(pid_t tgid);

    uid_t                   get_ruid() const;
    void                    set_ruid(uid_t uid);
    uid_t                   get_euid() const;
    void                    set_euid(uid_t uid);

    gid_t                   get_rgid() const;
    void                    set_rgid(gid_t uid);
    gid_t                   get_egid() const;
    void                    set_egid(gid_t uid);

    std::string const &     get_command() const;
    void                    set_command(std::string const & command);

    std::int32_t            get_exit_code() const;
    void                    set_exit_code(std::int32_t code);
    std::int32_t            get_exit_signal() const;
    void                    set_exit_signal(std::int32_t signal);

private:
    process_event_t         f_event = process_event_t::PROCESS_EVENT_UNKNOWN;

    std::uint32_t           f_cpu = 0;
    std::uint64_t           f_timestamp = 0;

    pid_t                   f_pid = -1;
    pid_t                   f_tgid = -1;

    pid_t                   f_parent_pid = -1;
    pid_t                   f_parent_tgid = -1;

    uid_t                   f_ruid = snapdev::NO_UID;
    uid_t                   f_euid = snapdev::NO_UID;

    gid_t                   f_rgid = snapdev::NO_GID;
    gid_t                   f_egid = snapdev::NO_GID;

    std::string             f_command = std::string();
    std::int32_t            f_exit_code = -1;
    std::int32_t            f_exit_signal = -1;
};



class process_changed
    : public connection
{
public:
    typedef std::shared_ptr<process_changed>           pointer_t;

                                process_changed();
    virtual                     ~process_changed() override;

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;
    virtual void                set_enable(bool enabled) override;
    virtual void                process_read() override;

    // new callback
    virtual void                process_event(process_changed_event const & event) = 0;

private:
    void                        listen_for_events();

    snapdev::raii_fd_t          f_socket = snapdev::raii_fd_t();
};



} // namespace ed
// vim: ts=4 sw=4 et

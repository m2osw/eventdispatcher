// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

// advgetopt lib
//
#include    <advgetopt/utils.h>


// C++ lib
//
#include    <memory>
#include    <string>


// C lib
//
#include    <unistd.h>



namespace cppprocess
{



enum class process_state_t : char
{
    // IMPORTANT: all states are not available on all versions of Linux
    //
    PROCESS_STATE_UNKNOWN = '?',        // could not read state
    PROCESS_STATE_RUNNING = 'R',
    PROCESS_STATE_SLEEPING = 'S',
    PROCESS_STATE_DISK_SLEEP = 'D',
    PROCESS_STATE_ZOMBIE = 'Z',
    PROCESS_STATE_STOPPED = 'T',
    PROCESS_STATE_TRACING_STOP = 't',
    PROCESS_STATE_PAGING = 'W',
    PROCESS_STATE_DEAD = 'X',
    PROCESS_STATE_DEAD2 = 'x',
    PROCESS_STATE_WAKE_KILL = 'K',
    PROCESS_STATE_WAKING = 'W',
    PROCESS_STATE_PARKED = 'P',
};


class process_info
{
public:
    typedef std::shared_ptr<process_info>      pointer_t;

                            process_info(pid_t pid);

    // basic process information
    //
    pid_t                   get_pid();
    pid_t                   get_ppid();
    pid_t                   get_pgid();

    // re-add function(s) as required, implement the retrieval in one of
    // the specialized load_...() functions.
    //

    // process name/arguments
    //
    std::string             get_name();
    std::string             get_command();
    std::string             get_basename();
    std::size_t             get_args_size();
    std::string             get_arg(int index);

    // CPU info, priority
    //
    process_state_t         get_state(bool force = true);
    //unsigned                get_cpu_percent();
    void                    get_times(unsigned long long & utime, unsigned long long & stime, unsigned long long & cutime, unsigned long long & cstime);
    int                     get_priority();
    int                     get_nice();

    // memory info
    //
    void                    get_page_faults(std::uint64_t & major, std::uint64_t & minor);
    std::uint64_t           get_total_size();
    std::uint64_t           get_rss_size();

    // I/O info
    //
    void                    get_tty(int & major, int & minor);

private:
    void                    load_stat(bool force = false);
    void                    load_cmdline();

    pid_t                   f_pid = -1;     // if -1, process died

    // load_stat()
    //
    std::string             f_name = std::string();
    process_state_t         f_state = process_state_t::PROCESS_STATE_UNKNOWN;
    pid_t                   f_ppid = -1;    // if -1, not yet loaded (see load_stat())
    gid_t                   f_pgid = -1;
    pid_t                   f_session = -1;
    int                     f_tty_major = -1;
    int                     f_tty_minor = -1;
    gid_t                   f_fp_group = -1;
    int                     f_kernel_flags = -1;
    std::uint64_t           f_minor_faults = -1;
    std::uint64_t           f_children_minor_faults = -1;
    std::uint64_t           f_major_faults = -1;
    std::uint64_t           f_children_major_faults = -1;
    int                     f_user_time = -1;
    int                     f_system_time = -1;
    int                     f_children_user_time = -1;
    int                     f_children_system_time = -1;
    int                     f_priority = -1;
    int                     f_nice = -1;
    int                     f_num_threads = -1;
    std::int64_t            f_start_time = -1;
    std::int64_t            f_virtual_size = -1;
    std::int64_t            f_rss = -1;
    std::int64_t            f_rss_limit = -1;
    std::int64_t            f_start_code = -1;
    std::int64_t            f_end_code = -1;
    std::int64_t            f_start_stack = -1;
    std::int64_t            f_kernel_esp = -1;
    std::int64_t            f_kernel_eip = -1;
    int                     f_wchan = -1;
    int                     f_exit_signal = -1;
    int                     f_processor = -1;
    int                     f_rt_priority = -1;
    int                     f_schedule_policy = -1;
    int                     f_delayacct_blkio_ticks = -1;
    std::int64_t            f_guest_time = -1;
    std::int64_t            f_children_guest_time = -1;
    std::int64_t            f_start_data = -1;
    std::int64_t            f_end_data = -1;
    std::int64_t            f_start_break = -1;
    std::int64_t            f_arg_start = -1;
    std::int64_t            f_arg_end = -1;
    std::int64_t            f_env_start = -1;
    std::int64_t            f_env_end = -1;
    int                     f_exit_code = -1;

    // load_cmdline()
    //
    advgetopt::string_list_t
                            f_args = advgetopt::string_list_t();
};




} // namespace cppprocess
// vim: ts=4 sw=4 et

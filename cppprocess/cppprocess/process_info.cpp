// Copyright (c) 2013-2023  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the process_info class.
 *
 * This file includes code to read data from the `/proc/<pid>/...` folder.
 * It parses the data and then saves it in various fields which you can
 * retrieve using the various function available in the process_info
 * class.
 *
 * The data is read when necessary and, in many cases, cached. In other
 * words, doing a get again may return data that was cached opposed to
 * data that was just read.
 *
 * See the proc(5) manual page for details about all the fields found in
 * the files under `/proc/<pid>/...`.
 *
 * \sa https://man7.org/linux/man-pages/man5/proc.5.html
 */

// self
//
#include    <cppprocess/process_info.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/tokenize_string.h>
#include    <snapdev/pathinfo.h>


// C++
//
#include    <cstdint>
#include    <iostream>


// last include
//
#include <snapdev/poison.h>



namespace cppprocess
{



/** \brief Initialize a process_info object.
 *
 * This function saves the pid_t of the process. The other functions are
 * then able to retrieve process details such as the name, argument list,
 * processing times, etc.
 *
 * Most of the data gets cached, you can also reset a process_info object
 * to retrieve the data anew from the corresponding `/proc/<pid>/...` file.
 *
 * \param[in] pid  The pid of the process you're interested in.
 */
process_info::process_info(pid_t pid)
    : f_pid(pid)
{
}


/** \brief Get the process identifier.
 *
 * This function retrieves the process identifier of this process_info
 * object.
 *
 * This function actually verifies that the process is still alive. If not,
 * then it returns -1. Once this function returns -1 once, it will then
 * always return -1.
 *
 * \return The process identifier.
 */
pid_t process_info::get_pid()
{
    if(f_pid != -1)
    {
        std::string const filename("/proc/" + std::to_string(f_pid));
        struct stat st;
        if(stat(filename.c_str(), &st) != 0)
        {
            f_pid = -1;
        }
        else if(!S_ISDIR(st.st_mode))
        {
            f_pid = -1;
        }
    }

    return f_pid;
}


/** \brief Get the parent process identifier.
 *
 * This function retrieves the parent process identifier of this
 * proc_info object.
 *
 * \return The parent process identifier.
 */
pid_t process_info::get_ppid()
{
    load_stat();

    return f_ppid;
}


/** \brief Get the process main group identifier.
 *
 * This function returns the process main group identifier. At first this
 * is the same as the main group of the user that started the process
 * although the process can change that parameter (if given the right to
 * do so).
 *
 * \return The group the process is a part of.
 */
pid_t process_info::get_pgid()
{
    load_stat();

    return f_pgid;
}


/** \brief Get the number of minor & major page faults.
 *
 * This function retrieves the minor & major number of page faults.
 *
 * \param[out] major  The major page fault since last update.
 * \param[out] minor  The minor page fault since last update.
 *
 * \return The process page fault statistics.
 */
void process_info::get_page_faults(std::uint64_t & major, std::uint64_t & minor)
{
    load_stat();

    minor = f_minor_faults;
    major = f_major_faults;
}


// Where did libproc get that from?! (i.e. you'd need to read all the
// process to compute this, I think)
///** \brief Get the immediate percent of CPU usage for this process.
// *
// * This function retrieves the CPU usage as a percent of total CPU
// * available.
// *
// * \return The immediate CPU usage as a percent.
// */
//unsigned process_info::get_pcpu() const -- TBD
//{
//    return f_pcpu;
//}


/** \brief Get the immediate process status.
 *
 * This function retrieves the CPU status of the process.
 *
 * The status is one of the following:
 *
 * \li D -- uninterruptible sleep (usually I/O)
 * \li R -- running or runnable
 * \li S -- Sleeping
 * \li T -- stopped by a job control signal or trace
 * \li W -- paging (should not occur)
 * \li X -- dead (should never appear)
 * \li Z -- defunct zombie process
 *
 * See `man 5 proc` for more details (versions when such and such flag
 * as defined).
 *
 * \warning
 * If you set the \p force parameter to false, then the status is not
 * updated. In most cases, you want to first call this function to
 * update the `stat` data and then read the other data which will be
 * what was read at the same time as this status.
 *
 * \param[in] force  Whether to force a reload of the `stat` file.
 *
 * \return The status of the process.
 */
process_state_t process_info::get_state(bool force)
{
    load_stat(force);

    return f_state;
}


/** \brief Get the percent usage of CPU by this process.
 *
 * \todo
 * Implement. At this time, this is not done because it requires reading
 * information about all the threads and compute the percent which is
 * not that simple. Also, right now we do not really need this info.
 *
 * \return Always -1 (until it gets implemented).
 */
int process_info::get_cpu_percent()
{
    return -1;
}


/** \brief Get the amount of time spent by this process.
 *
 * This function gives you information about the four variables
 * available cummulating the amount of time the process spent
 * running so far.
 *
 * \param[out] utime  The accumulated user time of this very task.
 * \param[out] stime  The accumulated kernel time of this very task.
 * \param[out] cutime  The accumulated user time of this task and
 *                     its children.
 * \param[out] cstime  The accumulated kernel time of this task
 *                     and its children.
 */
void process_info::get_times(
      unsigned long long & utime
    , unsigned long long & stime
    , unsigned long long & cutime
    , unsigned long long & cstime)
{
    load_stat();

    utime = f_user_time;
    stime = f_system_time;
    cutime = f_children_user_time;
    cstime = f_children_system_time;
}


/** \brief Get the real time priority of this process.
 *
 * This function returns the real time priority of the process.
 *
 * \return The process real time priority.
 */
int process_info::get_priority()
{
    load_stat();

    return f_priority;
}


/** \brief Get the Unix nice of this process.
 *
 * This function returns the Unix nice of the process.
 *
 * \return The process unix nice.
 */
int process_info::get_nice()
{
    load_stat();

    return f_nice;
}


/** \brief Get the size of this process.
 *
 * This function returns the total size of the process defined as
 * the virtual memory size.
 *
 * \todo
 * Look at loading the memory info as in `/proc/<pid>/status`.
 *
 * \return The process total virtual size.
 */
std::uint64_t process_info::get_total_size()
{
    load_stat();

    return f_rss
            + f_end_code - f_start_code
            + f_end_data - f_start_data;
}


/** \brief Get the RSS size of this process.
 *
 * This function returns the RSS size of the process defined as
 * the virtual memory size.
 *
 * \return The process RSS virtual size.
 */
std::uint64_t process_info::get_rss_size()
{
    load_stat();

    return f_rss;
}


/** \brief Get the process name.
 *
 * This function return the name as found in the `comm` file. This name
 * is usually the first 15 letters of the command name. This name can
 * be changed so it may differ from the one found in the `cmdline`.
 * We most often change our thread names to reflect what they are
 * used for.
 *
 * This is similar to the basename, however, the basename will be
 * "calculate" from the `cmdline` file instead.
 *
 * \note
 * The same name is found in the `stat` file (between parenthesis) so
 * we read it from there and not the `comm` file.
 *
 * \return The process name.
 */
std::string process_info::get_name()
{
    load_stat();

    return f_name;
}


/** \brief Get the process command.
 *
 * This function returns the command path and name as defined on the command
 * line.
 *
 * If something goes wrong (i.e. the process dies) then the function returns
 * an empty string.
 *
 * \return The command name and path.
 */
std::string process_info::get_command()
{
    load_cmdline();

    if(f_args.empty())
    {
        return std::string();
    }

    return f_args[0];
}


/** \brief Get the process (command) basename.
 *
 * By default, the process name is the full name used on the command line
 * to start this process. If that was a full path, then the full pass is
 * included in the process name.
 *
 * This function returns the basename only.
 *
 * \return The process basename.
 */
std::string process_info::get_basename()
{
    return snapdev::pathinfo::basename(get_command());
}


/** \brief Get the number of arguments defined on the command line.
 *
 * This function counts the number of arguments, including any empty
 * arguments.
 *
 * Count will be positive or null. The count does include the command
 * line (program name with index 0). This is why this function is
 * expected to return 1 or more. However, if the call happens after
 * the process died, then you will get zero.
 *
 * \return Count the number of arguments.
 *
 * \sa get_arg()
 */
std::size_t process_info::get_args_size()
{
    load_cmdline();

    return f_args.size();
}


/** \brief Get the argument at the specified index.
 *
 * This function returns one of the arguments of the command line of
 * this process. Note that it happens that arguments are empty strings.
 * The very first argument (index of 0) is the command full name, just
 * like in `argv[]`.
 *
 * \note
 * If the index is out of bounds, the function returns an empty string.
 * To know the number of available arguments, use the get_args_size()
 * function first.
 *
 * \param[in] index  The index of the argument to retrieve.
 *
 * \return The specified argument or an empty string.
 *
 * \sa get_args_size()
 */
std::string process_info::get_arg(int index)
{
    if(static_cast<std::size_t>(index) >= get_args_size())
    {
        return std::string();
    }

    return f_args[index];
}


/** \brief Get the controlling terminal major/minor of this process.
 *
 * This function returns the TTY device major and minor numbers.
 *
 * This is the controlling terminal. It may return zeroes in which case
 * there is no controlling terminal attached to that process.
 *
 * \param[out] major  The major device number of the controlling terminal.
 * \param[out] minor  The minor device number of the controlling terminal.
 */
void process_info::get_tty(int & major, int & minor)
{
    load_stat();

    major = f_tty_major;
    minor = f_tty_minor;
}






/** \brief Load the `stat` file.
 *
 * This function loads the `/proc/<pid>/stat` file.
 *
 * If force is false (the default) and the file was already read, it does
 * not get reloaded. If you want the most current data for this process,
 * make sure to call the get_state() function with true first, then call
 * the other functions which will then get the updated data.
 *
 * \note
 * The functions that use the data read by the function make sure to
 * call it. The fields that are not defined by your Linux kernel will
 * generally be set to 0 and never change.
 *
 * \param[in] force  Whether to force a reload.
 */
void process_info::load_stat(bool force)
{
    // already read?
    //
    if(f_ppid != -1
    && !force)
    {
        return;
    }

    // still active?
    //
    pid_t const pid(get_pid());
    if(pid == -1)
    {
        return;
    }

    // read stat
    //
    snapdev::file_contents s("/proc/" + std::to_string(pid) + "/stat");
    s.size_mode(snapdev::file_contents::size_mode_t::SIZE_MODE_READ);
    if(!s.read_all())
    {
        return;
    }

    // first we must extract the name because it can include spaces and
    // parenthesis so it completely breaks the rest of the parser otherwise
    //
    std::string line(s.contents());
    std::string::size_type const first_paren(line.find('('));
    std::string::size_type const last_paren(line.rfind(')'));

    // name not found!?
    //
    if(first_paren < 2
    || first_paren == std::string::npos
    || last_paren > 100)
    {
        return;
    }

    // pid mismatch?!
    //
    if(line.substr(0, first_paren - 1) != std::to_string(pid))
    {
        return;
    }

    // retrieve name
    //
    f_name = line.substr(first_paren + 1, last_paren - first_paren - 1);

    // retrieve the remaining fields (many)
    //
    // TODO: I don't think we need to (1) tokenize and then (2) convert
    //       to integers, instead we want to consider a function which
    //       converts directly to integers and saves the values to the
    //       vector
    //
    std::vector<std::string> fields;
    std::string remaining(line.substr(last_paren + 2));
    snapdev::tokenize_string(fields, remaining, " ");

    if(fields.size() >= 1)
    {
        f_state = static_cast<process_state_t>(fields[0][0]);
    }
    else
    {
        f_state = process_state_t::PROCESS_STATE_UNKNOWN;
    }

    if(fields.size() <= 1)
    {
        return;
    }

    // convert the fields to numbers
    // except for the status (first in `fields`), all are positive numbers
    //
    std::vector<std::int64_t> values;
    values.reserve(fields.size() - 1);
    std::transform(
          fields.cbegin() + 1
        , fields.cend()
        , std::back_inserter(values)
        , [](auto const & v) { return std::stoull(v); });

    // since the vector starts with the PPID, I include an offset
    // so that way the index looks like the one found in the docs
    //
    constexpr int const field_offset(4);

    // the number of values can vary so we use a function to make sure
    // we don't go over the maximum number of values available in our
    // vector
    //
    auto get_value = [values](int idx) {
            std::size_t const offset(idx - field_offset);
            return offset >= values.size()
                ? 0
                : values[offset];
        };

    // TBD: for the below values we could also consider using an array and
    //      use indices defined in an enum and then have a form of mapping
    //      which would ease updates (although I don't think the kernel
    //      makes changes to those values much of the time)

    f_ppid = get_value(4);
    f_pgid = get_value(5);
    f_session = get_value(6);

    std::uint32_t const tty(get_value(7));
    f_tty_major =  (tty >>  8) & 0xffff;
    f_tty_minor = ((tty >> 16) & 0xff00)
                | ((tty >>  0) & 0x00ff);

    f_fp_group = get_value(8);
    f_kernel_flags = get_value(9);  // see PF_* in /usr/src/linux-headers-<version>/include/linux/sched.h
    f_minor_faults = get_value(10);
    f_children_minor_faults = get_value(11);
    f_major_faults = get_value(12);
    f_children_major_faults = get_value(13);
    f_user_time = get_value(14);
    f_system_time = get_value(15);
    f_children_user_time = get_value(16);
    f_children_system_time = get_value(17);
    f_priority = get_value(18);
    f_nice = get_value(19);
    f_num_threads = get_value(20);  // earlier versions of Linux used this field for something else
    // skip 21
    f_start_time = get_value(22);
    f_virtual_size = get_value(23);
    f_rss = get_value(24);
    f_rss_limit = get_value(25);
    f_start_code = get_value(26);
    f_end_code = get_value(27);
    f_start_stack = get_value(28);
    f_kernel_esp = get_value(29);
    f_kernel_eip = get_value(30);
    // skip 31
    // skip 32
    // skip 33
    // skip 34
    f_wchan = get_value(35);
    // skip 36
    // skip 37
    f_exit_signal = get_value(38);
    f_processor = get_value(39);
    f_rt_priority = get_value(40);
    f_schedule_policy = get_value(41);  // see SCHED_* in /usr/src/linux-headers-<version>/include/linux/sched.h
    f_delayacct_blkio_ticks = get_value(42);
    f_guest_time = get_value(43);
    f_children_guest_time = get_value(44);
    f_start_data = get_value(45);
    f_end_data = get_value(46);
    f_start_break = get_value(47);
    f_arg_start = get_value(48);
    f_arg_end = get_value(49);
    f_env_start = get_value(50);
    f_env_end = get_value(51);
    f_exit_code = get_value(52);
}


/** \brief Load the command line.
 *
 * This function loads the command line and arguments once. It will be
 * cached when further calls happen.
 *
 * It will fill the f_args vector of strings. 0 will be the full path and
 * command name. The other strings are the arguments. The array is not
 * ended with a nullptr. Use the vector::size() to know whether you reached
 * the end or not.
 */
void process_info::load_cmdline()
{
    // already loaded?
    //
    if(!f_args.empty())
    {
        return;
    }

    // still active?
    //
    pid_t const pid(get_pid());
    if(pid == -1)
    {
        return;
    }

    // read cmdline
    //
    snapdev::file_contents cl("/proc/" + std::to_string(pid) + "/cmdline");
    cl.size_mode(snapdev::file_contents::size_mode_t::SIZE_MODE_READ);
    if(!cl.read_all())
    {
        return;
    }

    std::string const cmdline(cl.contents());

    // the following gives us the ability to handle the last string even if
    // not terminated with a null character
    //
    char const * l(cmdline.c_str());
    char const * e(l + cmdline.length());
    for(;;)
    {
        char const * s(l);
        while(l < e && *l != '\0')
        {
            ++l;
        }
        f_args.emplace_back(s, static_cast<std::string::size_type>(l - s));
        ++l;
        if(l >= e)
        {
            break;
        }
    }
}







} // namespace cppprocess
// vim: ts=4 sw=4 et

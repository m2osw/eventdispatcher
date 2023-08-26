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
 * \brief Implementation of the process_list loader.
 *
 * This object loads the complete list of process identifiers (pid_t)
 * found under /proc. You can further query about many of the parameters
 * of a process using the resulting process_info objects.
 *
 * If interested by a specific process for which you know the pid, you can
 * instead use a process_info object directly.
 */

// self
//
#include    <cppprocess/process_list.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/map_keyset.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <list>


// C
//
#include <signal.h>


// last include
//
#include <snapdev/poison.h>



namespace cppprocess
{



/** \brief The process list constructor determines the list of processes.
 *
 * The constructor builds the list of existing processes from /proc with
 * all the file names that are only digits. Those are the process PIDs.
 * The resulting list is saved in the process_list as proc_info objects
 * which can be quiered for additional information.
 *
 * The additional information is loaded at the time it gets queried which
 * means you may actually get an invalid result (i.e. if the process
 * died before you were able to query its data).
 *
 * You can refresh the list of processes by calling the refresh() function.
 * It will re-read the list of available processes by reading their pid
 * from the `/proc/...` folder.
 */
process_list::process_list()
{
    refresh();
}


/** \brief Refresh the list of processes.
 *
 * If you want to keep a process_list object around for a while, it is
 * going to decay over time (i.e. many processes die and new onces get
 * created).
 *
 * This function refreshes the list of process_info objects defined in
 * this process_list.
 */
void process_list::refresh()
{
    // Keep a copy of the existing keys; if still in that set at the end
    // of the following loop, delete those from the map
    //
    std::set<pid_t> keys;
    snapdev::map_keyset(keys, *this);

    // gather current set of processes (pid_t)
    //
    typedef std::list<std::string>  list_t;
    snapdev::glob_to_list<list_t> filenames;
    filenames.read_path<
          snapdev::glob_to_list_flag_t::GLOB_FLAG_IGNORE_ERRORS
        , snapdev::glob_to_list_flag_t::GLOB_FLAG_ONLY_DIRECTORIES>("/proc/[0-9]*");

    for(auto f : filenames)
    {
        if(f.length() < 7)  // strlen("/proc/[0-9]") == 7
        {
            continue;
        }

        // convert string to integer
        //
        pid_t pid(0);
        char const * s(f.c_str() + 6);
        for(; *s >= '0' && *s <= '9'; ++s)
        {
            pid *= 10;
            pid += *s - '0';
        }
        if(*s != '\0'
        || pid == 0)
        {
            // invalid character or number ("0" is not valid)
            //
            continue;
        }

        // got a pid considered valid, use it
        //
        keys.erase(pid);
        auto it(map::find(pid));
        if(it == end())
        {
            insert({pid, std::make_shared<process_info>(pid)});
        }
    }

    // delete processes from our list if they died
    //
    for(auto k : keys)
    {
        auto it(map::find(k));
        if(it != end())
        {
            erase(it);
        }
    }
}


/** \brief Find a process_info object by pid.
 *
 * This function returns the process info object searching by its process
 * identifier.
 *
 * \return A pointer to the process_info or null.
 */
process_info::pointer_t process_list::find(pid_t pid)
{
    auto it(map::find(pid));
    if(it == end())
    {
        return process_info::pointer_t();
    }

    return it->second;
}


/** \brief Find a process_info object by its process basename.
 *
 * This function goes through the map of processes and returns the
 * first one which name matches the specified \p basename.
 *
 * \return A pointer to a process_info or null.
 */
process_info::pointer_t process_list::find(std::string const & basename)
{
    for(auto & p : *this)
    {
        if(p.second->get_basename() == basename)
        {
            return p.second;
        }
    }

    return process_info::pointer_t();
}


/** \brief Check whether a process is running.
 *
 * When sending a STOP message to a process, or a SIGINT/SIGTERM signal,
 * the process may not stop right away. This function allows you to wait
 * and see that a process ends within a given amount of time.
 *
 * When \p timeout is set to 0, the function returns immediately after
 * checking whether the process is running or not.
 *
 * The \p sig parameter allows you to send a specific signal the first
 * time the kill() function is called. If set to 0, the process is not
 * sent any signal. The function only uses 0 to determine whether the
 * process is running or not.
 *
 * \param[in] pid  The process identifier to wait on.
 * \param[in] sig  Send that signal the first time.
 * \param[in] timeout  The amount of time to wait for the process to quit.
 *
 * \return true if the process is still running after \p timeout.
 */
bool is_running(pid_t pid, int sig, unsigned int timeout)
{
    bool const result(kill(pid, sig) == 0);
    if(timeout == 0 || !result)
    {
        return result;
    }

    time_t const deadline(time(nullptr) + timeout);
    do
    {
        // the kill() function returns immediately so we have to
        // sleep otherwise it would loop very quickly...
        //
        // (I do not know of a way to poll() on a dying process
        // unless it is your direct child or we have a lock...)
        //
        sleep(1);

        if(kill(pid, 0) != 0)
        {
            // the process is dead now
            //
            return false;
        }
    }
    while(deadline > time(nullptr));

    // still running
    //
    return true;
}


} // namespace cppprocess
// vim: ts=4 sw=4 et

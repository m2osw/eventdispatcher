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

// self
//
#include    <cppprocess/process_info.h>


// C++ lib
//
#include    <map>



namespace cppprocess
{



class process_list
    : public std::map<pid_t, process_info::pointer_t>
{
public:
    typedef std::shared_ptr<process_info>      pointer_t;

                                process_list();

    void                        refresh();
    process_info::pointer_t     find(pid_t pid);
    process_info::pointer_t     find(std::string const & basename);
};


bool is_running(pid_t pid, int sig = 0, unsigned int timeout = 0);


} // namespace cppprocess
// vim: ts=4 sw=4 et

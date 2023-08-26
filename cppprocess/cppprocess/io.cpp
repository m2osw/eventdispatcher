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

// self
//
#include    "cppprocess/io.h"

#include    <snapdev/has_member_function.h>


// snapdev
//
#include    <snapdev/not_used.h>


// last include
//
#include    <snapdev/poison.h>




namespace cppprocess
{


io::io(io_flags_t flags)
    : f_flags(flags)
{
}


io::~io()
{
}


io_flags_t io::get_flags() const
{
    return f_flags;
}


io::done_callbacks_t::callback_id_t io::add_process_done_callback(process_io_done_t done)
{
    return f_process_done.add_callback(done);
}


bool io::remove_process_done_callback(done_callbacks_t::callback_id_t id)
{
    return f_process_done.remove_callback(id);
}


void io::process_starting()
{
}


bool io::process_done(done_reason_t reason)
{
//std::cerr << "PROCESS done, reason = " << static_cast<int>(reason)
//<< " -- number of process done: " << f_process_done.size()
//<< "\n";

    // WARNING: we use `this` (instead of make_shared_from_this()) because
    //          the pipe_connection derives from connection which already
    //          derives from make_shared_from_this_enabled and having two
    //          of such fails badly; also if the user needs to keep a
    //          shared pointer, he can do so in his own class or in
    //          the std::bind() directly.
    //
    return f_process_done.call(this, reason);
}


int io::get_fd()
{
    return -1;
}


int io::get_other_fd()
{
    return -1;
}


void io::close_both()
{
}


void io::close_other()
{
}



} // namespace cppprocess
// vim: ts=4 sw=4 et

// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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
#include    <cppprocess/io_pipe.h>

#include    <cppprocess/exception.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace cppprocess
{



io_pipe::io_pipe(io_flags_t flags)
    : io(flags)
    , pipe_connection(flags_to_pipe_mode(flags))
{
}


void io_pipe::process_error()
{
    process_done(done_reason_t::DONE_REASON_ERROR);
}


void io_pipe::process_invalid()
{
    process_done(done_reason_t::DONE_REASON_INVALID);
}


void io_pipe::process_hup()
{
    process_done(done_reason_t::DONE_REASON_HUP);
}


int io_pipe::get_fd()
{
    return get_socket();
}


int io_pipe::get_other_fd()
{
    return get_other_socket();
}


void io_pipe::close_both()
{
    close();
}


void io_pipe::close_other()
{
    forked();
}


void io_pipe::process_starting()
{
    ed::communicator::instance()->add_connection(shared_from_this());
}


bool io_pipe::process_done(done_reason_t reason)
{
    close_both();
    ed::communicator::instance()->remove_connection(shared_from_this());
    return io::process_done(reason);
}


ed::pipe_t io_pipe::flags_to_pipe_mode(io_flags_t flags)
{
    switch(flags & (IO_FLAG_INPUT | IO_FLAG_OUTPUT))
    {
    case IO_FLAG_INPUT | IO_FLAG_OUTPUT:
        return ed::pipe_t::PIPE_BIDIRECTIONAL;

    case IO_FLAG_INPUT:
        return ed::pipe_t::PIPE_CHILD_INPUT;

    case IO_FLAG_OUTPUT:
        return ed::pipe_t::PIPE_CHILD_OUTPUT;

    default:
        throw cppprocess_invalid_parameters("io_pipe flags must be set to INPUT, OUTPUT, or both; neither is not an available option");

    }
    snapdev::NOT_REACHED();
    return ed::pipe_t::PIPE_BIDIRECTIONAL;
}



} // namespace cppprocess
// vim: ts=4 sw=4 et

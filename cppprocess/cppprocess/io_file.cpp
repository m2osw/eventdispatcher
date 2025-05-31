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
#include    <cppprocess/io_file.h>

#include    <cppprocess/exception.h>


// last include
//
#include    <snapdev/poison.h>



namespace cppprocess
{



io_file::io_file(io_flags_t flags)
    : io(flags)
{
}


void io_file::set_filename(std::string const & filename)
{
    if(f_file != nullptr)
    {
        throw cppprocess_in_use("io_file is already in use, filename cannot be updated.");
    }

    f_filename = filename;
}


std::string const & io_file::get_filename() const
{
    return f_filename;
}


void io_file::set_truncate(bool truncate)
{
    if(f_file != nullptr)
    {
        throw cppprocess_in_use("io_file is already in use, truncate flag cannot be updated.");
    }

    f_truncate = truncate;
}


bool io_file::get_truncate() const
{
    return f_truncate;
}


void io_file::set_append(bool append)
{
    if(f_file != nullptr)
    {
        throw cppprocess_in_use("io_file is already in use, append flag cannot be updated.");
    }

    f_append = append;
}


bool io_file::get_append() const
{
    return f_append;
}


void io_file::set_mode(int mode)
{
    if(f_file != nullptr)
    {
        throw cppprocess_in_use("io_file is already in use, mode cannot be updated.");
    }

    f_mode = mode;
}


int io_file::get_mode() const
{
    return f_mode;
}


int io_file::get_fd()
{
    return f_file.get();
}


int io_file::get_other_fd()
{
    return get_fd();
}


void io_file::close_both()
{
    f_file.reset();
}


void io_file::close_other()
{
    // TBD: do we have to do something here?
}


void io_file::process_starting()
{
    if(f_file == nullptr)
    {
        int flags(0);
        io_flags_t const io_flags(get_flags());
        if((io_flags & (IO_FLAG_INPUT | IO_FLAG_OUTPUT))
                                == (IO_FLAG_INPUT | IO_FLAG_OUTPUT))
        {
            flags |= O_RDWR;
        }
        else if((io_flags & IO_FLAG_INPUT) != 0)
        {
            flags |= O_RDONLY;
        }
        else if((io_flags & IO_FLAG_OUTPUT) != 0)
        {
            flags |= O_WRONLY | O_CREAT;
        }

        if((io_flags & IO_FLAG_OUTPUT) != 0)
        {
            if(f_truncate)
            {
                flags |= O_TRUNC;
            }
            if(f_append)
            {
                flags |= O_APPEND;
            }
        }

        f_file.reset(open(f_filename.c_str(), flags, f_mode));
    }
}



} // namespace cppprocess
// vim: ts=4 sw=4 et

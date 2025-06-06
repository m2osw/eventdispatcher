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
#pragma once

// self
//
#include    <cppprocess/io_pipe.h>
#include    <cppprocess/buffer.h>



namespace cppprocess
{



// pre-defined output pipe
class io_capture_pipe
    : public io_pipe
{
public:
    typedef std::shared_ptr<io_capture_pipe>
                            pointer_t;

                            io_capture_pipe();

    // pipe_connection implementation
    //
    virtual void            process_read() override;

    std::string             get_output(bool reset = false) const;
    std::string             get_trimmed_output(bool inside = false, bool reset = false) const;
    buffer_t                get_binary_output(bool reset = false) const;

private:
    buffer_t                f_output = buffer_t();
};



} // namespace cppprocess
// vim: ts=4 sw=4 et

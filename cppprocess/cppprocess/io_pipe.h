// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <cppprocess/io.h>


// eventdispatcher
//
#include    <eventdispatcher/pipe_connection.h>



namespace cppprocess
{



// base pipe
class io_pipe
    : public io
    , public ed::pipe_connection
{
public:
                            io_pipe(io_flags_t flags);

    // pipe_connection implementation
    //
    virtual void            process_error() override;
    virtual void            process_invalid() override;
    virtual void            process_hup() override;

    // io implementation
    //
    virtual int             get_fd() override;
    virtual int             get_other_fd() override;
    virtual void            close_both() override;
    virtual void            close_other() override;
    virtual void            process_starting() override;
    virtual bool            process_done(done_reason_t reason) override;

    static ed::pipe_t       flags_to_pipe_mode(io_flags_t flags);

private:
};



} // namespace cppprocess
// vim: ts=4 sw=4 et

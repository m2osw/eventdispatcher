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
#pragma once

// self
//
#include    <cppprocess/io.h>


// snapdev lib
//
#include    <snapdev/raii_generic_deleter.h>


// C lib
//
#include    <fcntl.h>



namespace cppprocess
{



class io_file
    : public io
{
public:
                            io_file(io_flags_t flags);

    void                    set_filename(std::string const & filename);
    std::string const &     get_filename() const;

    void                    set_truncate(bool truncate);
    bool                    get_truncate() const;
    void                    set_append(bool append);
    bool                    get_append() const;

    void                    set_mode(int mode);
    int                     get_mode() const;

    // io implementation
    //
    virtual int             get_fd() override;
    virtual int             get_other_fd() override;
    virtual void            close_both() override;
    virtual void            close_other() override;
    virtual void            process_starting() override;

private:
    std::string             f_filename = std::string();
    bool                    f_truncate = false;
    bool                    f_append = false;
    int                     f_mode = S_IRUSR | S_IWUSR;
    snapdev::raii_fd_t      f_file = snapdev::raii_fd_t();
};



} // namespace cppprocess
// vim: ts=4 sw=4 et

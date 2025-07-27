// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// self
//
#include    <cppprocess/io_pipe.h>
#include    <cppprocess/buffer.h>



namespace cppprocess
{



class io_data_pipe
    : public io_pipe
{
public:
    typedef std::shared_ptr<io_data_pipe>
                            pointer_t;

                            io_data_pipe();

    void                    add_input(std::string const & input);
    void                    add_input(buffer_t const & input);
    std::string             get_input(bool reset = false) const;
    buffer_t                get_binary_input(bool reset = false) const;

    // pipe_connection implementation
    //
    virtual bool            is_writer() const override;
    virtual void            process_write() override;

private:
    buffer_t                f_input = buffer_t();
    std::size_t             f_pos = 0;
};



} // namespace cppprocess
// vim: ts=4 sw=4 et

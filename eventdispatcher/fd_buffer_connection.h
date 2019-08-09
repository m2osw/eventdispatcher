// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    "eventdispatcher/fd_connection.h"



namespace ed
{



class fd_buffer_connection
    : public fd_connection
{
public:
    typedef std::shared_ptr<fd_buffer_connection>    pointer_t;

                                fd_buffer_connection(int fd, mode_t mode);

    bool                        has_input() const;
    bool                        has_output() const;
    virtual bool                is_writer() const override;

    // snap_fd_connection implementation
    virtual ssize_t             write(void const * data, size_t const length) override;

    // snap_connection implementation
    virtual void                process_read() override;
    virtual void                process_write() override;
    virtual void                process_hup() override;

    // new callback
    virtual void                process_line(std::string const & line) = 0;

private:
    std::string                 f_line = std::string(); // input -- do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
    std::vector<char>           f_output = std::vector<char>();
    size_t                      f_position = 0;
};



} // namespace ed
// vim: ts=4 sw=4 et

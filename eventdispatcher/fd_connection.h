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
#include    "eventdispatcher/connection.h"



namespace ed
{



class fd_connection
    : public connection
{
public:
    typedef std::shared_ptr<fd_connection>         pointer_t;

    enum class mode_t
    {
        FD_MODE_READ,
        FD_MODE_WRITE,
        FD_MODE_RW
    };

                                fd_connection(int fd, mode_t mode);

    void                        close();
    void                        mark_closed();

    // snap_connection implementation
    virtual bool                is_reader() const override;
    virtual bool                is_writer() const override;
    virtual int                 get_socket() const override;

    // new callbacks
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);

private:
    int                         f_fd = -1;
    mode_t                      f_mode = mode_t::FD_MODE_RW;
};



} // namespace ed
// vim: ts=4 sw=4 et

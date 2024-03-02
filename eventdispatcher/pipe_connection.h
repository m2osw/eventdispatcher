// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/connection.h>


// C
//
#include    <sys/types.h>




namespace ed
{



enum class pipe_t
{
    PIPE_BIDIRECTIONAL,     // use an AF_UNIX / AF_LOCAL
    PIPE_CHILD_INPUT,       // FIFO, parent writes/child reads
    PIPE_CHILD_OUTPUT,      // FIFO, child writes/parent reads
};


class pipe_connection
    : public connection
{
public:
    typedef std::shared_ptr<pipe_connection>    pointer_t;
    typedef std::vector<pointer_t>              vector_t;

                                pipe_connection(pipe_t type = pipe_t::PIPE_BIDIRECTIONAL);
    virtual                     ~pipe_connection() override;

    pipe_t                      type() const;
    int                         get_other_socket() const;

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;

    // new callbacks
    virtual ssize_t             read(void * buf, size_t count);
    virtual ssize_t             write(void const * buf, size_t count);
    virtual void                forked();
    virtual void                close();

private:
    pipe_t                      f_type = pipe_t::PIPE_BIDIRECTIONAL;
    pid_t                       f_parent = -1;  // the process that created these pipes (read/write to 0 if getpid() == f_parent, read/write to 1 if getpid() != f_parent)
    int                         f_socket[2] = { -1, -1 };    // socket pair
};



} // namespace ed
// vim: ts=4 sw=4 et

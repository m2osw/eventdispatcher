// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "eventdispatcher/connection.h"



namespace ed
{



// WARNING: a snap_communicator object must be allocated and held in a shared pointer (see pointer_t)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class communicator
    : public std::enable_shared_from_this<communicator>
{
public:
    typedef std::shared_ptr<communicator>       pointer_t;

    static pointer_t                    instance();

    connection::vector_t const &        get_connections() const;
    bool                                add_connection(connection::pointer_t connection);
    bool                                remove_connection(connection::pointer_t connection);
    void                                set_force_sort(bool status = true);

    virtual bool                        run();

private:
                                        communicator();
                                        communicator(communicator const & communicator) = delete;

    communicator &                      operator = (communicator const & communicator) = delete;

    connection::vector_t                f_connections = connection::vector_t();
    bool                                f_force_sort = true;
};
#pragma GCC diagnostic pop


} // namespace snap
// vim: ts=4 sw=4 et

// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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

#include "eventdispatcher/connection.h"
#include "eventdispatcher/exception.h"



namespace ed
{


DECLARE_EXCEPTION(event_dispatcher_exception, event_dispatcher_no_connection_found);


class qt_connection
    : public connection
{
public:
    typedef std::shared_ptr<qt_connection> pointer_t;

                                qt_connection();
    virtual                     ~qt_connection() override;

    // implements connection
    virtual int                 get_socket() const override;
    virtual bool                is_reader() const override;
    virtual void                process_read() override;

private:
    int                         f_fd = -1;
};


} // namespace ed
// vim: ts=4 sw=4 et

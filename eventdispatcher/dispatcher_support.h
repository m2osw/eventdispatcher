// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/dispatcher_base.h"



namespace ed
{



class dispatcher_support
{
public:
    virtual                     ~dispatcher_support();

    void                        set_dispatcher(dispatcher_base::pointer_t d);
    dispatcher_base::pointer_t  get_dispatcher() const;
    bool                        dispatch_message(message & msg);

    // new callback
    virtual void                process_message(message const & message);

private:
    dispatcher_base::weak_t     f_dispatcher = dispatcher_base::weak_t();
};



} // namespace ed
// vim: ts=4 sw=4 et

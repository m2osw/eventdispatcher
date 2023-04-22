// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Event dispatch support class.
 *
 * This class is used on connections that support the dispatcher. This means
 * those classes support messaging as defined by the eventdispatcher library.
 */


// self
//
#include    <eventdispatcher/message.h>



namespace ed
{


class dispatcher;
typedef std::shared_ptr<dispatcher>     dispatcher_pointer_t;
typedef std::weak_ptr<dispatcher>       dispatcher_weak_t;


class dispatcher_support
{
public:
    typedef std::shared_ptr<dispatcher_support> pointer_t;

    virtual                     ~dispatcher_support();

    void                        set_dispatcher(dispatcher_pointer_t d);
    dispatcher_pointer_t        get_dispatcher() const;

    // new callbacks
    //
    virtual bool                dispatch_message(message & msg);
    virtual void                process_message(message & msg);

private:
    dispatcher_weak_t           f_dispatcher = dispatcher_weak_t();
};



} // namespace ed
// vim: ts=4 sw=4 et

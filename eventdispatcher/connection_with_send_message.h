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
#include    "eventdispatcher/message.h"



namespace ed
{



class connection_with_send_message
{
public:
    virtual                     ~connection_with_send_message();

    // new callback
    virtual bool                send_message(message const & message, bool cache = false) = 0;

    virtual void                msg_help(message & message);
    virtual void                msg_alive(message & message);
    virtual void                msg_log(message & message);
    virtual void                msg_quitting(message & message);
    virtual void                msg_ready(message & message);
    virtual void                msg_restart(message & message);
    virtual void                msg_stop(message & message);
    virtual void                msg_log_unknown(message & message);
    virtual void                msg_reply_with_unknown(message & message);

    virtual void                help(string_list_t & commands);
    virtual void                ready(message & message);
    virtual void                stop(bool quitting);
};



} // namespace ed
// vim: ts=4 sw=4 et

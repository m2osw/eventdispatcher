// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to mark your class as one that can send messages. It
 * also handles system defined events.
 */

// self
//
#include    <eventdispatcher/message.h>


// libaddr
//
#include    <libaddr/addr.h>


// C++
//
#include    <list>



namespace ed
{



class connection_with_send_message
{
public:
    typedef std::shared_ptr<connection_with_send_message>
                                pointer_t;
    typedef std::weak_ptr<connection_with_send_message>
                                weak_t;
    typedef std::list<weak_t>   list_weak_t;

                                connection_with_send_message(std::string const & service_name = std::string());
    virtual                     ~connection_with_send_message();

    // new callbacks
    virtual bool                send_message(message & msg, bool cache = false) = 0;

    virtual void                msg_help(message & msg);
    virtual void                msg_alive(message & msg);
    virtual void                msg_leak(message & msg);
    virtual void                msg_log_rotate(message & msg);
    virtual void                msg_quitting(message & msg);
    virtual void                msg_ready(message & msg);
    virtual void                msg_restart(message & msg);
    virtual void                msg_service_unavailable(message & msg);
    virtual void                msg_stop(message & msg);
    virtual void                msg_log_unknown(message & msg);
    virtual void                msg_reply_with_unknown(message & msg);

    virtual void                help(string_list_t & commands);
    virtual void                ready(message & msg);
    virtual void                restart(message & msg);
    virtual void                stop(bool quitting);

    std::string                 get_service_name(bool required = false) const;
    bool                        is_ready() const;
    addr::addr                  get_my_address() const;
    bool                        register_service();
    void                        unregister_service();

private:
    std::string                 f_service_name = std::string();
    bool                        f_ready = false;
    addr::addr                  f_my_address = addr::addr();
};



} // namespace ed
// vim: ts=4 sw=4 et

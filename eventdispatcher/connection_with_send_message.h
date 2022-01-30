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

    // new callbacks
    virtual bool                send_message(message const & msg, bool cache = false) = 0;

    virtual void                msg_help(message & msg);
    virtual void                msg_alive(message & msg);
    virtual void                msg_leak(message & msg);
    virtual void                msg_log(message & msg);
    virtual void                msg_quitting(message & msg);
    virtual void                msg_ready(message & msg);
    virtual void                msg_restart(message & msg);
    virtual void                msg_stop(message & msg);
    virtual void                msg_log_unknown(message & msg);
    virtual void                msg_reply_with_unknown(message & msg);

    virtual void                help(string_list_t & commands);
    virtual void                ready(message & msg);
    virtual void                restart(message & msg);
    virtual void                stop(bool quitting);

    bool                        get_last_send_status() const;

    /** \brief Broadcast a message to a set of connections.
     *
     * This function sends one message to all the connections found in a
     * container set. The type of the container is not specified. It can
     * be any container which works with `for(auto ...)`.
     *
     * Note that this function should only be used with connections that
     * do not automatically allow for broadcasting such as UDP. This is
     * useful to send messages to a plethora of connections such as TCP
     * or Unix sockets.
     *
     * \tparam C  A connection container.
     *
     * \param[in] container  A set of connections (vector, map, set, etc.)
     * \param[in] msg  The message to broadcast.
     * \param[in] cache  Whether to cache the message if the connection is not
     * currently opened.
     *
     * \return true if the send_message() succeeded on all connections, false
     * otherwise. If false was returned, you can use the get_last_send_status()
     * function to discover which connections failed.
     */
    template<class C>
    bool broadcast_message(C & container, message const & msg, bool cache = false)
    {
        bool result(true);
        for(auto c : container)
        {
            f_last_send_status = c->send_message(msg, cache);
            if(!f_last_send_status)
            {
                result = false;
            }
        }
        return result;
    }

private:
    bool        f_last_send_status = true;
};



} // namespace ed
// vim: ts=4 sw=4 et

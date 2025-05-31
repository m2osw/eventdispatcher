// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Function used to broadcast a message to a list of messengers.
 *
 * If you have a service which handles many connections and you want to
 * broadcast a message to all of these connections at once, use the
 * ed::broadcast_message() function.
 *
 * The input is a container of connections. The container can either
 * have a list of shared pointers or weak pointers.
 */

// self
//
#include    <eventdispatcher/message.h>



namespace ed
{


/** \brief Broadcast a message to a set of connections.
 *
 * This function sends one message to all the connections found in a
 * container. The type of the container is not specified although it
 * must contain ed::connection shared pointers with a type of connection
 * that supports send_message().
 *
 * \todo
 * If we need to know which connections fail the send_message(), then
 * we want to look into offering a callback.
 *
 * \tparam C  A connection container.
 *
 * \param[in] container  A set of connections (vector, map, set, etc.)
 * \param[in] msg  The message to broadcast.
 * \param[in] cache  Whether to cache the message if the connection is not
 * currently opened.
 *
 * \return true if the send_message() succeeded on all connections, false
 * otherwise.
 */
template<typename C, typename T = typename C::value_type>
typename std::enable_if<std::is_same<
              T
            , std::shared_ptr<typename T::element_type>>::value
        , bool>::type
broadcast_message(C & container, message & msg, bool cache = false)
{
    bool result(true);
    for(auto c : container)
    {
        if(c != nullptr)
        {
            result = c->send_message(msg, cache) && result;
        }
    }
    return result;
}


/** \brief Broadcast a message to a set of connections.
 *
 * This function sends one message to all the connections found in a
 * container. The type of the container is not specified although it
 * must contain ed::connection weak pointers with a type of connection
 * that supports send_message().
 *
 * Further, this function removes any entry in the container which
 * is an expired weak pointer.
 *
 * Here is an example of usage where we use a list of weak pointers
 * from the connection_with_send_message class:
 *
 * \code
 *     ed::connection_with_send_message::list_weak_t list;
 *     ...
 *     // when you have new connections
 *     ed::communicator::instance()->add_connection(connection);
 *     list.push_back(connection);
 *     ...
 *     // when you lose a connections
 *     ed::communicator::instance()->remove_connection(connection);
 *     ...
 *     // when you want to broadcast a message
 *     ed::broadcast_message(list, msg);
 * \endcode
 *
 * Whenever you are done with a connection, you simply remove it from
 * the communicator and it gets removed from that list the next time
 * you call broadcast_message().
 *
 * \todo
 * If we need to know which connections fail the send_message(), then
 * we want to look into offering a callback.
 *
 * \tparam C  A connection container.
 *
 * \param[in] container  A set of connections (vector, map, set, etc.)
 * \param[in] msg  The message to broadcast.
 * \param[in] cache  Whether to cache the message if the connection is not
 * currently opened.
 *
 * \return true if the send_message() succeeded on all connections, false
 * otherwise.
 */
template<typename C, typename T = typename C::value_type>
typename std::enable_if<std::is_same<
              T
            , std::weak_ptr<typename T::element_type>>::value
        , bool>::type
broadcast_message(C & container, message & msg, bool cache = false)
{
    bool result(true);

    auto c(container.begin());
    while(c != container.end())
    {
        auto p(c->lock());
        if(p == nullptr)
        {
            c = container.erase(c);
        }
        else
        {
            result = p->send_message(msg, cache) && result;
            ++c;
        }
    }

    return result;
}



} // namespace ed
// vim: ts=4 sw=4 et

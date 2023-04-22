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
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/dispatcher_support.h>
#include    <eventdispatcher/timer.h>


// libaddr
//
#include    <libaddr/addr_unix.h>



namespace ed
{


namespace detail
{
class local_stream_client_permanent_message_connection_impl;
}
// namespace detail



class local_stream_client_permanent_message_connection
    : public timer
    , public dispatcher_support
    , public connection_with_send_message
{
public:
    typedef std::shared_ptr<local_stream_client_permanent_message_connection>    pointer_t;

    static std::int64_t const   DEFAULT_PAUSE_BEFORE_RECONNECTING = 60LL * 1000000LL;  // 1 minute

                                local_stream_client_permanent_message_connection(
                                          addr::addr_unix const & address
                                        , std::int64_t const pause = DEFAULT_PAUSE_BEFORE_RECONNECTING
                                        , bool const use_thread = true
                                        , bool const blocking = false
                                        , bool const close_on_exec = true
                                        , std::string const & service_name = std::string());
    virtual                     ~local_stream_client_permanent_message_connection() override;

    bool                        is_connected() const;
    void                        disconnect();
    void                        mark_done();
    void                        mark_done(bool messenger);
    addr::addr_unix             get_address() const;

    // connection_with_send_message implementation
    virtual bool                send_message(message & msg, bool cache = false) override;

    // connection implementation
    virtual void                process_timeout() override;
    virtual void                process_error() override;
    virtual void                process_hup() override;
    virtual void                process_invalid() override;
    virtual void                connection_removed() override;

    // new callbacks
    virtual void                process_connection_failed(std::string const & error_message);
    virtual void                process_connected();

private:
    std::shared_ptr<detail::local_stream_client_permanent_message_connection_impl>
                                f_impl = std::shared_ptr<detail::local_stream_client_permanent_message_connection_impl>();
    std::int64_t                f_pause = 0;
    bool const                  f_use_thread = true;
};



} // namespace ed
// vim: ts=4 sw=4 et

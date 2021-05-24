// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/** \file
 * \brief The implementation of the TCP appender.
 *
 * This file implements the sending of log messages via TCP.
 */

// self
//
#include    "snaplogger/network/tcp_appender.h"



// snaplogger lib
//
#include    "snaplogger/guard.h"


// eventdispatcher lib
//
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


namespace
{


APPENDER_FACTORY(tcp);



class appender_connection
    : public ed::tcp_client_permanent_message_connection
{
public:
                            appender_connection(addr::addr const & server_address);
    virtual                 ~appender_connection() override;

    void                    msg_pause(ed::message & msg);
    void                    msg_unpause(ed::message & msg);

    bool                    is_paused() const;

private:
    ed::dispatcher<appender_connection>::pointer_t
                            f_dispatcher = ed::dispatcher<appender_connection>::pointer_t();
    bool                    f_paused = false;
};


ed::dispatcher<appender_connection>::dispatcher_match::vector_t const g_appender_connection_messages =
{
    {
        "PAUSE"
      , &appender_connection::msg_pause
    },
    {
        "UNPAUSE"
      , &appender_connection::msg_unpause
    },

    // ALWAYS LAST
    {
        nullptr
      , &appender_connection::msg_reply_with_unknown
      , &ed::dispatcher<appender_connection>::dispatcher_match::always_match
    }
};



appender_connection::appender_connection(addr::addr const & server_address)
    : tcp_client_permanent_message_connection(
              server_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
            , server_address.get_port())
    , f_dispatcher(new ed::dispatcher<appender_connection>(
              this
            , g_appender_connection_messages))
{
    set_name("tcp-appender-connection");
//#ifdef _DEBUG
//    f_dispatcher->set_trace();
//#endif
    set_dispatcher(f_dispatcher);
}


appender_connection::~appender_connection()
{
}


void appender_connection::msg_pause(ed::message & msg)
{
    snap::NOTUSED(msg);

    f_paused = true;
}


void appender_connection::msg_unpause(ed::message & msg)
{
    snap::NOTUSED(msg);

    f_paused = false;
}


bool appender_connection::is_paused() const
{
    return f_paused;
}





}
// no name namespace



tcp_appender::tcp_appender(std::string const & name)
    : base_network_appender(name, "tcp")
    , f_communicator(ed::communicator::instance())
{
}


tcp_appender::~tcp_appender()
{
}


void tcp_appender::set_config(advgetopt::getopt const & opts)
{
    base_network_appender::set_config(opts);

    // COMPRESSION
    //
    std::string const compression_field(get_name() + "::compression");
    if(opts.is_defined(compression_field))
    {
        std::string const compression(opts.get_string(compression_field));
        if(compression == "message")
        {
            f_compression = compression_t::COMPRESSION_PER_MESSAGE;
        }
        else if(compression == "blocks")
        {
            f_compression = compression_t::COMPRESSION_BLOCKS;
        }
        else
        {
            f_compression = compression_t::COMPRESSION_NONE;
        }
    }
}


void tcp_appender::server_address_changed()
{
    f_communicator->remove_connection(f_connection);
    f_connection.reset();
}


void tcp_appender::process_message(snaplogger::message const & msg, std::string const & formatted_message)
{
    snaplogger::guard g;

    ed::message log_message;
    log_message_to_ed_message(msg, log_message);

    // TODO: handle the possible compression
    //
    // 1. Grouping
    //
    //    The grouping will also require a path/filename for the
    //    temporary data
    //
    // 2. Compression Method
    //
    //    We should offer a method such as gz, bz, or xz
    //
    // 3. Compression Level
    //
    //    We should offer a level (1 to 9).
    //
    // 4. Thread
    //
    //    We should offer the ability to run in a thread (I think the core
    //    library already does that part so probably useless here?)

    if(f_connection == nullptr)
    {
        f_connection = std::make_shared<appender_connection>(f_server_address);
        if(!f_communicator->add_connection(f_connection))
        {
            f_connection.reset();
        }
    }

    // currently paused?
    //
    if(f_connection != nullptr
    && std::static_pointer_cast<appender_connection>(f_connection)->is_paused())
    {
        return;
    }

    // send message via TCP
    //
    if(f_connection == nullptr
    || !std::static_pointer_cast<appender_connection>(f_connection)->send_message(log_message, true))
    {
        // how could we report that? we are the logger...
        //
        if(f_fallback_to_console
        && isatty(fileno(stdout)))
        {
            std::cout << formatted_message.c_str();
        }
    }
}





} // snaplogger_network namespace
// vim: ts=4 sw=4 et

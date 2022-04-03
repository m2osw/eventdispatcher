// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the remote snapcommunicator connection.
 *
 * This is the implementation of the remote snapcommunicator connection.
 * Connection used to communicate with other snapcommunicators running
 * on other servers.
 */

// self
//
#include    "remote_snapcommunicator.h"



namespace sc
{



/** \class remote_snapcommunicator
 * \brief Describe a remove snapcommunicator by IP address, etc.
 *
 * This class defines a snapcommunicator server. Mainly we include
 * the IP address of the server to connect to.
 *
 * The object also maintains the status of that server. Whether we
 * can connect to it (because if not the connection stays in limbo
 * and we should not try again and again forever. Instead we can
 * just go to sleep and try again "much" later saving many CPU
 * cycles.)
 *
 * It also gives us a way to quickly track snapcommunicator objects
 * that REFUSE our connection.
 */


/** \brief Setup a remote_snapcommunicator object.
 *
 * This initialization function sets up the attached snap_timer
 * to 1 second delay before we try to connect to this remote
 * snapcommunicator. The timer is reused later when the connection
 * is lost, a snapcommunicator returns a REFUSE message to our
 * CONNECT message, and other similar errors.
 *
 * \param[in] cs  The snap communicator server shared pointer.
 * \param[in] addr  The address to connect to.
 */
remote_snapcommunicator::remote_snapcommunicator(
              server::pointer_t cs
            , addr::addr const & address)
    : tcp_client_permanent_message_connection(
              address
            , cs->connection_mode()
            , REMOTE_CONNECTION_DEFAULT_TIMEOUT)
    , base_connection(cs)
    , f_address(addr::string_to_addr(addr.toUtf8().data(), "", 4040, "tcp"))
{
}


remote_snapcommunicator::~remote_snapcommunicator()
{
    try
    {
        SNAP_LOG_DEBUG
            << "deleting remote_snapcommunicator connection: "
            << f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
            << SNAP_LOG_SEND;
    }
    catch(addr::addr_invalid_argument const &)
    {
    }
}


void remote_snapcommunicator::process_message(ed::message const & message)
{
    if(f_server_name.empty())
    {
        f_server_name = message.get_sent_from_server();
    }

    f_server->process_message(shared_from_this(), message, false);
}


void remote_snapcommunicator::process_connection_failed(std::string const & error_message)
{
    tcp_client_permanent_message_connection::process_connection_failed(error_message);

    SNAP_LOG_ERROR
        << "the connection to a remote communicator failed: \""
        << error_message
        << "\"."
        << SNAP_LOG_SEND;

    // were we connected? if so this is a hang up
    //
    if(f_connected
    && !f_server_name.empty())
    {
        f_connected = false;

        snap::snap_communicator_message hangup;
        hangup.set_command("HANGUP");
        hangup.set_service(".");
        hangup.add_parameter("server_name", f_server_name);
        f_communicator_server->broadcast_message(hangup);
    }

    // we count the number of failures, after a certain number we raise a
    // flag so that way the administrator is warned about the potential
    // problem; we take the flag down if the connection comes alive
    //
    if(f_failures <= 0)
    {
        f_failure_start_time = time(nullptr);
        f_failures = 1;
    }
    else if(f_failures < std::numeric_limits<decltype(f_failures)>::max())
    {
        ++f_failures;
    }

    // a remote connection can have one of three timeouts at this point:
    //
    // 1. one minute between attempts, this is the default
    // 2. five minutes between attempts, this is used when we receive a
    //    DISCONNECT, leaving time for the remote computer to finish
    //    an update or reboot
    // 3. one whole day between attemps, when the remote computer sent
    //    us a "Too Busy" error
    //
    // so... we want to generate an error when:
    //
    // (a) we made at least 20 attempts
    // (b) at least one hour went by
    // (c) each attempt resulted in an error
    //
    // note that 20 attempts in the default case [(1) above] represents
    // about 20 min.; and 20 attempts in the seconds case [(2) above]
    // represents 100 min. which is nearly two hours; however, the
    // "Too Busy" error means we'd wait 20 days before flagging that
    // computer as dead, this may be of concern?
    //
    time_t const time_elapsed(time(nullptr) - f_failure_start_time);
    if(!f_flagged
    && f_failures >= 20
    && time_elapsed > 60LL * 60LL)
    {
        f_flagged = true;

        std::stringstream ss;

        ss << "connecting to "
           << f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
           << ", failed "
           << std::to_string(f_failures)
           << " times in a row for "
           << std::setfill('0')
           << std::setw(2)
           << ((time_elapsed / 1000000LL / 60 / 60) % 24)
           << ":"
           << std::setw(2)
           << ((time_elapsed / 1000000LL / 60) % 60)
           << ":"
           << std::setw(2)
           << ((time_elapsed / 1000000LL) % 60)
           << " (HH:MM:SS), please verify this IP address"
              " and that it is expected that the computer"
              " fails connecting. If not, please remove that IP address"
              " from the list of neighbors AND THE FIREWALL if it is there too.";

        snap::snap_flag::pointer_t flag(SNAP_FLAG_UP(
                      "snapcommunicator"
                    , "remote-connection"
                    , "connection-failed"
                    , ss.str()
                ));
        flag->set_priority(95);
        flag->add_tag("security");
        flag->add_tag("data-leak");
        flag->add_tag("network");
        flag->save();
    }
}


void remote_snapcommunicator::process_connected()
{
    f_connected = true;

    // take the remote connection failure flag down
    //
    // Note: by default we set f_failures to -1 so when we reach here we
    //       get the flag down once; after that, we take the flag down
    //       only if we raised it in between, that way we save some time
    //
    if(f_failures != 0
    || f_failure_start_time != 0
    || f_flagged)
    {
        f_failure_start_time = 0;
        f_failures = 0;
        f_flagged = false;

        snap::snap_flag::pointer_t flag(SNAP_FLAG_DOWN(
                           "snapcommunicator"
                         , "remote-connection"
                         , "connection-failed")
                     );
        flag->save();
    }

    tcp_client_permanent_message_connection::process_connected();

    f_communicator_server->process_connected(shared_from_this());

    // reset the wait to the default 5 minutes
    //
    // (in case we had a shutdown event from that remote communicator
    // and changed the timer to 15 min.)
    //
    // later we probably want to change the mechanism if we want to
    // slowdown over time
    //
    set_timeout_delay(REMOTE_CONNECTION_DEFAULT_TIMEOUT);
}


addr::addr const & remote_snapcommunicator::get_address() const
{
    return f_address;
}



} // namespace sc
// vim: ts=4 sw=4 et

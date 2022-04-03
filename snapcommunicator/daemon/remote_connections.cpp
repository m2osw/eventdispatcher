// Snap Websites Server -- server to handle inter-process communication
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
 * \brief Implementation of the remote server connection.
 *
 * The Snap! Communicator has three types of connections:
 *
 * * this communicator to a remote communicator
 * * a remote communicator to this communicator
 * * local clients
 *
 * The remote connection handles connections from this communicator to
 * a remote communicator.
 */

// self
//
#include    "remote_connections.h"


//// snapwebsites lib
////
//#include <snapwebsites/chownnm.h>
//#include <snapwebsites/flags.h>
//#include <snapwebsites/glob_dir.h>
//#include <snapwebsites/loadavg.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/qcompatibility.h>
//#include <snapwebsites/snap_communicator.h>
//#include <snapwebsites/snapwebsites.h>
//
//
//// snapdev lib
////
//#include <snapdev/not_used.h>
//#include <snapdev/tokenize_string.h>
//
//
//// libaddr lib
////
//#include <libaddr/addr_exception.h>
//#include <libaddr/addr_parser.h>
//#include <libaddr/iface.h>
//
//
//// Qt lib
////
//#include <QFile>
//
//
//// C++ lib
////
//#include <atomic>
//#include <cmath>
//#include <fstream>
//#include <iomanip>
//#include <sstream>
//#include <thread>
//
//
//// C lib
////
//#include <grp.h>
//#include <pwd.h>
//#include <sys/resource.h>


// included last
//
#include <snapdev/poison.h>







namespace sc
{



remote_connections::remote_connections(
              server::pointer_t server
            , addr::addr const & my_addr)
    : f_server(server)
    , f_my_address(my_addr)
{
}



addr::addr remote_connections::get_my_address() const
{
    return f_my_address;
}


void remote_connections::add_remote_communicator(std::string const & addr_port)
{
    SNAP_LOG_DEBUG
        << "adding remote communicator at "
        << addr_port
        << SNAP_LOG_SEND;

    // no default address for neighbors
    //
    addr::addr remote_addr(
            addr::string_to_addr(addr_port, std::string(), 4040, "tcp"));

    if(remote_addr == f_my_address)
    {
        // TBD: this may be normal (i.e. neighbors should send us our IP
        //      right back to us!)
        //
        SNAP_LOG_WARNING
            << "address of remote snapcommunicator, \""
            << addr_port
            << "\", is the same as my address, which means it is not remote."
            << SNAP_LOG_SEND;
        return;
    }

    std::string const addr(remote_addr.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS));
    int const port(remote_addr.get_port());

    // was this address already added
    //
    // TODO: use addr::addr objects in the map and the == operator
    //       will then use the one from addr::addr (and not a string)
    //
    if(f_all_ips.find(addr))
    {
        if(remote_addr < f_my_address)
        {
            // make sure it is defined!
            //
            auto it(f_smaller_ips.find(remote_addr));
            if(it != f_smaller_ips.end())
            {
                // if not currently connected, we want to try reconnecting
                //
                // (if there is a mistake, we could still be disabled here
                // when we should have been enabled, although it should not
                // be the case, do not take any chances)
                //
                if(!it->is_connected())
                {
                    // reset that timer to run ASAP in case the timer is enabled
                    //
                    // just in case, we reset the timeout as well, we want to
                    // do it since we are back in business now
                    //
                    it->set_timeout_delay(remote_snapcommunicator::REMOTE_CONNECTION_TOO_BUSY_TIMEOUT);
                    it->set_timeout_date(time(nullptr) * 1'000'000LL);
                    it->set_enable(true);
                }
            }
            else
            {
                SNAP_LOG_ERROR
                    << "smaller remote address is defined in f_all_ips but not in f_smaller_ips?"
                    << SNAP_LOG_SEND;
            }
        }
        else
        {
            // we may already be GOSSIP-ing about this one (see below)
            //
            SNAP_LOG_DEBUG("new remote connection ")(addr_port)(" has a larger address than us. This is a GOSSIP channel.");
        }
        return;
    }

    // keep a copy of all addresses
    //
    f_all_ips[addr] = port;

    // if this new IP is smaller than ours, then we start a connection
    //
    if(remote_addr < f_my_address)
    {
        // smaller connections are created as remote snap communicator
        // which are permanent message connections
        //
        remote_snapcommunicator::pointer_t remote_communicator(std::make_shared<remote_snapcommunicator>(f_server, remote_addr));
        f_smaller_ips[addr] = remote_communicator;
        remote_communicator->set_name("remote communicator connection: " + addr);

        // make sure not to try to connect to all remote communicators
        // all at once
        //
        time_t const now(time(nullptr));
        if(now > f_last_start_date)
        {
            f_last_start_date = now;
        }
        remote_communicator->set_timeout_date(f_last_start_date * 1'000'000LL);

        // TBD: 1 second between attempts for each remote communicator,
        //      should that be smaller? (i.e. not the same connection but
        //      between all the remote connection attempts.)
        //
        f_last_start_date += 1LL;

        if(!ed::communicator::instance()->add_connection(remote_communicator))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR
                << "new remote connection to "
                << addr_port
                << " could not be added to the ed::communicator list of connections"
                << SNAP_LOG_SEND;

            auto it(f_smaller_ips.find(addr));
            if(it != f_smaller_ips.end())
            {
                f_smaller_ips.erase(it);
            }
        }
        else
        {
            SNAP_LOG_DEBUG
                << "new remote connection added for "
                << addr_port
                << SNAP_LOG_SEND;
        }
    }
    else //if(remote_addr != f_my_address) -- already tested at the beginning of the function
    {
        // in case the remote snapcommunicator has a larger address
        // it is expected to CONNECT to us; however, it may not yet
        // know about us so we want to send a GOSSIP message; this
        // means creating a special connection which attempts to
        // send the GOSSIP message up until it succeeds or the
        // application quits
        //
        f_gossip_ips[addr] = std::make_shared<gossip_connection>(
                                      shared_from_this()
                                    , remote_addr);
        f_gossip_ips[addr]->set_name("gossip to remote snap communicator: " + addr);

        if(!ed::communicator::instance()->add_connection(f_gossip_ips[addr]))
        {
            // this should never happens here since each new creates a
            // new pointer
            //
            SNAP_LOG_ERROR
                << "new gossip connection to "
                << addr_port
                << " could not be added to the ed::communicator list of connections."
                << SNAP_LOG_SEND;

            auto it(f_gossip_ips.find(addr));
            if(it != f_gossip_ips.end())
            {
                f_gossip_ips.erase(it);
            }
        }
        else
        {
            SNAP_LOG_DEBUG
                << "new gossip connection added for "
                << addr_port
                << SNAP_LOG_SEND;
        }
    }
}


/** \brief Stop all gossiping at once.
 *
 * This function can be called to remove all the gossip connections
 * at once.
 *
 * In most cases this function is called whenever the snapcommunicator
 * daemon receives a STOP or a SHUTDOWN.
 *
 * Also these connections do not support any other messages than the
 * GOSSIP and RECEIVED.
 */
void remote_connections::stop_gossiping()
{
    while(!f_gossip_ips.empty())
    {
        ed::communicator::instance()->remove_connection(*f_gossip_ips.begin());
        f_gossip_ips.erase(f_gossip_ips.begin());
    }
}


/** \brief A remote communicator refused our connection.
 *
 * When a remote snap communicator server already manages too many
 * connections, it may end up refusing our additional connection.
 * When this happens, we have to avoid trying to connect again
 * and again.
 *
 * Here we use a very large delay of 24h before trying to connect
 * again later. I do not really think this is necessary because
 * if we have too many connections we anyway always have too many
 * connections. That being said, once in a while a computer dies
 * and thus the number of connections may drop to a level where
 * we will be accepted.
 *
 * At some point we may want to look into having seeds instead
 * of allowing connections to all the nodes.
 *
 * \param[in] address  The address of the snapcommunicator that refused a
 *                     CONNECT because it is too busy.
 */
void remote_connections::too_busy(addr::addr const & address)
{
    auto it(f_smaller_ips.find(address));
    if(it != f_smaller_ips.end()
    {
        // wait for 1 day and try again (is 1 day too long?)
        f_smaller_ips[addr]->set_timeout_delay(remote_snapcommunicator::REMOTE_CONNECTION_TOO_BUSY_TIMEOUT);
        f_smaller_ips[addr]->set_enable(true);
        SNAP_LOG_INFO("remote communicator ")(addr)(" was marked as too busy. Pause for 1 day before trying to connect again.");
    }
}


/** \brief Another system is shutting down, maybe rebooting.
 *
 * This function makes sure we wait for some time, instead of waisting
 * our time trying to reconnect again and again.
 *
 * \param[in] addr  The address of the snapcommunicator that refused a
 *                  CONNECT because it is shutting down.
 */
void remote_connections::shutting_down(QString const & addr)
{
    if(f_smaller_ips.contains(addr))
    {
        // wait for 5 minutes and try again
        //
        f_smaller_ips[addr]->set_timeout_delay(remote_snapcommunicator::REMOTE_CONNECTION_RECONNECT_TIMEOUT);
        f_smaller_ips[addr]->set_enable(true);
        SNAP_LOG_DEBUG("remote communicator ")(addr)(" was said it was shutting down. Pause for 5 minutes before trying to connect again.");
    }
}


void remote_connections::server_unreachable(QString const & addr)
{
    // we do not have the name of the computer in snapcommunicator so
    // we just broadcast the IP address of the non-responding computer
    //
    snap::snap_communicator_message unreachable;
    unreachable.set_service(".");
    unreachable.set_command("UNREACHABLE");
    unreachable.add_parameter("who", addr);
    f_communicator_server->broadcast_message(unreachable);
}


void remote_connections::gossip_received(QString const & addr)
{
    auto it(f_gossip_ips.find(addr));
    if(it != f_gossip_ips.end())
    {
        snap::snap_communicator::instance()->remove_connection(*it);
        f_gossip_ips.erase(it);
    }
}


void remote_connections::forget_remote_connection(QString const & addr_port)
{
    QString addr(addr_port);
    int const pos(addr.indexOf(':'));
    if(pos > 0)
    {
        // forget about the port if present
        //
        addr = addr.mid(0, pos);
    }
    auto it(f_smaller_ips.find(addr));
    if(it != f_smaller_ips.end())
    {
        snap::snap_communicator::instance()->remove_connection(*it);
        f_smaller_ips.erase(it);
    }
}


/** \brief Count the number of live remote connections.
 *
 * This function gives us the total number of computers we are connected
 * with right now. Of course, it may be that one of them just broke,
 * but it should still be close enough.
 *
 * The GOSSIP connects are completely ignored since those are just and
 * only to sendthe GOSSIP message and not for a complete communication
 * channel. This is used to quickly get connections made between
 * snapcommunitors when one wakes up and is not to connect to the
 * other (i.e. A connects to B means A has a larger IP address than B.)
 *
 * \warning
 * The function counts from scratch each time it gets called in case it
 * changed since the last time the funciton was called. This is to make
 * sure we always get it right (instead of doing a ++ or a -- on an
 * event and miss one here or there... although that other method would
 * be much better, but its difficult to properly count disconnections.)
 *
 * \return The number of live connections.
 */
size_t remote_connections::count_live_connections() const
{
    size_t count(0);

    // smaller IPs, we connect to, they are always all there (all our
    // neighbors with smaller IPs are in this list) because we are
    // responsible to connect to them
    //
    // we have to go through the list since some connections may be
    // down, the is_connected() function tells us the current status
    //
    //for(auto ip : f_smaller_ips)
    //{
    //    if(ip->is_connected())
    //    {
    //        ++count;
    //    }
    //}

    // the larger IPs list includes connections to us from some other
    // service; here we verify that it is indeed a remote connection
    // (which it should be since it's in the remote connection list)
    //
    //for(auto ip : f_larger_ips)
    //{
    //    // only live remote connections should be in this list, but verify
    //    // none the less (dead connections get removed from this list)
    //    //
    //    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(ip));
    //    base_connection::connection_type_t const type(base->get_connection_type());
    //    if(type == base_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
    //    {
    //        ++count;
    //    }
    //}

    // unfortunately, the f_larger_ips is not actually used and the local
    // connections are left in the complete list of connections in the
    // snap::snap_communicator::instance() all mixed up
    //
    snap::snap_communicator::snap_connection::vector_t const & all_connections(snap::snap_communicator::instance()->get_connections());
    for(auto const & conn : all_connections)
    {
        remote_snapcommunicator::pointer_t sc(std::dynamic_pointer_cast<remote_snapcommunicator>(conn));
        if(sc != nullptr)
        {
            // this is a remote connection by definition
            //
            ++count;
        }
        else
        {
            // this is either a local or a remote connection
            //
            // these are connections we receive from from our listeners
            //
            base_connection_pointer_t bc(std::dynamic_pointer_cast<base_connection>(conn));
            if(bc != nullptr
            && bc->is_remote())
            {
                ++count;
            }
        }
    }

    return count;
}


} // sc namespace
// vim: ts=4 sw=4 et

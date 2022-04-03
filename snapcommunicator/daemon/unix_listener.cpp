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
 * \brief Implementation of the listener.
 *
 * The listener connection is the one listening for connections from
 * local and remote services.
 */

// self
//
#include    "unix_listener.h"


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



/** \class unix_listener
 * \brief Handle new connections from clients.
 *
 * This class is an implementation of the snap server connection so we can
 * handle new connections from various clients.
 */




/** \brief The listener initialization.
 *
 * The listener creates a new TCP server to listen for incoming
 * TCP connection.
 *
 * \warning
 * At this time the \p max_connections parameter is ignored.
 *
 * \param[in] address  The address:port to listen on. Most often it is
 * 0.0.0.0:4040 (plain connection) or 0.0.0.0:4041 (secure connection).
 * \param[in] certificate  The filename of a PEM file with a certificate.
 * \param[in] private_key  The filename of a PEM file with a private key.
 * \param[in] max_connections  The maximum number of connections to keep
 *                             waiting; if more arrive, refuse them until
 *                             we are done with some existing connections.
 * \param[in] local  Whether this connection expects local services only.
 * \param[in] server_name  The name of the server running this instance.
 * \param[in] secure  Whether to create a secure (true) or non-secure (false)
 *                    connection.
 */
unix_listener::unix_listener(
          server::pointer_t cs
        , addr::unix const & address
        , int max_connections
        , std::string const & server_name)
    : local_stream_server_connection(address, max_connections)
    , f_server(cs)
    , f_server_name(server_name)
{
}


void listener::process_accept()
{
    // a new client just connected, create a new service_connection
    // object and add it to the snap_communicator object.
    //
    tcp_client_server::bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred, report in the logs
        int const e(errno);
        SNAP_LOG_ERROR
            << "somehow accept() failed with errno: "
            << e
            << " -- "
            << strerror(e)
            << SNAP_LOG_SEND;
        return;
    }

    service_connection::pointer_t connection(
            std::shared_ptr<service_connection>(
                      f_server
                    , new_client
                    , f_server_name));

    // TBD: is that a really weak test?
    //
    //QString const addr(connection->get_remote_address());
    // the get_remote_address() function may return an IP and a port so
    // parse that to remove the port; also remote_addr() has a function
    // that tells us whether the IP is private, local, or public
    //
    addr::addr const remote_addr(addr::string_to_addr(connection->get_remote_address().toUtf8().data(), "0.0.0.0", 4040, "tcp"));
    addr::addr::network_type_t const network_type(remote_addr.addr::get_network_type());
    if(f_local)
    {
        if(network_type != addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            // TODO: look into making this an ERROR() again and return, in
            //       effect viewing the error as a problem and refusing the
            //       connection (we had a problem with the IP detection
            //       which should be resolved now that we use the `addr`
            //       class
            //
            SNAP_LOG_WARNING("received what should be a local connection from \"")(connection->get_remote_address())("\".");
            //return;
        }

        // set a default name in each new connection, this changes
        // whenever we receive a REGISTER message from that connection
        //
        connection->set_name("client connection");

        connection->set_server_name(f_server_name);
    }
    else
    {
        if(network_type == addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            SNAP_LOG_ERROR("received what should be a remote connection from \"")(connection->get_remote_address())("\".");
            return;
        }

        // set a name for remote connections
        //
        // the following name includes a space which prevents someone
        // from send to such a connection, which is certainly a good
        // thing since there can be duplicate and that name is not
        // sensible as a destination
        //
        // we will change the name once we receive the CONNECT message
        // and as we send the ACCEPT message
        //
        connection->set_name(QString("remote connection from: %1").arg(connection->get_remote_address())); // remote host connected to us
        connection->mark_as_remote();
    }

    if(!snap::snap_communicator::instance()->add_connection(connection))
    {
        // this should never happen here since each new creates a
        // new pointer
        //
        SNAP_LOG_ERROR("new client connection could not be added to the snap_communicator list of connections");
    }
}




} // sc namespace
// vim: ts=4 sw=4 et

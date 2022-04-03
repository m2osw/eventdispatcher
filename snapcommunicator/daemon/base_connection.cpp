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
 * \brief Base declaration of all the connections.
 *
 * The snapcommunicator has three types of connections:
 *
 * * Connection from this server to another snapcommunicator server
 * * Connection from another snapcommunicator server to this server
 * * Connection from a local server to the snapcommunicator
 *
 * The first and second are managed between snapcommunicator servers
 * to create the RPC between all your tools.
 *
 * The last type of connections happen from all the local services
 * to the snapcommunicator.
 *
 * In all cases, these connections derive from the base_connection
 * class so that way many of the common functions can be implemented
 * in one place.
 */



// self
//
#include    "base_connection.h"


// // snapwebsites lib
// //
// #include <snapwebsites/chownnm.h>
// #include <snapwebsites/flags.h>
// #include <snapwebsites/glob_dir.h>
// #include <snapwebsites/loadavg.h>
// #include <snapwebsites/log.h>
// #include <snapwebsites/qcompatibility.h>
// #include <snapwebsites/snap_communicator.h>
// #include <snapwebsites/snapwebsites.h>
// 
// 
// // snapdev lib
// //
// #include <snapdev/not_used.h>
// #include <snapdev/tokenize_string.h>
// 
// 
// // libaddr lib
// //
// #include <libaddr/addr_exception.h>
// #include <libaddr/addr_parser.h>
// #include <libaddr/iface.h>
// 
// 
// // C++ lib
// //
// #include <atomic>
// #include <cmath>
// #include <fstream>
// #include <iomanip>
// #include <sstream>
// #include <thread>
// 
// 
// // C lib
// //
// #include <grp.h>
// #include <pwd.h>
// #include <sys/resource.h>


// included last
//
#include <snapdev/poison.h>






namespace sc
{





/** \brief Initialize the base_connection() object.
 *
 * The constructor saves the communicator server pointer
 * so one can access it from any derived version.
 */
base_connection::base_connection(server::pointer_t cs)
    : f_server(cs)
{
}


/** \brief We have a destructor to make it virtual.
 *
 * Everything is otherwise automatically released.
 */
base_connection::~base_connection()
{
}


/** \brief Save when the connection started.
 *
 * This function is called whenever a CONNECT or REGISTER message
 * is received since those mark the time when a connection starts.
 *
 * You can later retrieve when the connection started with the
 * get_connection_started() function.
 *
 * This call also resets the f_ended_on in case we were able to
 * reuse the same connection multiple times (reconnecting means
 * a new socket and thus a brand new connection object...)
 */
void base_connection::connection_started()
{
    f_started_on = time(nullptr);
    f_ended_on = -1;
}


/** \brief Return information on when the connection started.
 *
 * This function gives you the date and time when the connection
 * started, meaning when the connection received a CONNECT or
 * REGISTER event.
 *
 * If the events have no yet occured, then the connection returns
 * -1 instead.
 *
 * \return The date and time when the connection started in microseconds.
 */
int64_t base_connection::get_connection_started() const
{
    return f_started_on;
}


/** \brief Connection ended, save the date and time of the event.
 *
 * Whenever we receive a DISCONNECT or UNREGISTER we call this
 * function. It also gets called in the event a connection
 * is deleted without first receiving a graceful DISCONNECT
 * or UNREGISTER event.
 */
void base_connection::connection_ended()
{
    // save the current date only if the connection really started
    // before and also only once (do not update the end time again
    // until a connection_started() call happens)
    //
    if(f_started_on != -1
    && f_ended_on == -1)
    {
        f_ended_on = time(nullptr);
    }
}


/** \brief Timestamp when the connection was ended.
 *
 * This value represents the time when the UNREGISTER, DISCONNECT,
 * or the destruction of the service_connection object occurred. It
 * represents the time when the specific service was shutdown.
 *
 * \return The approximative date when the connection ended in microseconds.
 */
int64_t base_connection::get_connection_ended() const
{
    return f_ended_on;
}


/** \brief Save the name of the server.
 *
 * \param[in] server_name  The name of the server that is on the other
 *                         side of this connection.
 */
void base_connection::set_server_name(std::string const & server_name)
{
    f_server_name = server_name;
}


/** \brief Get the name of the server.
 *
 * \return The name of the server that is on the other
 *         side of this connection.
 */
QString base_connection::get_server_name() const
{
    return f_server_name;
}


/** \brief Save the address of that connection.
 *
 * This is only used for remote connections on either the CONNECT
 * or ACCEPT message.
 *
 * \param[in] my_address  The address of the server that is on the
 *                        other side of this connection.
 */
void base_connection::set_my_address(addr::addr const & my_address)
{
    f_my_address = my_address;
}


/** \brief Get the address of that connection.
 *
 * This function returns a valid address only after the CONNECT
 * or ACCEPT message were received for this connection.
 *
 * \return The address of the server that is on the
 *         other side of this connection.
 */
addr::addr base_connection::get_my_address() const
{
    return f_my_address;
}


/** \brief Define the type of snapcommunicator server.
 *
 * This function is called whenever a CONNECT or an ACCEPT is received.
 * It saves the type=... parameter. By default the type is empty meaning
 * that the connection was not yet fully initialized.
 *
 * When a REGISTER is received instead of a CONNECT or an ACCEPT, then
 * the type is set to "client".
 *
 * \param[in] type  The type of connection.
 */
void base_connection::set_connection_type(connection_type_t type)
{
    f_type = type;
}


/** \brief Retrieve the current type of this connection.
 *
 * By default a connection is given the type CONNECTION_TYPE_DOWN,
 * which means that it is not currently connected. To initialize
 * a connection one has to either CONNECT (between snapcommunicator
 * servers) or REGISTER (a service such as snapbackend, snapserver,
 * snapwatchdog, and others.)
 *
 * The type is set to CONNECTION_TYPE_LOCAL for local services and
 * CONNECTION_TYPE_REMOTE when representing another snapserver.
 *
 * \return The type of server this connection represents.
 */
connection_type_t base_connection::get_connection_type() const
{
    return f_type;
}


/** \brief Define the list of services supported by the snapcommunicator.
 *
 * Whenever a snapcommunicator connects to another one, either by
 * doing a CONNECT or replying to a CONNECT by an ACCEPT, it is
 * expected to list services that it supports (the list could be
 * empty as it usually is on a Cassandra node.) This function
 * saves that list.
 *
 * This defines the name of services and thus where to send various
 * messages such as a PING to request a service to start doing work.
 *
 * \param[in] services  The list of services this server handles.
 */
void base_connection::set_services(set::string const & services)
{
    std::set<std::string> list;
    snapdev::tokenize_string(list, services, { "," });
    for(auto const & s : list)
    {
        f_services[s] = true;
    }
}


/** \brief Retrieve the list of services offered by other snapcommunicators.
 *
 * This function saves in the input parameter \p services the list of
 * services that this very snapcommunicator offers.
 *
 * \param[in,out] services  The map where all the services are defined.
 */
void base_connection::get_services(sorted_list_of_strings_t & services)
{
    f_services.merge(services);
}


/** \brief Check whether the service is known by that connection.
 *
 * This function returns true if the service was defined as one
 * this connection supports.
 *
 * \param[in] name  The name of the service to check for.
 *
 * \return true if the service is known.
 */
bool base_connection::has_service(std::string const & name)
{
    return f_services.find(name) != f_services.end();
}


/** \brief Define the list of services we heard of.
 *
 * This function saves the list of services that were heard of by
 * another snapcommunicator server. This list may be updated later
 * with an ACCEPT event.
 *
 * This list is used to know where to forward a message if we do
 * not have a more direct link to those services (i.e. the same
 * service defined in our own list or in a snapcommunicator
 * we are directly connected to.)
 *
 * \param[in] services  The list of services heard of.
 */
void base_connection::set_services_heard_of(std::string const & services)
{
    std::set<std::string> list;
    snapdev::tokenize_string(list, services, { "," });
    for(auto const & s : list)
    {
        f_services_heard_of[s] = true;
    }
}


/** \brief Retrieve the list of services heard of by another server.
 *
 * This function saves in the input parameter \p services the list of
 * services that this snapcommunicator heard of.
 *
 * \param[in,out] services  The map where all the services are defined.
 */
void base_connection::get_services_heard_of(sorted_list_of_strings_t & services)
{
    f_services_heard_of.merge(services);
}


/** \brief List of defined commands.
 *
 * This function saves the list of commands known by another process.
 * The \p commands parameter is broken up at each comma and the
 * resulting list saved in the f_understood_commands map for fast
 * retrieval.
 *
 * In general a process receives the COMMANDS event whenever it
 * sent the HELP event to request for this list.
 *
 * \param[in] commands  The list of understood commands.
 */
void base_connection::set_commands(std::string const & commands)
{
    snap::snap_string_list cmds(commands.split(','));
    for(auto const & c : cmds)
    {
        std::string const name(c.trimmed());
        if(!name.empty())
        {
            f_understood_commands[name] = true;
        }
    }
}


/** \brief Check whether a certain command is understood by this connection.
 *
 * This function checks whether this connection understands \p command.
 *
 * \param[in] command  The command to check for.
 *
 * \return true if the command is supported, false otherwise.
 */
bool base_connection::understand_command(std::string const & command)
{
    return f_understood_commands.find(command) != f_understood_commands.end();
}


/** \brief Check whether this connection received the COMMANDS message.
 *
 * This function returns true if the list of understood commands is
 * defined. This means we do know whether a verification (i.e. a call
 * to the understand_command() function) will return false because the
 * list of commands is empty or because a command is not understood.
 *
 * \return true if one or more commands are understood.
 */
bool base_connection::has_commands() const
{
    return !f_understood_commands.empty();
}


/** \brief Remove a command.
 *
 * This function is used to make the system think that certain command
 * are actually not understood.
 *
 * At this time, it is only used when a connection goes away and we
 * want to send a STATUS message to various services interested in
 * such a message.
 *
 * \param[in] command  The command to remove.
 */
void base_connection::remove_command(std::string const & command)
{
    auto it(f_understood_commands.find(command));
    if(it != f_understood_commands.end())
    {
        f_understood_commands.erase(it);
    }
}


/** \brief Mark that connection as a remote connection.
 *
 * When we receive a connection from another snapconnector, we call
 * this function so later we can very quickly determine whether the
 * connection is a remote connection.
 */
void base_connection::mark_as_remote()
{
    f_remote_connection = true;
}


/** \brief Check whether this connection is a remote connection.
 *
 * The function returns false by default. If the mark_as_remote()
 * was called, this function returns true.
 *
 * \return true if the connection was marked as a remote connection.
 */
bool base_connection::is_remote() const
{
    return f_remote_connection;
}


/** \brief Set whether this connection wants to receive LOADAVG messages.
 *
 * Whenever a frontend wants to know which backend to use for its
 * current client request, it can check a set of IP addresses for
 * the least loaded computer. Then it can use that IP address to
 * process the request.
 *
 * \param[in] wants_loadavg  Whether this connection wants
 *    (REGISTERFORLOADAVG) or does not want (UNREGISTERFORLOADAVG)
 *    to receive LOADAVG messages from this snapcommunicator.
 */
void base_connection::set_wants_loadavg(bool wants_loadavg)
{
    f_wants_loadavg = wants_loadavg;
}


/** \brief Check whether this connection wants LOADAVG messages.
 *
 * This function returns true if the connection last sent us a
 * REGISTERFORLOADAVG message.
 *
 * \return true if the LOADAVG should be sent to this connection.
 */
bool base_connection::wants_loadavg() const
{
    return f_wants_loadavg;
}



} // namespace sc
// vim: ts=4 sw=4 et

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

/** \file
 * \brief Implementation of the Connection with Send Message class.
 *
 * This is a base class which ease the implementation of a connection
 * which is able to send and receive messages.
 */


// self
//
#include    "eventdispatcher/connection_with_send_message.h"

#include    "eventdispatcher/connection.h"
#include    "eventdispatcher/dispatcher_support.h"
#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/logger.h>
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// boost
//
#include    <boost/algorithm/string/join.hpp>


// C
//
#ifdef __SANITIZE_ADDRESS__
#include    <sanitizer/lsan_interface.h>
#endif


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize the connection.
 *
 * This constructor initialize the connection with a send_message() function.
 * The function takes an optional \p service_name which is used in various
 * messages such as the REGISTER and UNREGISTER messages.
 *
 * If the \p service_name parameter is an empty string, then the functions
 * that require that name throw when reached.
 *
 * \param[in] service_name  The name of your service.
 *
 * \sa register_service()
 * \sa unregister_service()
 */
connection_with_send_message::connection_with_send_message(std::string const & service_name)
    : f_service_name(service_name)
{
}


/** \brief Clean up.
 *
 * The destructor makes sure to clean up the connection with send_message()
 * objects.
 */
connection_with_send_message::~connection_with_send_message()
{
}



/** \brief Reply to the watchdog message ALIVE.
 *
 * To check whether a service is alive, send the ALIVE message. This
 * function builds an ABSOLUTELY reply and attaches the "serial" parameter
 * as is if present in the ALIVE message. It also includes the original
 * "timestamp" parameter.
 *
 * The function also adds one field named "reply_timestamp" with the Unix
 * time in seconds when the reply is being sent.
 *
 * \note
 * The "serial" parameter is expected to be used to make sure that no messages
 * are lost, or if loss is expected, to see whether loss is heavy or not.
 *
 * \note
 * The "serial" and "timestamp" parameters do not get checked. If present
 * in the original message, they get copied verbatim to the destination.
 * This allows you to include anything you want in those parameters although
 * we suggest you use the "timestamp" only for a value representing time.
 *
 * \param[in] msg  The ALIVE message.
 */
void connection_with_send_message::msg_alive(message & msg)
{
    message absolutely;
    absolutely.user_data(msg.user_data<void>());
    absolutely.reply_to(msg);
    absolutely.set_command("ABSOLUTELY");
    if(msg.has_parameter("serial"))
    {
        absolutely.add_parameter("serial", msg.get_parameter("serial"));
    }
    if(msg.has_parameter("timestamp"))
    {
        absolutely.add_parameter("timestamp", msg.get_parameter("timestamp"));
    }
    absolutely.add_parameter("reply_timestamp", time(nullptr));
    if(!send_message(absolutely, false))
    {
        SNAP_LOG_WARNING
            << "could not reply to \""
            << msg.get_command()
            << "\" with an ABSOLUTELY message."
            << SNAP_LOG_SEND;
    }
}


/** \brief Build the HELP reply and send it.
 *
 * When a daemon registers with the snapcommunicator, it sends a REGISTER
 * command. As a result, your daemon is sent a HELP command which must be
 * answered with a COMMANDS message which includes the list of commands
 * (a.k.a. messages) that your daemon supports.
 *
 * The list of commands is built using the list of Expression() strings
 * found in the dispatcher of your daemon. If that list includes null
 * pointers or custom match functions, then the list is deemed to
 * include functions that the default loop cannot determine. As a result,
 * your help() function will be called and it must be overridden, otherwise
 * it will call the default which throws.
 *
 * \exception implementation_error
 * This exception is thrown if the resulting list of commands is empty.
 *
 * \param[in] msg  The HELP message.
 *
 * \sa help()
 */
void connection_with_send_message::msg_help(message & msg)
{
    snapdev::NOT_USED(msg);

    bool need_user_help(true);
    string_list_t commands;

    dispatcher_base * d;
    dispatcher_support * ds(dynamic_cast<dispatcher_support *>(this));
    if(ds != nullptr)
    {
        // we extract the bare pointer because in the other case
        // we only get a bare pointer... (which we can't safely
        // put in a shared pointer, although we could attempt to
        // use shared_from_this() but we could have a class without
        // it?)
        //
        d = ds->get_dispatcher().get();
    }
    else
    {
        d = dynamic_cast<dispatcher_base *>(this);
    }
    if(d != nullptr)
    {
        need_user_help = d->get_commands(commands);
    }

    // the user has "unknown" commands (as far as the dispatcher is concerned)
    // in his list of commands so we have to let him enter them "manually"
    //
    // this happens whenever there is an entry which is a regular expression
    // or something similar which we just cannot grab
    //
    if(need_user_help)
    {
        help(commands);
    }

    // the list of commands just cannot be empty
    //
    if(commands.empty())
    {
        throw implementation_error(
                "connection_with_send_message::msg_help()"
                " is not able to determine the commands this messenger supports");
    }

    // Now prepare the COMMAND message and send it
    //
    // Note: we turn off the caching on this message, it does not make sense
    //       because if snapcommunicator is not running, then caching won't
    //       happen work anyway (i.e. snapcommunicator has to send HELP first
    //       and then we send the reply, if it has to restart, then just
    //       sending COMMANDS will fail.)
    //
    message reply;
    reply.user_data(msg.user_data<void>());
    reply.set_command("COMMANDS");
    reply.add_parameter("list", boost::algorithm::join(commands, ","));
    if(!send_message(reply, false))
    {
        SNAP_LOG_WARNING
            << "could not reply to \""
            << msg.get_command()
            << "\" with a COMMANDS message."
            << SNAP_LOG_SEND;
    }
}


/** \brief Run the sanitizer leak checker.
 *
 * This function calls the function printing out all the leaks found
 * at this time in this software (\em leaks as in any block of memory
 * currently allocated).
 *
 * The message does nothing if the library was not compiled with the
 * sanitizer feature turned on.
 *
 * The message can be sent any number of times.
 *
 * \note
 * This is a debug message only.
 *
 * \todo
 * Find a solution which actually works. The LSAN system doesn't offer
 * to (correctly) print the list of leaks at any given time.
 *
 * \param[in] msg  The LEAK message, which is ignored.
 */
void connection_with_send_message::msg_leak(ed::message & msg)
{
    snapdev::NOT_USED(msg);

#ifdef __SANITIZE_ADDRESS__
    __lsan_do_recoverable_leak_check();
#else
    std::cerr << "error: leaks are not being tracked;"
                " use the --sanitize option to turn on this feature."
        << std::endl;
#endif
}


/** \brief Reopen file-based logger appenders.
 *
 * Whenever logrotate runs or some changes are made to the log
 * definitions, the corresponding daemons need to reopen snaplogger
 * to make use of the new file and settings. This command is used
 * for that purpose.
 *
 * \note
 * If the snaplogger is not currently configured, this message is
 * ignored.
 *
 * \param[in] msg  The LOG_ROTATE message.
 */
void connection_with_send_message::msg_log_rotate(message & msg)
{
    snapdev::NOT_USED(msg);

    if(snaplogger::is_configured())
    {
        // send log in the old file and format
        //
        SNAP_LOG_INFO
            << "-------------------- Logging reconfiguration request."
            << SNAP_LOG_SEND;

        // reconfigure
        //
        snaplogger::reopen();

        // send log to new file and format
        //
        SNAP_LOG_INFO
            << "-------------------- Logging reconfiguration done."
            << SNAP_LOG_SEND;
    }
}


/** \brief Call you stop() function with true.
 *
 * This command means that someone is asking your daemon to quit as soon as
 * possible because the Snap! environment is being asked to shutdown.
 *
 * The value 'true' means that all the daemons are being asked to stop and
 * not just you.
 *
 * \param[in] msg  The QUITTING message.
 *
 * \sa msg_stop()
 * \sa stop()
 */
void connection_with_send_message::msg_quitting(message & msg)
{
    snapdev::NOT_USED(msg);

    stop(true);
}


/** \brief Calls your ready() function with the message.
 *
 * All daemons using the snapcommunicator daemon have to have a ready()
 * function which gets called once the HELP and COMMAND message were
 * handled. This is when your daemon is expected to be ready to start
 * working. Some daemons start working immediately no matter
 * what (i.e. snapwatchdog and snapfirewall do work either way), but
 * those are rare.
 *
 * \param[in] msg  The READY message.
 *
 * \sa ready()
 */
void connection_with_send_message::msg_ready(message & msg)
{
    // pass the message so any additional info can be accessed.
    //
    ready(msg);
}


/** \brief Calls your restart() function with the message.
 *
 * This message has no implementation by default. What we want to
 * do is find a clean way to restart any service instantly.
 *
 * The RESTART message is expected to be used whenever a modification
 * to some file or the system environment somehow affects your service
 * in such a way that it requires a restart. For example, after an
 * upgrade of the eventdispatcher library, you should restart all the
 * services that make use of it. For this reason, we have a RESTART
 * message.
 *
 * The message comes with one parameter named `reason` which describes
 * why the RESTART was sent:
 *
 * \li `reason=upgrade` -- something (library/tools) was upgraded
 * \li `reason=config` -- a configuration file was updated
 *
 * \note
 * At the time we created this message, a live configuration was not
 * available. Now that we have the fluid-settings service, that changed
 * and in most cases, `reason=config` should not be necessary anymore.
 * (only for fluid-settings itself and for some parameters not found
 * in the fluid-settings).
 *
 * \note
 * There are currently some services that make use of a CONFIG message
 * whenever their configuration changes. Pretty much all services do
 * not support a live configuration change (because it initializes their
 * objects from the configuration data once on startup and in many cases
 * it would be very complicated to allow for changes to occur on the fly).
 * \note
 * In those existing implementations, we really just do a restart anyway.
 * \note
 * The new versions will be using fluid-settings. We can still support
 * a RESTART message, but a RELOADCONFIG (as we had in snapcommunicator)
 * should not be used.
 *
 * \param[in] msg  The RESTART message.
 *
 * \sa restart()
 */
void connection_with_send_message::msg_restart(message & msg)
{
    // pass the message so any additional info can be accessed.
    //
    restart(msg);
}


/** \brief Call you stop() function with false.
 *
 * This command means that someone is asking your daemon to stop.
 *
 * The value 'false' means just your daemon was asked to stop and not the
 * entire system to shutdown (otherwise you would receive a QUITTING command
 * instead.)
 *
 * \param[in] msg  The STOP message.
 *
 * \sa msg_quitting()
 */
void connection_with_send_message::msg_stop(message & msg)
{
    snapdev::NOT_USED(msg);

    stop(false);
}


/** \brief Handle the UNKNOWN message.
 *
 * Whenever we send a command to another daemon, that command can be refused
 * by sending an UNKNOWN reply. This function handles the UNKNOWN command
 * by simply recording that as an error in the logs.
 *
 * \param[in] msg  The UNKNOWN message we just received.
 */
void connection_with_send_message::msg_log_unknown(message & msg)
{
    // we sent a command that the other end did not understand
    // and got an UNKNOWN reply
    //
    SNAP_LOG_ERROR
        << "we sent command \""
        << msg.get_parameter("command")
        << "\" and the destination replied with \"UNKNOWN\""
           " so we probably did not get the expected result."
        << SNAP_LOG_SEND;
}


/** \brief Send the UNKNOWN message as a reply.
 *
 * This function replies to the \p message with the UNKNOWN message as
 * expected by all our connection objects when a service receives a
 * message it does not know how to handle.
 *
 * It is expected to be used in your dispatcher_match array.
 *
 * \note
 * This function is virtual which allows you to add it to your array of
 * of dispatcher_match items. The following shows an example of what that
 * can look like.
 *
 * \code
 *  {
 *      ed::dispatcher<my_connection>::define_match(
 *          ...
 *      ),
 *      ...
 *
 *      // ALWAYS LAST
 *      ed::dispatcher<my_connection>::define_catch_all()
 *  };
 * \endcode
 *
 * \param[in] msg  The message to reply to.
 */
void connection_with_send_message::msg_reply_with_unknown(message & msg)
{
    message unknown;
    unknown.user_data(msg.user_data<void>());
    unknown.reply_to(msg);
    unknown.set_command("UNKNOWN");
    unknown.add_parameter("command", msg.get_command());
    if(!send_message(unknown, false))
    {
        SNAP_LOG_WARNING
            << "could not reply to \""
            << msg.get_command()
            << "\" with UNKNOWN message."
            << SNAP_LOG_SEND;
    }
    else
    {
        SNAP_LOG_MINOR
            << "unknown command \""
            << msg.get_command()
            << "\"."
            << SNAP_LOG_SEND;
    }
}


/** \brief The default help() function does nothing.
 *
 * This implementation does nothing. It is expected that you reimplement
 * this function depending on your daemon's need.
 *
 * The help() function gets called whenever the list of commands can't be
 * 100% defined automatically.
 *
 * Your function is expected to add commands to the \p commands parameter
 * as in:
 *
 * \code
 *      commands.push_back("MSG1");
 *      commands.push_back("MSG2");
 *      commands.push_back("MSG3");
 * \endcode
 *
 * This allows you to handle those three messages with a single entry in
 * your list of dispatcher_match objects with a regular expression such
 * as "MSG[1-3]".
 *
 * \param[in,out] commands  List of commands to update.
 */
void connection_with_send_message::help(string_list_t & commands)
{
    snapdev::NOT_USED(commands);

    // do nothing by default -- user is expected to overload this function
}


/** \brief The default ready() function does nothing.
 *
 * This implementation does nothing. It is expected that you reimplement
 * this function depending on your daemon's need. Most often this function
 * is the one that really starts your daemons process.
 *
 * \param[in,out] msg  The READY message.
 */
void connection_with_send_message::ready(message & msg)
{
    snapdev::NOT_USED(msg);

    // do nothing by default -- user is expected to overload this function
    //
    SNAP_LOG_WARNING
        << "default ready() function was called."
        << SNAP_LOG_SEND;
}


/** \brief The default restart() function does nothing.
 *
 * This implementation does nothing. It is expected that you reimplement
 * this function depending on your daemon's need. Most often this function
 * calls the stop() function in order to restart the daemon. If only a
 * configuration file changed and your daemon is capable of reading the
 * new settings without a full restart, then just read that new config.
 *
 * \param[in,out] msg  The RESTART message.
 *
 * \sa msg_restart()
 */
void connection_with_send_message::restart(message & msg)
{
    snapdev::NOT_USED(msg);

    // do nothing by default -- user is expected to overload this function
    //
    SNAP_LOG_WARNING
        << "default restart() function was called."
        << SNAP_LOG_SEND;
}


/** \brief The default stop() function does nothing.
 *
 * This implementation does nothing. It is expected that you reimplement
 * this function depending on your daemon's need.
 *
 * \param[in] quitting  Whether the QUITTING (true) or STOP (false) command
 *                      was received.
 */
void connection_with_send_message::stop(bool quitting)
{
    snapdev::NOT_USED(quitting);

    // do nothing by default -- user is expected to overload this function
    //
    SNAP_LOG_WARNING
        << "default stop() function was called."
        << SNAP_LOG_SEND;
}


/** \brief Retrieve the name of this service.
 *
 * A messenger used with the snapcommunicator is viewed as a service and
 * it needs to have a name. That name is specified at the time you create
 * your service (see constructor).
 *
 * This function returns that name.
 *
 * \exception name_undefined
 * When \p required is true, this function make raise a `name_undefined` if
 * the service name is empty. Fix this issue by adding the name in your
 * constructor.
 *
 * \param[in] required  Set to true if you want to raise an error when the
 * service name is not defined.
 *
 * \return The service name of this connection.
 */
std::string connection_with_send_message::get_service_name(bool required) const
{
    if(required
    && f_service_name.empty())
    {
        throw name_undefined("service name is required but not available.");
    }

    return f_service_name;
}


/** \brief Register your snapcommunicator service.
 *
 * This function registers your snapcommunicator service by sending
 * the REGISTER command to it. The service name must have been defined
 * in your constructor.
 *
 * The function is expected to be called in your process_connected()
 * function.
 *
 * \code
 *     void my_messenger::process_connected()
 *     {
 *         // make sure to call default function
 *         snap_tcp_client_permanent_message_connection::process_connected();
 *
 *         // then register
 *         register_service();
 *     }
 * \endcode
 *
 * \note
 * The function generates a fatal error if the send_message() fails.
 * However, you are responsible for quitting your service if the function
 * returns false. This very function does not attempt anything more.
 *
 * \exception name_undefined
 * The function makes use of the service name as specified in the
 * constructor. It cannot be empty.
 *
 * \return true if the send_message() succeeded.
 */
bool connection_with_send_message::register_service()
{
    message register_msg;
    register_msg.set_command("REGISTER");
    register_msg.add_parameter("service", get_service_name());
    register_msg.add_version_parameter();
    if(!send_message(register_msg, false))
    {
        SNAP_LOG_FATAL
            << "could not REGISTER with snapcommunicator."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


/** \brief Unregister from the snapcommunicator.
 *
 * The register_service() function registers your service with the
 * snapcommunicator. This function is the converse. It sends a message
 * to UNREGISTER you from the snapcommunicator. This means other services
 * will not be able to send you messages anymore.
 *
 * \exception name_undefined
 * The function makes use of the service name as specified in the
 * constructor. It cannot be empty.
 */
void connection_with_send_message::unregister_service()
{
    // mark ourself as done so once the last message(s) were sent,
    // we get automatically removed from the communicator
    //
    connection * c(dynamic_cast<connection *>(this));
    if(c == nullptr)
    {
        throw implementation_error("ed::connection_with_send_message must derive from ed::connection.");
    }
    c->mark_done();

    // unregister ourself from the snapcommunicator daemon
    //
    message unregister_msg;
    unregister_msg.set_command("UNREGISTER");
    unregister_msg.add_parameter("service", get_service_name(true));
    if(!send_message(unregister_msg, false))
    {
        SNAP_LOG_WARNING
            << "could not UNREGISTER from snapcommunicator."
            << SNAP_LOG_SEND;
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

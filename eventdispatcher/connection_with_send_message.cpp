// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief Implementation of the Connection with Send Message class.
 *
 * This is a base class which ease the implementation of a connection
 * which is able to send and receive messages.
 */


// self
//
#include    "eventdispatcher/connection_with_send_message.h"

#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/connection.h"
#include    "eventdispatcher/dispatcher.h"
#include    "eventdispatcher/dispatcher_support.h"
#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/names.h"


// snaplogger
//
#include    <snaplogger/logger.h>
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>
#include    <snapdev/join_strings.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


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
    // verify the name right away, after all it will be used in a message
    // where it will be verified the same way and the test has to pass
    //
    verify_message_name(f_service_name, true);
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
 * time in seconds with a precision of nanoseconds (after the decimal point,
 * see snapdev::timespec_ex) when the reply is being sent.
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
    absolutely.set_command(g_name_ed_cmd_absolutely);
    if(msg.has_parameter(g_name_ed_param_serial))
    {
        absolutely.add_parameter(g_name_ed_param_serial, msg.get_parameter(g_name_ed_param_serial));
    }
    if(msg.has_parameter(g_name_ed_param_timestamp))
    {
        absolutely.add_parameter(g_name_ed_param_timestamp, msg.get_parameter(g_name_ed_param_timestamp));
    }
    absolutely.add_parameter(g_name_ed_param_reply_timestamp, snapdev::now().to_timestamp());
    if(!send_message(absolutely, false))
    {
        SNAP_LOG_WARNING
            << "could not reply to \""
            << msg.get_command()
            << "\" with an "
            << g_name_ed_cmd_absolutely
            << " message."
            << SNAP_LOG_SEND;
    }
}


/** \brief Build the HELP reply and send it.
 *
 * When a service registers with the communicator daemon, it sends a REGISTER
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
    send_commands(&msg);
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
 * to (correctly) print the list of leaks at any given time. It seems
 * to only work once.
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
                " use the --sanitize option to compile with this feature."
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
 * All daemons using the communicator daemon have to have a ready()
 * function which gets called once the HELP and COMMAND messages were
 * handled. This is when your daemon is expected to be ready to start
 * working. Some daemons start working immediately no matter what
 * (i.e. sitter and iplock do work either way), but those are rare.
 * Yet others require more similar message as they need the clock to
 * be synchronized (cluck) or the cluster lock service to be ready
 * (prinbee) and yet others verify that the firewall is up before
 * doing much of anything.
 *
 * The READY message has one parameter: "my_address", which is the IP
 * address of the computer. Use the get_my_address() function to retrieve
 * it. Just make sure to do that only after you received the READY
 * message. Also, the port in that address is the communicator daemon
 * port used for connections between communicator daemons.
 *
 * \note
 * You get that IP:port address even if you connect using the Unix
 * connection. This IP address can be useful. For example, Prinbee
 * also needs to listen on that IP address for connections from
 * other Prinbee daemons.
 *
 * \param[in] msg  The READY message.
 *
 * \sa ready()
 */
void connection_with_send_message::msg_ready(message & msg)
{
    // get this computer's address with port 4042 by default
    // (the port is the "Remote Communicator Daemon Port")
    //
    if(msg.has_parameter(g_name_ed_param_my_address))
    {
        f_my_address = addr::string_to_addr(msg.get_parameter(g_name_ed_param_my_address));
    }

    // pass the message so any additional info can be accessed by callee.
    //
    ready(msg);

    f_ready = true;
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
 * New versions will be using fluid-settings. We can still support
 * a RESTART message, but a RELOADCONFIG (as we had in the communicator
 * daemon) should not be used.
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


/** \brief Reply when sending a message to an unavailable service.
 *
 * When sending a message to a service, you can setup the `"cache=..."`
 * parameter of the message to send you a reply in case the service is
 * not available.
 *
 * This works in conjunction of the `"no[=true]"` parameter because
 * that's the only way for a client to know that its message is going
 * to be lost.
 *
 * The default implementation does nothing.
 *
 * \note
 * There is also a TRANSMISSION_REPORT capability in the communicator
 * daemon which does something similar.
 *
 * \param[in] msg  The SERVICE_UNAVAILABLE message.
 */
void connection_with_send_message::msg_service_unavailable(message & msg)
{
    // do nothing by default
    snapdev::NOT_USED(msg);
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


/** \brief Handle the UNKNOWN or INVALID message.
 *
 * Whenever a command is sent to another daemon, that command can be refused
 * by sending:
 *
 * \li an UNKNOWN reply--the message is not handled by the destination;
 * \li an INVALID reply--the message is understand but was missused (missing
 * parameter, parameter value of the wrong type, etc.).
 *
 * This function handles the UNKNOWN or INVALID command by simply recording
 * that as an error in the logs.
 *
 * \param[in] msg  The UNKNOWN or INVALID message we just received.
 */
void connection_with_send_message::msg_log_unknown(message & msg)
{
    // we sent a command that the other end did not understand
    // and got an UNKNOWN reply
    //
    SNAP_LOG_ERROR
        << "we sent command \""
        << (msg.has_parameter(g_name_ed_param_command)
                ? msg.get_parameter(g_name_ed_param_command)
                : "<undefined>")
        << "\" and the destination replied with \""
        << msg.get_command()
        << "\""
           " so we probably did not get the expected result."
        << (msg.has_parameter(g_name_ed_param_message)
                ? " Message: " + msg.get_parameter(g_name_ed_param_message)
                : "")
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
 *      ed::dispatcher::define_match(
 *          ...
 *      ),
 *      ...
 *
 *      // ALWAYS LAST
 *      ed::dispatcher::define_catch_all()
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
    unknown.set_command(g_name_ed_cmd_unknown);
    unknown.add_parameter(g_name_ed_param_command, msg.get_command());
    if(!send_message(unknown, false))
    {
        SNAP_LOG_WARNING
            << "could not reply to \""
            << msg.get_command()
            << "\" with "
            << g_name_ed_cmd_unknown
            << " message."
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


/** \brief The default help() function calls your help callbacks.
 *
 * It is expected that you reimplement this function or add help callbacks
 * to your connection_with_send_message object.
 *
 * The help() function gets called whenever the list of commands can't be
 * 100% defined automatically (i.e. some messages are using regular
 * expressions, for example).
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
void connection_with_send_message::help(advgetopt::string_set_t & commands)
{
    snapdev::NOT_USED(f_help_callbacks.call(std::ref(commands)));
}


/** \brief Add a help callback.
 *
 * Whenever some of the callbacks do not use one of the default match
 * functions (one_to_one_match() or one_to_one_callback_match()), you
 * need to pass the name to the list of help commands. To do so, you
 * either
 *
 * * reimplement the help() function (if you are overloading
 *   the connection itself); or
 * * you add a callback using this function.
 *
 * You callback must return `true` to make sure it works as expected.
 */
void connection_with_send_message::add_help_callback(help_callback_t callback)
{
    f_help_callbacks.add_callback(callback);
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
 * A messenger used with the communicator daemon is viewed as a service and
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


/** \brief Check whether the READY message was received.
 *
 * This function returns true if the msg_ready() function was called, which
 * means we received the READY message from the communicatord. In most cases,
 * the user defined ready() callback will perform the final initialization
 * so after that point the connection is considered fully initialized and
 * thus ready.
 *
 * \note
 * For message based services which do not conenct to the communicator
 * deamon and do not receive the READY message, the function always
 * returns false.
 *
 * \return true if the READY message was received.
 */
bool connection_with_send_message::is_ready() const
{
    return f_ready;
}


/** \brief Retrieve the IP address of this computer.
 *
 * This function returns the IP address of this computer. The address is
 * actually sent to us by the communicatord through the READY message.
 * This means it won't be defined until you get that message.
 *
 * To know whether the address is defined, you can use the is_default()
 * function:
 *
 * \code
 *     addr::addr a(get_my_address());
 *     if(!a.is_default())
 *     {
 *         ...a is defined with this computer's IP address...
 *     }
 * \endcode
 *
 * \return The IP address of this computer or the default IP address if not
 * yet defined (i.e. READY was not yet received).
 */
addr::addr connection_with_send_message::get_my_address() const
{
    return f_my_address;
}


/** \brief Register your messenger service with communicatord.
 *
 * This function registers your messenger (a communicatord or
 * fluid_settings_connection) service by sending the REGISTER command to
 * it. The service name must have been defined in your constructor.
 * If you are using communicatord (or fluid_settings_connection) then
 * this function gets called automatically.
 *
 * The function is expected to be called in your ready() function.
 *
 * \code
 *     void my_messenger::process_connected()
 *     {
 *         // make sure to call default function
 *         // (it may not be a TCP client, adjust as required)
 *         tcp_client_permanent_message_connection::process_connected();
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
    register_msg.set_command(g_name_ed_cmd_register);
    register_msg.add_parameter(g_name_ed_param_service, get_service_name());
    register_msg.add_version_parameter();
    if(!send_message(register_msg, false))
    {
        SNAP_LOG_FATAL
            << "could not send \""
            << g_name_ed_cmd_register
            << "\" to communicatord."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


/** \brief Unregister a service from the communicator daemon.
 *
 * The register_service() function registers your service with the
 * communicator daemon. This function is the converse. It sends a message
 * to UNREGISTER you from the communicator daemon. This means other services
 * will not be able to send you messages anymore.
 *
 * \exception name_undefined
 * The function makes use of the service name as specified in the
 * constructor. It cannot be empty.
 *
 * \exception implementation_error
 * If the connection_with_send_message is not a valid connection object,
 * then this exception is raised. It should never happen.
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

    // unregister ourself from the communicator daemon
    //
    message unregister_msg;
    unregister_msg.set_command(g_name_ed_cmd_unregister);
    unregister_msg.add_parameter(g_name_ed_param_service, get_service_name(true));
    if(!send_message(unregister_msg, false))
    {
        SNAP_LOG_WARNING
            << "could not \""
            << g_name_ed_cmd_unregister
            << "\" from communicatord."
            << SNAP_LOG_SEND;

        communicator::instance()->remove_connection(c->shared_from_this());
    }
}


/** \brief Sent the COMMANDS message to the communicatord.
 *
 * This function gathers the list of commands this connection understands.
 * It then sends that list the communicator daemon.
 *
 * It is expected that the communicator daemon only accumulate the
 * command names. This is important since
 *
 * 1. some services make use of fully dynamic commands, which get
 *    added only at the time it becomes necessary. For example,
 *    the cluck service adds a few commands such as LOCKED and
 *    LOCK_FAILED;
 * 2. the messages may travel via UDP which does not guarantee
 *    the order in which the messages will be delivered.
 *
 * \warning
 * The \p msg parameter must be the HELP message from the communicator
 * daemon. The function verifies such.
 *
 * \param[in] msg  The message we are replying to or nullptr.
 */
void connection_with_send_message::send_commands(message * msg)
{
    if(msg != nullptr
    && msg->get_command() != g_name_ed_cmd_help)
    {
        SNAP_LOG_ERROR
            << "the 'msg' parameter to send_commands() must be a \""
            << g_name_ed_cmd_help
            << "\" message or nullptr. No commands will be sent."
            << SNAP_LOG_SEND;
        return;
    }

    dispatcher * d(nullptr);
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
        // in some cases, the user directly derive from the dispatcher
        //
        d = dynamic_cast<dispatcher *>(this);
    }

    advgetopt::string_set_t commands;
    bool need_user_help(true);
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
    //       because if the communicator daemon is not running, then caching
    //       won't work anyway (i.e. the communicator daemon has to send HELP
    //       first and then we send the reply, if it has to restart, then just
    //       sending COMMANDS will fail).
    //
    message commands_msg;
    if(msg != nullptr)
    {
        commands_msg.user_data(msg->user_data<void>());
        commands_msg.reply_to(*msg);
    }
    else
    {
        // TODO: use names? only the ones defining these are in communicatord
        //       which depends on us
        //
        commands_msg.set_server("."); // g_name_communicatord_server_me
        commands_msg.set_service("communicatord"); // g_name_communicatord_service_communicatord
    }
    commands_msg.set_command(g_name_ed_cmd_commands);
    commands_msg.add_parameter(
              g_name_ed_param_list
            , snapdev::join_strings(commands, ","));
    if(!send_message(commands_msg, false))
    {
        SNAP_LOG_WARNING
            << "could not send \""
            << g_name_ed_cmd_commands
            << "\" message."
            << SNAP_LOG_SEND;
    }
}



} // namespace ed
// vim: ts=4 sw=4 et

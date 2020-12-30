// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the Connection with Send Message class.
 *
 * This is a base class which ease the implementation of a connection
 * which is able to send and receive messages.
 */


// self
//
#include    "eventdispatcher/connection_with_send_message.h"

#include    "eventdispatcher/dispatcher_support.h"
#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/logger.h>
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// boost lib
//
#include    <boost/algorithm/string/join.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a connection_with_send_message object.
 *
 * This constructor initializes a connectopm which supports a send_message()
 * function. This allows that object to send a certain number of default
 * messages such as the UNKNOWN message automatically.
 */
connection_with_send_message::~connection_with_send_message()
{
}



/** \brief Build the HELP reply and send it.
 *
 * When a daemon registers with the snapcommunicator, it sends a REGISTER
 * command. As a result, the daemon is sent a HELP command which must be
 * answered with a COMMAND and the list of commands that this connection
 * supports.
 *
 * \note
 * If the environment logger is not currently configured, this message
 * gets ignored.
 *
 * \param[in] message  The HELP message.
 *
 * \sa help()
 */
void connection_with_send_message::msg_help(message & msg)
{
    snap::NOTUSED(msg);

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
        throw event_dispatcher_implementation_error(
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


/** \brief Reply to the watchdog message ALIVE.
 *
 * To check whether a service is alive, send the ALIVE message. This
 * function builds a ABSOLUTELY reply and attaches the "serial" parameter
 * as is if present. It will also include the original "timestamp" parameter
 * when present.
 *
 * The function also adds one field named "reply_timestamp" with the Unix
 * time when the reply is being sent.
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
 * \param[in] message  The STOP message.
 */
void connection_with_send_message::msg_alive(message & msg)
{
    message absolutely;
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
            << "\" with an ABSOLULTELY message."
            << SNAP_LOG_SEND;
    }
}


/** \brief Reconfigure the logger.
 *
 * Whenever the logrotate runs or some changes are made to the log
 * definitions, the corresponding daemons need to reconfigure their
 * logger to make use of the new file and settings. This command is
 * used for this purpose.
 *
 * \note
 * If the environment logger is not currently configured, this message
 * is ignored.
 *
 * \param[in] message  The STOP message.
 */
void connection_with_send_message::msg_log(message & msg)
{
    snap::NOTUSED(msg);

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
 * \param[in] message  The STOP message.
 *
 * \sa msg_stop()
 * \sa stop()
 */
void connection_with_send_message::msg_quitting(message & msg)
{
    snap::NOTUSED(msg);

    stop(true);
}


/** \brief Calls your ready() function with the message.
 *
 * All daemons using the snapcommunicator daemon have to have a ready()
 * function which gets called once the HELP and COMMAND message were
 * handled. This is when your daemon is expected to be ready to start
 * working. Some daemon, though, start working immediately no matter
 * what (i.e. snapwatchdog and snapfirewall do work either way.)
 *
 * \param[in] message  The READY message.
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
 * This message has no implementation by default at the moment. What we
 * want to do is find a clean way to restart any service instantly.
 *
 * The RESTART message is expected to be used whenever a modification
 * to some file or the system environment somehow affects your service
 * in such a way that it requires a restart. For example, after an
 * upgrade of eventdispatcher library, you should restart all the services
 * that make use of this library. For this reason, we have a RESTART
 * message.
 *
 * The message comes with one parameter named `reason` which describes
 * why the RESTART was sent:
 *
 * \li `reason=upgrade` -- something (library/tools) was upgraded
 * \li `reason=config` -- a congiguration file was updated
 *
 * \note
 * There are currently some services that make use of a CONFIG message
 * whenever their configuration changes. Pretty much all services do
 * not support a live configuration change (because it initializes their
 * objects from the configuration data once on startup and in many cases
 * it would be very complicated to allow for changes to occur.)
 * \note
 * In those existing implementations, we really just do a restart anyway.
 *
 * \param[in] message  The RESTART message.
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
 * \param[in] message  The STOP message.
 *
 * \sa msg_quitting()
 */
void connection_with_send_message::msg_stop(message & msg)
{
    snap::NOTUSED(msg);

    stop(false);
}


/** \brief Handle the UNKNOWN message.
 *
 * Whenever we send a command to another daemon, that command can be refused
 * by sending an UNKNOWN reply. This function handles the UNKNOWN command
 * by simply recording that as an error in the logs.
 *
 * \param[in] message  The UNKNOWN message we just received.
 */
void connection_with_send_message::msg_log_unknown(message & msg)
{
    // we sent a command that the other end did not understand
    // and got an UNKNOWN reply
    //
    SNAP_LOG_ERROR
        << "we sent unknown command \""
        << msg.get_parameter("command")
        << "\" and probably did not get the expected result."
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
 *      ...
 *
 *      // ALWAYS LAST
 *      {
 *          nullptr
 *        , &my_service_connection::msg_reply_with_unknown
 *        , &ed::dispatcher<my_service_connection>::dispatcher_match::always_match
 *      }
 *  };
 * \endcode
 *
 * \param[in] message  The messageto reply to.
 */
void connection_with_send_message::msg_reply_with_unknown(message & msg)
{
    message unknown;
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
 *      commands << "MSG1";
 *      commands << "MSG2";
 *      commands << "MSG3";
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
    snap::NOTUSED(commands);

    // do nothing by default -- user is expected to overload this function
}


/** \brief The default ready() function does nothing.
 *
 * This implementation does nothing. It is expected that you reimplement
 * this function depending on your daemon's need. Most often this function
 * is the one that really starts your daemons process.
 *
 * \param[in,out] message  The READY message.
 */
void connection_with_send_message::ready(message & msg)
{
    snap::NOTUSED(msg);

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
 * calls the stop() function in order to restart the deamon. If only a
 * configuration file changed and your daemon is capable of reading the
 * new settings without a full restart, then just read that new config.
 *
 * \param[in,out] message  The RESTART message.
 *
 * \sa msg_restart()
 */
void connection_with_send_message::restart(message & msg)
{
    snap::NOTUSED(msg);

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
    snap::NOTUSED(quitting);

    // do nothing by default -- user is expected to overload this function
    //
    SNAP_LOG_WARNING
        << "default stop() function was called."
        << SNAP_LOG_SEND;
}



} // namespace ed
// vim: ts=4 sw=4 et

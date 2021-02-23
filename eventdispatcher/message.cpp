// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */


// self
//
#include    "eventdispatcher/message.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/string_replace_many.h>


// boost lib
//
#include    <boost/algorithm/string.hpp>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Parse a message from the specified paremeter.
 *
 * This function transformed the input string in a set of message
 * fields.
 *
 * The message format supported is:
 *
 * \code
 *      ( '<' sent-from-server ':' sent-from-service ' ')? ( ( server ':' )? service '/' )? command ' ' ( parameter_name '=' value ';' )*
 * \endcode
 *
 * The sender "\<sent-from-server:sent-from-service" names are added by
 * snapcommunicator when it receives a message which is destined for
 * another service (i.e. not itself). This can be used by the receiver
 * to reply back to the exact same process if it is a requirement for that
 * message (i.e. a process that sends a LOCK message, for example,
 * expects to receive the LOCKED message back as an answer.) Note that
 * it is assumed that there cannot be more than one service named
 * 'service' per server. This is enforced by the snapcommunicator
 * REGISTER function.
 *
 * \code
 *      // to reply to the exact message sender, one can use the
 *      // following two lines of code:
 *      //
 *      reply.set_server(message.get_sent_from_server());
 *      reply.set_service(message.get_sent_from_service());
 *
 *      // or use the reply_to() helper function
 *      //
 *      reply.reply_to(message);
 * \endcode
 *
 * The space after the command cannot be there unless parameters follow.
 * Parameters must be separated by semi-colons.
 *
 * The value of a parameter gets quoted when it includes a ';'. Within
 * the quotes, a Double Quote can be escaped inside by adding a backslash
 * in front of it (\"). Newline characters (as well as return carriage)
 * are also escaped using \n and \r respectively. Finally, we have to
 * escape backslashes themselves by doubling them, so \ becomes \\.
 *
 * Note that only parameter values support absolutely any character.
 * All the other parameters are limited to the latin alphabet, digits,
 * and underscores ([A-Za-z0-9_]+). Also all commands are limited
 * to uppercase letters only.
 *
 * \note
 * The input message is not saved as a cached version of the message
 * because we assume it may not be 100% optimized (canonicalized.)
 *
 * \param[in] message  The string to convert into fields in this
 *                     message object.
 *
 * \return true if the message was succesfully parsed; false when an
 *         error occurs and in that case no fields get modified.
 *
 * \sa to_message()
 * \sa get_sent_from_server()
 * \sa get_sent_from_service()
 * \sa reply_to()
 */
bool message::from_message(std::string const & original_message)
{
    std::string sent_from_server;
    std::string sent_from_service;
    std::string server;
    std::string service;
    std::string command;
    parameters_t parameters;

    // someone using telnet to test sending messages will include a '\r'
    // so run a trim on the message in case it is there
    //
    std::string msg(original_message);
    boost::trim(msg);
    char const * m(msg.c_str());

    // sent-from indicated?
    //
    if(*m != '\0' && *m == '<')
    {
        // the name of the server and server sending this message
        //
        // First ++m to skip the '<'
        //
        for(++m; *m != '\0' && *m != ':'; ++m)
        {
            if(*m == ' ')
            {
                // invalid syntax from input message
                //
                SNAP_LOG_ERROR
                    << "a message with sent_from_server must not include a space in the server name ("
                    << original_message
                    << ")."
                    << SNAP_LOG_SEND;
                return false;
            }

            sent_from_server += *m;
        }
        if(*m != '\0')
        {
            // First ++m to skip the ':'
            for(++m; *m != '\0' && *m != ' '; ++m)
            {
                sent_from_service += *m;
            }
        }
        if(*m == '\0')
        {
            // invalid syntax from input message
            //
            SNAP_LOG_ERROR
                << "a message cannot only include a 'sent from service' definition."
                << SNAP_LOG_SEND;
            return false;
        }
        // Skip the ' '
        ++m;
    }

    bool has_server(false);
    bool has_service(false);
    for(; *m != '\0' && *m != ' '; ++m)
    {
        if(*m == ':')
        {
            if(has_server
            || has_service
            || command.empty())
            {
                // we cannot have more than one ':'
                // and the name cannot be empty if ':' is used
                // we also cannot have a ':' after the '/'
                //
                SNAP_LOG_ERROR
                    << "a server name cannot be empty when specified, also it cannot include two server names and a server name after a service name was specified."
                    << SNAP_LOG_SEND;
                return false;
            }
            has_server = true;
            server = command;
            command.clear();
        }
        else if(*m == '/')
        {
            if(has_service
            || command.empty())
            {
                // we cannot have more than one '/'
                // and the name cannot be empty if '/' is used
                //
                SNAP_LOG_ERROR
                    << "a service name is mandatory when the message includes a slash (/), also it cannot include two service names."
                    << SNAP_LOG_SEND;
                return false;
            }
            has_service = true;
            service = command;
            command.clear();
        }
        else
        {
            command += *m;
        }
    }

    if(command.empty())
    {
        // command is mandatory
        //
        SNAP_LOG_ERROR
            << "a command is mandatory in a message."
            << SNAP_LOG_SEND;
        return false;
    }

    // if we have a space, we expect one or more parameters
    //
    if(*m == ' ')
    {
        for(++m; *m != '\0';)
        {
            // first we have to read the parameter name (up to the '=')
            //
            char const *s(m);
            for(; *m != '\0' && *m != '='; ++m);
            std::string const param_name(s, m - s);
            if(param_name.empty())
            {
                // parameters must have a name
                //
                SNAP_LOG_ERROR
                    << "could not accept message because an empty parameter name is not valid."
                    << SNAP_LOG_SEND;
                return false;
            }
            try
            {
                verify_message_name(param_name);
            }
            catch(event_dispatcher_invalid_message const & e)
            {
                // name is not empty, but it has invalid characters in it
                //
                SNAP_LOG_ERROR
                    << "could not accept message because parameter name \""
                    << param_name
                    << "\" is not considered valid: "
                    << e.what()
                    << SNAP_LOG_SEND;
                return false;
            }

            if(*m == '\0'
            || *m != '=')
            {
                // ?!?
                //
                SNAP_LOG_ERROR
                    << "message parameters must be followed by an equal (=) character."
                    << SNAP_LOG_SEND;
                return false;
            }
            ++m;    // skip '='

            // retrieve the parameter name at first
            //
            std::string param_value;
            if(*m == '"')
            {
                // quoted parameter
                //
                for(++m; *m != '"'; ++m)
                {
                    if(*m == '\0')
                    {
                        // closing quote (") is missing
                        //
                        SNAP_LOG_ERROR
                            << "a quoted message parameter must end with a quote (\")."
                            << SNAP_LOG_SEND;
                        return false;
                    }

                    // restored escaped double quotes
                    // (note that we do not yet restore other backslashed
                    // characters, that's done below)
                    //
                    if(*m == '\\' && m[1] != '\0' && m[1] == '"')
                    {
                        ++m;
                    }
                    // here the character may be ';'
                    //
                    param_value += *m;
                }

                // skip the quote
                //
                ++m;
            }
            else
            {
                // parameter value is found as is
                //
                for(; *m != '\0' && *m != ';'; ++m)
                {
                    param_value += *m;
                }
            }

            if(*m != '\0')
            {
                if(*m != ';')
                {
                    // this should never happend
                    //
                    SNAP_LOG_ERROR
                        << "two parameters must be separated by a semicolon (;)."
                        << SNAP_LOG_SEND;
                    return false;
                }

                // skip the ';'
                //
                ++m;
            }

            // also restore new lines and blackslashes if any
            //
            std::string const unsafe_value(snap::string_replace_many(
                    param_value,
                    {
                        { "\\\\", "\\" },
                        { "\\n", "\n" },
                        { "\\r", "\r" }
                    }));

            // we got a valid parameter, add it
            //
            parameters[param_name] = unsafe_value;
        }
    }

    f_sent_from_server = sent_from_server;
    f_sent_from_service = sent_from_service;
    f_server = server;
    f_service = service;
    f_command = command;
    f_parameters.swap(parameters);
    f_cached_message.clear();

    return true;
}


/** \brief Transform all the message parameters in a string.
 *
 * This function transforms all the message parameters in a string
 * and returns the result. The string is a message we can send over
 * TCP/IP (if you make sure to add a "\n", note that the
 * send_message() does that automatically) or over UDP/IP.
 *
 * \note
 * The function caches the result so calling the function many times
 * will return the same string and thus the function is very fast
 * after the first call (assuming you do not modify the message on
 * each call to to_message().)
 *
 * \note
 * The sent-from information gets saved in the message only if both,
 * the server name and service name it was sent from are defined.
 *
 * \exception event_dispatcher_invalid_message
 * This function raises an exception if the message command was not
 * defined since a command is always mandatory.
 *
 * \return The converted message as a string.
 *
 * \sa get_sent_from_server()
 * \sa get_sent_from_service()
 * \sa set_reply_to_server()
 * \sa set_reply_to_service()
 */
std::string message::to_message() const
{
    if(f_cached_message.empty())
    {
        if(f_command.empty())
        {
            throw event_dispatcher_invalid_message("message::to_message(): cannot build a valid message without at least a command.");
        }

        // add info about the sender
        //
        // ['<' <sent-from-server> ':' <sent-from-service> ' ']
        //
        if(!f_sent_from_server.empty()
        || !f_sent_from_service.empty())
        {
            f_cached_message += '<';
            f_cached_message += f_sent_from_server;
            f_cached_message += ':';
            f_cached_message += f_sent_from_service;
            f_cached_message += ' ';
        }

        // add server and optionally the destination server name if both are defined
        //
        // ['<' <sent-from-server> ':' <sent-from-service> ' ']
        //      [[<server> ':'] <name> '/']
        //
        if(!f_service.empty())
        {
            if(!f_server.empty())
            {
                f_cached_message += f_server;
                f_cached_message += ':';
            }
            f_cached_message += f_service;
            f_cached_message += '/';
        }

        // ['<' <sent-from-server> ':' <sent-from-service> ' ']
        //      [[<server> ':'] <name> '/'] <command>
        //
        f_cached_message += f_command;

        // add parameters if any
        //
        // ['<' <sent-from-server> ':' <sent-from-service> ' ']
        //      [[<server> ':'] <name> '/'] <command>
        //      [' ' <param1> '=' <value1>][';' <param2> '=' <value2>]...
        //
        char sep(' ');
        for(auto p : f_parameters)
        {
            f_cached_message += sep;
            f_cached_message += p.first;
            f_cached_message += '=';

            std::string safe_value(snap::string_replace_many(
                    p.second,
                    {
                        { "\\", "\\\\" },
                        { "\n", "\\n" },
                        { "\r", "\\r" }
                    }));

            if(safe_value.find(';') != std::string::npos
            || (!safe_value.empty() && safe_value[0] == '\"'))
            {
                // escape the double quotes
                //
                boost::algorithm::replace_all(safe_value, "\"", "\\\"");

                // quote the resulting parameter and save in f_cached_message
                //
                f_cached_message += '"';
                f_cached_message += safe_value;
                f_cached_message += '"';
            }
            else
            {
                // no special handling necessary
                //
                f_cached_message += safe_value;
            }

            sep = ';';
        }
    }

    return f_cached_message;
}


/** \brief Where this message came from.
 *
 * Some services send a message expecting an answer directly sent back
 * to them. Yet, those services may have multiple instances in your cluster
 * (i.e. snapcommunicator runs on all computers, snapwatchdog, snapfirewall,
 * snaplock, snapdbproxy are likely to run on most computers, etc.)
 * This parameter defines which computer the message came from. Thus,
 * you can use that information to send the message back to that
 * specific computer. The snapcommunicator on that computer will
 * then forward the message to the specified service instance.
 *
 * If empty (the default,) then the normal snapcommunicator behavior is
 * used (i.e. send to any instance of the service that is available.)
 *
 * \return The address and port of the specific service this message has to
 *         be sent to.
 *
 * \sa set_sent_from_server()
 * \sa set_sent_from_service()
 * \sa get_sent_from_service()
 */
std::string const & message::get_sent_from_server() const
{
    return f_sent_from_server;
}


/** \brief Set the name of the server that sent this message.
 *
 * This function saves the name of the server that was used to
 * generate the message. This can be used later to send a reply
 * to the service that sent this message.
 *
 * The snapcommunicator tool is actually in charge of setting this
 * parameter and you should never have to do so from your tool.
 * The set happens whenever the snapcommunicator receives a
 * message from a client. If you are not using the snapcommunicator
 * then you are welcome to use this function for your own needs.
 *
 * \param[in] sent_from_server  The name of the source server.
 *
 * \sa get_sent_from_server()
 * \sa get_sent_from_service()
 * \sa set_sent_from_service()
 */
void message::set_sent_from_server(std::string const & sent_from_server)
{
    if(f_sent_from_server != sent_from_server)
    {
        // this name can be empty and it supports lowercase
        //
        verify_message_name(sent_from_server, true);

        f_sent_from_server = sent_from_server;
        f_cached_message.clear();
    }
}


/** \brief Who sent this message.
 *
 * Some services send messages expecting an answer sent right back to
 * them. For example, the snaplock tool sends the message LOCKENTERING
 * and expects the LOCKENTERED as a reply. The reply has to be sent
 * to the exact same instance t hat sent the LOCKENTERING message.
 *
 * In order to do so, the system makes use of the server and service
 * name the data was sent from. Since the name of each service
 * registering with snapcommunicator must be unique, it 100% defines
 * the sender of the that message.
 *
 * If empty (the default,) then the normal snapcommunicator behavior is
 * used (i.e. send to any instance of the service that is available locally,
 * if not available locally, try to send it to another snapcommunicator
 * that knows about it.)
 *
 * \return The address and port of the specific service this message has to
 *         be sent to.
 *
 * \sa get_sent_from_server()
 * \sa set_sent_from_server()
 * \sa set_sent_from_service()
 */
std::string const & message::get_sent_from_service() const
{
    return f_sent_from_service;
}


/** \brief Set the name of the server that sent this message.
 *
 * This function saves the name of the service that sent this message
 * to snapcommuncator. It is set by snapcommunicator whenever it receives
 * a message from a service it manages so you do not have to specify this
 * parameter yourselves.
 *
 * This can be used to provide the name of the service to reply to. This
 * is useful when the receiver does not already know exactly who sends it
 * certain messages.
 *
 * \param[in] sent_from_service  The name of the service that sent this message.
 *
 * \sa get_sent_from_server()
 * \sa set_sent_from_server()
 * \sa get_sent_from_service()
 */
void message::set_sent_from_service(std::string const & sent_from_service)
{
    if(f_sent_from_service != sent_from_service)
    {
        // this name can be empty and it supports lowercase
        //
        verify_message_name(sent_from_service, true);

        f_sent_from_service = sent_from_service;
        f_cached_message.clear();
    }
}


/** \brief The server where this message has to be delivered.
 *
 * Some services need their messages to be delivered to a service
 * running on a specific computer. This function returns the name
 * of that server.
 *
 * If the function returns an empty string, then snapcommunicator is
 * free to send the message to any server.
 *
 * \return The name of the server to send this message to or an empty string.
 *
 * \sa set_server()
 * \sa get_service()
 * \sa set_service()
 */
std::string const & message::get_server() const
{
    return f_server;
}


/** \brief Set the name of a specific server where to send this message.
 *
 * In some cases you may want to send a message to a service running
 * on a specific server. This function can be used to specify the exact
 * server where the message has to be delivered.
 *
 * This is particularly useful when you need to send a reply to a
 * specific daemon that sent you a message.
 *
 * The name can be set to ".", which means send to a local service
 * only, whether it is available or not. This option can be used
 * to avoid/prevent sending a message to other computers.
 *
 * The name can be set to "*", which is useful to broadcast the message
 * to all servers even if the destination service name is
 * "snapcommunicator".
 *
 * \param[in] server  The name of the server to send this message to.
 *
 * \sa get_server()
 * \sa get_service()
 * \sa set_service()
 */
void message::set_server(std::string const & server)
{
    if(f_server != server)
    {
        // this name can be empty and it supports lowercase
        //
        if(server != "."
        && server != "*")
        {
            verify_message_name(server, true);
        }

        f_server = server;
        f_cached_message.clear();
    }
}


/** \brief Retrieve the name of the service the message is for.
 *
 * This function returns the name of the service this message is being
 * sent to.
 *
 * \return Destination service.
 *
 * \sa get_server()
 * \sa set_server()
 * \sa set_service()
 */
std::string const & message::get_service() const
{
    return f_service;
}


/** \brief Set the name of the service this message is being sent to.
 *
 * This function specifies the name of the server this message is expected
 * to be sent to.
 *
 * When a service wants to send a message to snapcommunicator, no service
 * name is required.
 *
 * \param[in] service  The name of the destination service.
 *
 * \sa get_server()
 * \sa set_server()
 * \sa get_service()
 */
void message::set_service(std::string const & service)
{
    if(f_service != service)
    {
        // broadcast is a special case that the verify_message_name() does not
        // support
        //
        if(service != "*"
        && service != "?"
        && service != ".")
        {
            // this name can be empty and it supports lowercase
            //
            verify_message_name(service, true);
        }

        f_service = service;
        f_cached_message.clear();
    }
}


/** \brief Copy sent information to this message.
 *
 * This function copies the sent information found in message
 * to this message server and service names.
 *
 * This is an equivalent to the following two lines of code:
 *
 * \code
 *      reply.set_server(message.get_sent_from_server());
 *      reply.set_service(message.get_sent_from_service());
 * \endcode
 *
 * \param[in] original_message  The source message you want to reply to.
 */
void message::reply_to(message const & original_message)
{
    set_server(original_message.get_sent_from_server());
    set_service(original_message.get_sent_from_service());
}


/** \brief Get the command being sent.
 *
 * Each message is an equivalent to an RPC command being send between
 * services.
 *
 * The command is a string of text, generally one or more words
 * concatenated (no space allowed) such as STOP and LOCKENTERING.
 *
 * \note
 * The command string may still be empty if it was not yet assigned.
 *
 * \return The command of this message.
 */
std::string const & message::get_command() const
{
    return f_command;
}


/** \brief Set the message command.
 *
 * This function is used to define the RPC-like command of this message.
 *
 * The name of the command gets verified using the verify_message_name() function.
 * It cannot be empty and all letters have to be uppercase.
 *
 * \param[in] command  The command to send to a connection.
 *
 * \sa verify_message_name()
 */
void message::set_command(std::string const & command)
{
    // this name cannot be empty and it does not support lowercase
    // characters either
    //
    verify_message_name(command, false, false);

    if(f_command != command)
    {
        f_command = command;
        f_cached_message.clear();
    }
}


/** \brief Retrieve the message version this library was compiled with.
 *
 * This function returns the MESSAGE_VERSION that this library was
 * compiled with. Since we offer a shared object (.so) library, it
 * could be different from the version your application was compiled
 * with. If that's the case, your application may want to at least
 * warn the user about the dicrepancy.
 *
 * \return The MESSAGE_VERSION at the time this library was compiled.
 */
message_version_t message::get_message_version() const
{
    return MESSAGE_VERSION;
}


/** \brief Check the version parameter.
 *
 * This function retrieves the version parameter which has to exist and
 * be an integer. If this is not the case, then an exception is raised.
 *
 * If the version is defined, it gets checked against this library's
 * compile time MESSAGE_VERSION variable. If equal, then the function
 * returns true. Otherwise, it returns false.
 *
 * In most cases, the very first message that you send with your service
 * should be such that it includes the version the libeventdispatcher
 * your program is linked against. In other words, you want to call
 * the add_version_parameter() in your sender and call this function
 * in your receiver.
 *
 * Make sure to design a way to disconnect cleanly so the other
 * party knows that the communication is interrupted because the
 * versions do not match. This allows your application to prevent
 * re-connecting over and over again when it knows it will fail
 * each time.
 *
 * \exception event_dispatcher_invalid_message
 * If you call this function and no version parameter was added to
 * the message, then this exception is raised.
 *
 * \return true if the version is present and valid.
 */
bool message::check_version_parameter() const
{
    return get_integer_parameter(MESSAGE_VERSION_NAME) == MESSAGE_VERSION;
}


/** \brief Add version parameter.
 *
 * Add a parameter named `"version"` with the current version of the
 * message protocol.
 *
 * In the snapcommunicator tool, this is sent over with the CONNECT
 * message. It allows the snapcommunicators to make sure they will
 * properly understand each others.
 */
void message::add_version_parameter()
{
    add_parameter(MESSAGE_VERSION_NAME, MESSAGE_VERSION);
}


/** \brief Add a string parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text. Especially, we
 * manage the '\0' character as the end of the message.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, std::string const & value)
{
    verify_message_name(name);

    f_parameters[name] = value;
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, int32_t value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, uint32_t value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, long long value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, unsigned long long value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, int64_t value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Add an integer parameter to the message.
 *
 * Messages can include parameters (variables) such as a URI or a word.
 *
 * The value is not limited, although it probably should be limited to
 * standard text as these messages are sent as text.
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 *
 * \sa verify_message_name()
 */
void message::add_parameter(std::string const & name, uint64_t value)
{
    verify_message_name(name);

    f_parameters[name] = std::to_string(value);
    f_cached_message.clear();
}


/** \brief Check whether a parameter is defined in this message.
 *
 * This function checks whether a parameter is defined in a message. If
 * so it returns true. This is important because the get_parameter()
 * functions throw if the parameter is not available (i.e. which is
 * what is used for mandatory parameters.)
 *
 * The parameter name is verified by the verify_message_name() function.
 *
 * \param[in] name  The name of the parameter.
 *
 * \return true if that parameter exists.
 *
 * \sa verify_message_name()
 */
bool message::has_parameter(std::string const & name) const
{
    verify_message_name(name);

    return f_parameters.find(name) != f_parameters.end();
}


/** \brief Retrieve a parameter as a string from this message.
 *
 * This function retrieves the named parameter from this message as a string,
 * which is the default.
 *
 * The name must be valid as defined by the verify_message_name() function.
 *
 * \note
 * This function returns a copy of the parameter so if you later change
 * the value of that parameter, what has been returned does not change
 * under your feet.
 *
 * \exception event_dispatcher_invalid_message
 * This exception is raised whenever the parameter is not defined or
 * if the parameter \p name is not considered valid.
 *
 * \param[in] name  The name of the parameter.
 *
 * \return A copy of the parameter value.
 *
 * \sa verify_message_name()
 */
std::string message::get_parameter(std::string const & name) const
{
    verify_message_name(name);

    auto const it(f_parameters.find(name));
    if(it != f_parameters.end())
    {
        return it->second;
    }

    throw event_dispatcher_invalid_message(
              "message::get_parameter(): parameter \""
            + name
            + "\" of command \""
            + f_command
            + "\" not defined, try has_parameter() before calling"
              " the get_parameter() function.");
}


/** \brief Retrieve a parameter as an integer from this message.
 *
 * This function retrieves the named parameter from this message as a string,
 * which is the default.
 *
 * The name must be valid as defined by the verify_message_name() function.
 *
 * \exception event_dispatcher_invalid_message
 * This exception is raised whenever the parameter is not a valid integer,
 * it is not set, or the parameter name is not considered valid.
 *
 * \param[in] name  The name of the parameter.
 *
 * \return The parameter converted to an integer.
 *
 * \sa verify_message_name()
 */
std::int64_t message::get_integer_parameter(std::string const & name) const
{
    verify_message_name(name);

    auto const it(f_parameters.find(name));
    if(it != f_parameters.end())
    {
        std::int64_t r;
        if(!advgetopt::validator_integer::convert_string(it->second, r))
        {
            throw event_dispatcher_invalid_message(
                      "message::get_integer_parameter(): command \""
                    + f_command
                    + "\" expected integer for \""
                    + name
                    + "\" but \""
                    + it->second
                    + "\" could not be converted.");
        }
        return r;
    }

    throw event_dispatcher_invalid_message(
                  "message::get_integer_parameter(): parameter \""
                + name
                + "\" of command \""
                + f_command
                + "\" not defined, try has_parameter() before calling"
                  " the get_integer_parameter() function.");
}


/** \brief Retrieve the list of parameters from this message.
 *
 * This function returns a constant reference to the list of parameters
 * defined in this message.
 *
 * This can be useful if you allow for variable lists of parameters, but
 * generally the get_parameter() and get_integer_parameter() are prefered.
 *
 * \warning
 * This is a direct reference to the list of parameter. If you call the
 * add_parameter() function, the new parameter will be visible in that
 * new list and an iterator is likely not going to be valid on return
 * from that call.
 *
 * \return A constant reference to the list of message parameters.
 *
 * \sa get_parameter()
 * \sa get_integer_parameter()
 */
message::parameters_t const & message::get_all_parameters() const
{
    return f_parameters;
}


/** \brief Verify various names used with messages.
 *
 * The messages use names for:
 *
 * \li commands
 * \li services
 * \li parameters
 *
 * All those names must be valid as per this function. They are checked
 * on read and on write (i.e. add_parameter() and get_paramter() both
 * check the parameter name to make sure you did not mistype it.)
 *
 * A valid name must start with a letter or an underscore (although
 * we suggest you do not start names with underscores; we want to
 * have those reserved for low level system like messages,) and
 * it can only include letters, digits, and underscores.
 *
 * The letters are limited to uppercase for commands. Also certain
 * names may be empty (See concerned functions for details on that one.)
 *
 * \note
 * The allowed letters are 'a' to 'z' and 'A' to 'Z' only. The allowed
 * digits are '0' to '9' only. The underscore is '_' only.
 *
 * A few valid names:
 *
 * \li commands: PING, STOP, LOCK, LOCKED, QUITTING, UNKNOWN, LOCKEXITING
 * \li services: snapcommunicator, snapserver, snaplock, MyOwnService
 * \li parameters: URI, name, IP, TimeOut
 *
 * At this point all our services use lowercase, but this is not enforced.
 * Actually, mixed case or uppercase service names are allowed.
 *
 * \exception event_distpatcher_invalid_message
 * This exception is raised if the name includes characters considered
 * invalid.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] can_be_empty  Whether the name can be empty.
 * \param[in] can_be_lowercase  Whether the name can include lowercase letters.
 */
void verify_message_name(std::string const & name, bool can_be_empty, bool can_be_lowercase)
{
    if(!can_be_empty
    && name.empty())
    {
        std::string err("a message name cannot be empty.");
        SNAP_LOG_FATAL
            << err
            << SNAP_LOG_SEND;
        throw event_dispatcher_invalid_message(err);
    }

    for(auto const & c : name)
    {
        if((c < 'a' || c > 'z' || !can_be_lowercase)
        && (c < 'A' || c > 'Z')
        && (c < '0' || c > '9')
        && c != '_')
        {
            std::string err("a message name must be composed of ASCII"
                            " 'a'..'z', 'A'..'Z', '0'..'9', or '_'"
                            " only (also a command must be uppercase only,) \"");
            err += name;
            err += "\" is not valid.";
            SNAP_LOG_FATAL
                << err
                << SNAP_LOG_SEND;
            throw event_dispatcher_invalid_message(err);
        }
    }

    if(!name.empty()
    && name[0] >= '0' && name[0] <= '9')
    {
        std::string err("parameter name cannot start with a digit, \"");
        err += name;
        err += "\" is not valid.";
        SNAP_LOG_FATAL
            << err
            << SNAP_LOG_SEND;
        throw event_dispatcher_invalid_message(err);
    }
}




} // namespace ed
// vim: ts=4 sw=4 et

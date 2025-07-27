// Copyright (c) 2020-2025  Made to Order Software Corp.  All Rights Reserved
// All rights reserved
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

// self
//
#include    "eventdispatcher/logrotate_udp_messenger.h"

#include    "eventdispatcher/communicator.h"


// libaddr
//
#include    <libaddr/addr_parser.h>


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief The UDP server initialization.
 *
 * The UDP server creates a new UDP server to listen on incoming
 * messages over a UDP connection.
 *
 * This UDP implementation is used to listen to the `LOG_ROTATE` message
 * specifically. It is often that you want to restart the snaplogger
 * and this can be used as the way to do it. The UDP message can be
 * sent using the ed-signal command line tool:
 *
 * \code
 *     ed-signal --server 127.0.0.1:1234 --message LOG_ROTATE --type udp
 * \endcode
 *
 * The reaction of the server is to restart the snaplogger as implemented in
 * the default dispatcher messages (see the messages.cpp for details).
 *
 * If you already have a UDP service in your application, you can simply
 * add the default dispatcher messages and the `LOG_ROTATE` message will automatically
 * be handled for you. No need to create yet another network connection.
 * We do so in this constructor like so:
 *
 * \code
 *     f_dispatcher->add_communicator_commands();
 * \endcode
 *
 * \param[in] address  The address/port to listen on. Most often it is the private
 * address (127.0.0.1).
 * \param[in] secret_code  A secret code to verify in order to accept UDP
 * messages.
 */
logrotate_udp_messenger::logrotate_udp_messenger(
          addr::addr const & address
        , std::string const & secret_code)
    : ed::udp_server_message_connection(address)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("logrotate_udp_messenger");
    set_secret_code(secret_code);
    f_dispatcher->add_communicator_commands();
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);
}


/** \brief The logrotate messenger destructor.
 *
 * This function is here because the logrotate_udp_messenger derives from
 * classes that have virtual functions and the destructor of such classes
 * have to be defined virtual.
 */
logrotate_udp_messenger::~logrotate_udp_messenger()
{
}



namespace
{


/** \brief Options to handle the logrotate UDP IP:port.
 *
 */
advgetopt::option const g_options[] =
{
    // LOGROTATE OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("logrotate-listen")
        , advgetopt::ShortName(U'R')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("the host and port to listen on for `LOG_ROTATE` messages.")
    ),
    advgetopt::define_option(
          advgetopt::Name("logrotate-secret")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("")
        , advgetopt::Help("a secret code to be used along the logrotate-listen option; use empty (the default) to not have to use a secret code.")
    ),

    // END
    //
    advgetopt::end_options()
};

} // no name namespace



logrotate_extension::logrotate_extension(
          advgetopt::getopt & opts
        , std::string const & default_address
        , int default_port)
    : f_opts(opts)
    , f_default_address(default_address)
    , f_default_port(default_port)
{
}


logrotate_extension::~logrotate_extension()
{
}


void logrotate_extension::add_logrotate_options()
{
    // add options
    //
    f_opts.parse_options_info(g_options, true);

    // add the default of the --logrotate-listen option manually so it can
    // be dynamic
    //
    advgetopt::option_info::pointer_t o(f_opts.get_option("logrotate-listen"));
    if(o == nullptr)
    {
        SNAP_LOG_ERROR
            << "somehow the \"--logrotate-listen\" option was not added and we can't find it."
            << SNAP_LOG_SEND;
        return;
    }

    if(!f_default_address.empty())
    {
        if(f_default_port > 0)
        {
            o->set_default(f_default_address + ':' + std::to_string(f_default_port));
        }
        else
        {
            o->set_default(f_default_address);
        }
    }
    else if(f_default_port > 0)
    {
        // TODO: make sure this works, otherwise use 127.0.0.1 as the default IP
        //
        o->set_default(':' + std::to_string(f_default_port));
    }

}


/** \brief Call this function after you finalized option processing.
 *
 * This function acts on the logrotate various command line options.
 * Assuming the command line options were valid, this function will
 * open a UDP port which will listen for `LOG_ROTATE` messages . On such
 * a message, it will trigger a reopening of any file the logger
 * is dealing with.
 *
 * Once you are ready to quit your process, make sure to call the
 * disconnect_logrotate_messenger() function to remove this logrotate
 * extension from the communicator. Not doing so would block the
 * communicator since it would continue to listen for `LOG_ROTATE` messages.
 */
void logrotate_extension::process_logrotate_options()
{
    addr::addr const logrotate_addr(addr::string_to_addr(
          f_opts.get_string("logrotate-listen")
        , f_default_address
        , f_default_port
        , "udp"));

    f_logrotate_messenger = std::make_shared<ed::logrotate_udp_messenger>(
          logrotate_addr
        , f_opts.get_string("logrotate-secret"));

    ed::communicator::instance()->add_connection(f_logrotate_messenger);
}


/** \brief Remove the logrotate UDP messenger from the communicator.
 *
 * Once you are ready to quit, make sure to remove the UDP messenger from
 * the communicator by calling this function. You can safely call this
 * function multiple times. You can also call this function early (before
 * quitting), just keep in mind that means you won't get your logs rotated
 * anymore if you do so too early.
 */
void logrotate_extension::disconnect_logrotate_messenger()
{
    ed::communicator::instance()->remove_connection(f_logrotate_messenger);
    f_logrotate_messenger.reset();
}



} // namespace ed
// vim: ts=4 sw=4 et

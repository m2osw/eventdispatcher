// Copyright (c) 2020-2021 Virtual Entertainment LLC
// All rights reserved
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
#pragma once

/** \file
 * \brief A UDP implementation to specifically handle LOG messages.
 *
 * This class is a UDP implementation which is capable of handling the LOG
 * message to restart the snaplogger in your application.
 *
 * The idea is that this functionality is very often useful and used in
 * your logrotate script as in:
 *
 * \code
 *    /var/log/snapwebsites/\*.log {
 *        ...
 *        postrotate
 *            /usr/bin/ed-signal --server 127.0.0.1:1234 --message LOG --type udp
 *        endscript
 *        ...
 *    }
 * \endcode
 *
 * The ed-signal tool is very useful in this case to send that LOG message.
 * The UDP channel is perfect because we do not expect a reply and can send
 * the message and exit right away.
 *
 * To use this class, add it as a member of your class:
 *
 * \code
 *     class MyClass
 *     {
 *     ...
 *     private:
 *         logrotate_udp_messenger::pointer_t    f_logrotate_messenger = logrotate_udp_messenger::pointer_t();
 *     ...
 *     };
 * \endcode
 *
 * Instantiate it in your constructor as in: (TODO: this was moved to the
 * logrotate_extension class, update these docs!)
 *
 * See also the logrotate_extension which does the f_opts management for you.
 *
 * \code
 *     std::string const logrotate_listen(f_opts.get_string("logrotate-listen"));
 *     if(logrotate_listen.empty())
 *     {
 *         SNAP_LOG_FATAL
 *             << "the \"logrotate_listen=...\" must be defined."
 *             << SNAP_LOG_SEND;
 *         throw std::runtime_error("the \"logrotate_listen=...\" parameter must be defined.");
 *     }
 *     addr::addr const logrotate_addr(addr::string_to_addr(
 *           logrotate_listen
 *         , "127.0.0.1"
 *         , DEFAULT_LOGROTATE_UDP_PORT
 *         , "udp"));
 *
 *     f_logrotate_udp_messenger = std::make_shared<ed::logrotate_udp_messenger>(
 *           logrotate_udp_addr
 *         , f_opts.get_string("logrotate_secret_code"));
 *     f_communicator->add_connection(f_logrotate_messenger);
 * \endcode
 *
 * Note that the `logrotate_secret_code` is not required. You can set it to
 * the empty string, especially if this code is to run on backend computers.
 *
 * In order to quit your application cleanly, don't forget to remove that
 * connection from the communicator:
 *
 * \code
 *     f_communicator->remove_connection(f_logrotate_messenger);
 * \endcode
 *
 * \todo
 * Update example with the logrotate_extension.
 *
 * \todo
 * Look into an implementation that uses the local stream UDP instead of
 * an INET UDP. This will make it a lot safer.
 */


// self
//
#include    "eventdispatcher/connection_with_send_message.h"
#include    "eventdispatcher/dispatcher.h"
#include    "eventdispatcher/udp_server_message_connection.h"


// addr lib
//
#include    <libaddr/addr.h>


// advgetopt lib
//
#include    <advgetopt/advgetopt.h>




namespace ed
{


/** \brief Accept messages over UDP.
 *
 * This class is an implementation of a UDP service used to listen for
 * logrotate changes and re-start the logger.
 *
 * Whenever logrotate rotates the logs, apps need to close the old log
 * (now .log.1) and open the new log (just .log). This is not automatic
 * in the app. (although we could have an auto-restart a little after
 * logrotate is expected to be done, that would not be as accurate as
 * the log rotation could easily be missed).
 */
class logrotate_udp_messenger
    : public ed::udp_server_message_connection
    , public ed::connection_with_send_message
{
public:
    typedef std::shared_ptr<logrotate_udp_messenger>              pointer_t;
    typedef ed::dispatcher<logrotate_udp_messenger>::pointer_t    dispatcher_pointer_t;

                            logrotate_udp_messenger(
                                      addr::addr const & address
                                    , std::string const & secret_code);
    virtual                 ~logrotate_udp_messenger() override;

    // connection_with_send_message implementation
    virtual bool            send_message(ed::message & msg, bool cache = false) override;

private:
    dispatcher_pointer_t    f_dispatcher = dispatcher_pointer_t();
};


class logrotate_extension
{
public:
                            logrotate_extension(
                                      advgetopt::getopt & opts
                                    , std::string const & default_address
                                    , int default_port);

    void                    add_logrotate_options();
    void                    process_logrotate_options();
    void                    disconnect_logrotate_messenger();

private:
    advgetopt::getopt &     f_opts;
    std::string             f_default_address = std::string();
    int                     f_default_port = -1;
    ed::logrotate_udp_messenger::pointer_t
                            f_logrotate_messenger = ed::logrotate_udp_messenger::pointer_t();
};


} // namespace ed
// vim: ts=4 sw=4 et

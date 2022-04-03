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
 * \brief Implementation of the Snap! Communicator.
 *
 * This is the crux of this application, the service which manages all the
 * communication between all the other services (well nearly all of them).
 * The Snap! Communicator is what is called an RPC service. You use it
 * to send messages to various services to run commands and get replies.
 *
 * In most cases, the send_message() will be instantaneous. You must have
 * an event loop (see the ed::communicator object) to wait for replies.
 */

// self
//
#include    "server.h"

#include    "remote_snapcommunicator.h"


// advgetopt
//
#include    <advgetopt/options.h>


//// snapwebsites
////
//#include <snapwebsites/chownnm.h>
//#include <snapwebsites/flags.h>
//#include <snapwebsites/glob_dir.h>
//#include <snapwebsites/loadavg.h>
//#include <snapwebsites/log.h>
//#include <snapwebsites/qcompatibility.h>
//#include <snapwebsites/snap_communicator.h>
//#include <snapwebsites/snapwebsites.h>


// snapdev lib
//
//#include <snapdev/not_used.h>
//#include <snapdev/tokenize_string.h>


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



namespace
{


const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("certificate")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("certificate for --secure-listen connections.")
    ),
    advgetopt::define_option(
          advgetopt::Name("debug-all-messages")
        , advgetopt::Flags(advgetopt::standalone_flag<
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("log all the messages received by the snapcommunicator (except lock & log related because it creates issues).")
    ),
    advgetopt::define_option(
          advgetopt::Name("local-listen")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("<IP:port> to open a local TCP connection (no encryption).")
    ),
    advgetopt::define_option(
          advgetopt::Name("max-connections")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("100")
        , advgetopt::Help("maximum number of connections allowed by this snapcommunicator.")
    ),
    advgetopt::define_option(
          advgetopt::Name("max-pending-connections")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("25")
        , advgetopt::Help("maximum number of client connections waiting to be accepted.")
    ),
    advgetopt::define_option(
          advgetopt::Name("private-key")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("private key for --secure-listen connections.")
    ),
    advgetopt::define_option(
          advgetopt::Name("remote-listen")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("<IP:port> to open a remote TCP connection (no encryption).")
    ),
    advgetopt::define_option(
          advgetopt::Name("secure-listen")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("<IP:port> to open a remote TCP connection (with encryption, requires the --certificate & --private-key).")
    ),
    advgetopt::define_option(
          advgetopt::Name("server-name")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("name of this server, defaults to `hostname` if undefined here.")
    ),
    advgetopt::define_option(
          advgetopt::Name("services")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("/usr/share/snapwebsites/services")
        , advgetopt::Help("path to the list of service files.")
    ),
    advgetopt::define_option(
          advgetopt::Name("unix-listen")
        , advgetopt::Flags(advgetopt::all_flag<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("a Unix socket name to listen for local connections.")
    ),
    advgetopt::end_options()
};

advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};

constexpr char const * const g_configuration_files[] =
{
    "/etc/eventdispatcher/ed-signal.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapcommunicator",
    .f_group_name = "eventdispatcher",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPCOMMUNICATOR",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = EVENTDISPATCHER_VERSION_STRING,
    .f_license = "GPL v2 or newer",
    .f_copyright = "Copyright (c) 2012-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop

}
// noname namespace




/** \class server
 * \brief Set of connections in the snapcommunicator tool.
 *
 * All the connections and sockets in general will all appear
 * in this class.
 */




/** \brief Construct the server object.
 *
 * This function saves the server pointer in the server
 * object. It is used later to gather various information and call helper
 * functions.
 */
server::server(int argc, char * argv[])
    : f_opts(g_options_environment)
    , f_logrotate(f_opts, "127.0.0.1", 4988)
{
    snaplogger::add_logger_options(f_opts);
    f_logrotate.add_logrotate_options();
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/eventdispatcher/logger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }
    f_logrotate.process_logrotate_options();
}


/** \brief Initialize the server.
 *
 * This function is used to initialize the connetions object. This means
 * setting up a few parameters such as the nice level of the application
 * and priority scheme for listening to events.
 *
 * Then it creates two sockets: one listening on TCP/IP and the other
 * listening on UDP/IP. The TCP/IP is for other servers to connect to
 * and listen communicate various status between various servers. The
 * UDP/IP is used to very quickly send messages between servers. The
 * UDP/IP messages are viewed as signals to wake up a server so it
 * starts working on new data (in most cases, at least.)
 *
 * \return 0 on success, 1 on error.
 */
int server::init()
{
    // keep a copy of the server name handy
    f_server_name = f_opts.is_defined("server_name");
    if(f_server_name.empty())
    {
        f_server_name = snapdev::gethostname();
    }

    f_number_of_processors = std::max(1U, std::thread::hardware_concurrency());

    f_debug_all_messages = advgetopt::is_true(f_opts.is_defined("debug_all_messages"));

    // check a user defined maximum number of connections
    // by default this is set to SNAP_COMMUNICATOR_MAX_CONNECTIONS,
    // which at this time is 100
    //
    f_max_connections = f_opts.get_integer("max_connections");

    // read the list of available services
    //
    {
        std::string path_to_services(f_opts.get_parameter("services"));
        path_to_services += "/*.service";
        typedef std::set<std::string> service_files_t;
        snapdev::glob_to_list<service_files_t> dir;
        if(dir.read_path<snapdev::glob_to_list_flag_t::GLOB_NOESCAPE>(path_to_services))
        {
            // we have some local services (note that snapcommunicator is
            // not added as a local service)
            //
            dir.enumerate_glob([&](std::string const & path)
                    {
                        if(path.length() > 8)
                        {
                            int const s(path.rfind('/') + 1);
                            int e(path.length() - s);
                            std::string end(path.substr(path.length() - 8));
                            if(end == ".service")
                            {
                                e -= 8;
                            }
                            std::string const key(path.substr(s, e));
                            f_local_services_list[key] = true;
                        }
                    });
        }
        else
        {
            SNAP_LOG_ERROR
                << "search of services failed: "
                << get_last_error_message()
                << SNAP_LOG_SEND;
        }

        // the list of local services cannot (currently) change while
        // snapcommunicator is running so generate the corresponding
        // string once
        //
        f_local_services = f_local_services_list.keys().join(",");
    }

    f_communicator = ed::communicator::instance();

    // capture Ctrl-C (SIGINT)
    //
    f_interrupt = std::make_shared<interrupt>(shared_from_this());
    f_communicator->add_connection(f_interrupt);

    int const max_pending_connections(f_opts.get_integer("max_pending_connections"));
    if(max_pending_connections < 5
    || max_pending_connections > 1000)
    {
        SNAP_LOG_FATAL
            << "the --max-pending-connections option must be a valid number between 5 and 1000. "
            << max_pending_connections
            << " is not valid."
            << SNAP_LOG_SEND;
        return 1;
    }

    // create two listeners, for new arriving TCP/IP connections
    //
    // one listener is used to listen for local services which have to
    // connect using the 127.0.0.1 IP address
    //
    // the other listener listens to your local network and accepts
    // connections from other snapcommunicator servers
    //
    // TCP local
    {
        addr::addr listen_addr(addr::string_to_addr(
                  f_opts.get_string("local_listen")
                , "0.0.0.0"
                , 4040
                , "tcp"));
        if(listen_addr.get_network_type() != addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            SNAP_LOG_FATAL
                << "The --local-listen option must be a loopback IP address. "
                << f_opts.get_string("local_listen")
                << " is not acceptable."
                << SNAP_LOG_SEND;
            return 1;
        }

        // make this listener the local listener
        //
        f_local_listener = std::shared_ptr<listener>(
                                  shared_from_this()
                                , addr
                                , std::string()
                                , std::string()
                                , max_pending_connections
                                , true
                                , f_server_name);
        f_local_listener->set_name("snap communicator local listener");
        f_communicator->add_connection(f_local_listener);
    }
    // unix
    if(f_opts.is_defined("unix_listen"))
    {
        addr::unix listen_addr(addr::unix(f_opts.get_string("unix_listen"));

        f_unix_listener = std::make_shared<unix_listener>(
                  shared_from_this()
                , listen_addr
                , max_pending_connections
                , false
                , f_server_name);
        f_secure_listener->set_name("snap communicator secure listener");
        f_communicator->add_connection(f_listener);
    }
    // plain remote
    std::string const listen_str(f_opts.get_string("remote_listen"));
    addr::addr listen_addr(addr::string_to_addr(listen_str, "0.0.0.0", 4040, "tcp"));
    {
        // make this listener the remote listener, however, if the IP
        // address is 127.0.0.1 we skip on this one, we do not need
        // two listener on the local IP address
        //
        if(listen_addr.get_network_type() != addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            f_public_ip = listen_addr.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS);
            f_remote_listener = std::shared_ptr<listener>(
                      shared_from_this()
                    , listen_addr
                    , std::string()
                    , std::string()
                    , max_pending_connections
                    , false
                    , f_server_name);
            f_remote_listener->set_name("snap communicator remote listener");
            f_communicator->add_connection(f_listener);
        }
        else
        {
            SNAP_LOG_WARNING
                << "remote \"listen\" parameter is \""
                << listen_str
                << "\" (local loopback) so it is ignored and no remote connections will be possible."
                << SNAP_LOG_SEND;
        }
    }
    // secure remote
    std::string const certificate(f_server->get_parameter("certificate").toUtf8().data());
    std::string const private_key(f_server->get_parameter("private_key").toUtf8().data());
    if(!certificate.empty()
    && !private_key.empty()
    && f_opts.is_defined("secure_listen"))
    {
        addr::addr listen_addr(addr::string_to_addr(
                  f_opts.get_string("secure_listen")
                , "0.0.0.0"
                , 4041
                , "tcp"));

        // make this listener the remote listener, however, if the IP
        // address is 127.0.0.1 we skip on this one, we do not need
        // two listener on the local IP address
        //
        if(listen_addr.get_network_type() != addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
        {
            f_secure_listener = std::make_shared<listener>(
                      shared_from_this()
                    , listen_addr
                    , certificate
                    , private_key
                    , max_pending_connections
                    , false
                    , f_server_name);
            f_secure_listener->set_name("snap communicator secure listener");
            f_communicator->add_connection(f_listener);
        }
        else
        {
            SNAP_LOG_WARNING
                << "remote \"secure_listen\" parameter is \""
                << listen_str
                << "\" (local loopback) so it is ignored and no secure remote connections will be possible."
                << SNAP_LOG_SEND;
        }
    }
    else
    {
        SNAP_LOG_INFO
            << "no certificate, private key, or secure-listen was defined, no secure connection will be possible."
            << SNAP_LOG_SEND;
    }

    {
        QString addr("127.0.0.1"); // this default should work just fine
        int port(4041);
        tcp_client_server::get_addr_port(f_server->get_parameter("signal"), addr, port, "tcp");

        ping_impl::pointer_t ping(new ping_impl(shared_from_this(), addr.toUtf8().data(), port));
        ping->set_secret_code(f_server->get_parameter("signal_secret").toUtf8().data());
        ping->set_name("snap communicator messenger (UDP)");
        f_ping = ping;
        f_communicator->add_connection(f_ping);
    }

    {
        f_loadavg_timer.reset(new load_timer(shared_from_this()));
        f_loadavg_timer->set_name("snap communicator load balancer timer");
        f_communicator->add_connection(f_loadavg_timer);
    }

    // transform the my_address to a snap_addr::addr object
    //
    f_my_address = addr::addr(addr::string_to_addr(f_server->get_parameter("my_address").toUtf8().data(), "", listen_addr.get_port(), "tcp"));
    bool const cia(addr::find_addr_interface(f_my_address, false) != nullptr);
    if(!cia)
    {
        std::string const addr(f_my_address.to_ipv6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS));
        SNAP_LOG_FATAL("my_address \"")(addr)("\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?");
        throw snap::snap_exception(QString("my_address \"%1\" not found on this computer. Did a copy of the configuration file and forgot to change that entry?.").arg(QString::fromUtf8(addr.c_str())));
    }

    f_remote_snapcommunicators.reset(new remote_communicator_connections(shared_from_this(), f_my_address));

    // the add_neighbors() function parses the list of neighbors and
    // creates a permanent connection
    //
    // note that the first time add_neighbors is called it reads the
    // list of cached neighbor IP:port info and connects those too
    //
    // note how we first add ourselves, this is important to get the
    // correct size() when defining the CLUSTERUP/DOWN neighbors_count
    // parameter although we do not want to add 127.0.0.1 as an IP
    //
    if(listen_addr.get_network_type() != addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK)
    {
        add_neighbors(listen_str);
    }
    else
    {
        // this is a problem so flag it otherwise we are likely to miss it!
        //
        std::stringstream ss;

        ss << "the snapcommunicator \"listen="
           << listen_str
           << "\" parameter is the loopback IP address."
             " This will prevent any tool that wants to make use of the"
             " CLUSTERUP, CLUSTERDOWN, CLUSTERCOMPLETE, and CLUSTERINCOMPLETE"
             " (and query CLUSTERSTATUS) messages.";

        SNAP_LOG_ERROR(ss.str());

        snap::snap_flag::pointer_t flag(SNAP_FLAG_UP(
                      "snapcommunicator"
                    , "cluster"
                    , "no-cluster"
                    , ss.str()));
        flag->set_priority(82);
        flag->add_tag("initialization");
        flag->add_tag("network");
        flag->save();
    }
    f_explicit_neighbors = canonicalize_neighbors(f_server->get_parameter("neighbors"));
    add_neighbors(f_explicit_neighbors);

    // if we are in a one computer environment this call would never happens
    // unless someone sends us a CLUSTERSTATUS, but that does not have the
    // exact same effect
    //
    cluster_status(nullptr);
}


void server::drop_privileges()
{
    // drop to non-priv user/group if we are root
    // (i.e. this code is skip on programmer's machines)
    //
    if( getuid() == 0 )
    {
        // Group first, then user. Otherwise you lose privs to change your group!
        //
        {
            struct group const * grp(getgrnam(f_groupname.toUtf8().data()));
            if( nullptr == grp )
            {
                SNAP_LOG_FATAL("Cannot locate group \"")(f_groupname)("\"! Create it first, then run the server.");
                throw snap::snap_exception(QString("Cannot locate group \"%1\"! Create it first, then run the server.")
                                    .arg(f_groupname));
            }
            int const sw_grp_id(grp->gr_gid);
            //
            if( setegid( sw_grp_id ) != 0 )
            {
                int const e(errno);
                SNAP_LOG_FATAL("Cannot drop privileges to group \"")(f_groupname)("\"! errno: ")(e)(", ")(strerror(e));
                throw snap::snap_exception(QString("Cannot drop privileges group \"%1\"!")
                                    .arg(f_groupname));
            }
        }
        //
        {
            struct passwd const * pswd(getpwnam(f_username.toUtf8().data()));
            if( nullptr == pswd )
            {
                SNAP_LOG_FATAL("Cannot locate user \"")(f_username)("\"! Create it first, then run the server.");
                throw snap::snap_exception(QString("Cannot locate user \"%1\"! Create it first, then run the server.")
                                    .arg(f_username));
            }
            int const sw_usr_id(pswd->pw_uid);
            //
            if( seteuid( sw_usr_id ) != 0 )
            {
                int const e(errno);
                SNAP_LOG_FATAL("Cannot drop privileges to user \"")(f_username)("\"! Create it first, then run the server. errno: ")(e)(", ")(strerror(e));
                throw snap::snap_exception(QString("Cannot drop privileges to user \"%1\"! Create it first, then run the server.")
                                    .arg(f_username));
            }
        }
    }
}


/** \brief The execution loop.
 *
 * This function runs the execution loop until the snapcommunicator
 * system receives a QUIT or STOP message.
 */
int server::run()
{
    int r(init());
    if(r != 0)
    {
        return r;
    }

    // run "forever" (until we receive a QUIT message)
    //
    f_communicator->run();

    // we are done, cleanly get rid of the communicator
    //
    f_communicator.reset();

    // we received a RELOADCONFIG, exit with 1 so systemd restarts us
    //
    return f_force_restart ? 1 : 0;
}


/** \brief Make sure that the connection understands a command.
 *
 * This function checks whether the specified connection (\p connection)
 * understands the command about to be sent to it (\p reply).
 *
 * \note
 * The test is done only when snapcommunicator is run in debug
 * mode to not waste time.
 *
 * \param[in,out] connection  The concerned connection that has to understand the command.
 * \param[in] message  The message about to be sent to \p connection.
 */
void server::verify_command(base_connection::pointer_t connection, snap::snap_communicator_message const & message)
{
    // debug turned on?
    if(!f_server->is_debug())
    {
        // nope, do not waste any more time
        return;
    }

    if(!connection->has_commands())
    {
        // if we did not yet receive the COMMANDS message then we cannot
        // pretend that the understand_command() will return a sensible
        // result, so ignore that test...
        //
        return;
    }

    if(connection->understand_command(message.get_command()))
    {
        // all good, the command is implemented
        //
        return;
    }

    // if you get this message, it could be that you do implement
    // the command, but do not advertise it in your COMMANDS
    // reply to the HELP message sent by snapcommunicator
    //
    snap::snap_communicator::snap_connection::pointer_t c(std::dynamic_pointer_cast<snap::snap_communicator::snap_connection>(connection));
    if(c)
    {
        SNAP_LOG_FATAL("connection \"")(c->get_name())("\" does not understand ")(message.get_command())(".");
        throw snap::snap_exception(QString("Connection \"%1\" does not implement command \"%2\".").arg(c->get_name()).arg(message.get_command()));
    }

    SNAP_LOG_FATAL("connection does not understand ")(message.get_command())(".");
    throw snap::snap_exception(QString("Connection does not implement command \"%1\".").arg(message.get_command()));
}


/** \brief Process a message we just received.
 *
 * This function is called whenever a TCP or UDP message is received.
 * The function accepts all TCP messages, however, UDP messages are
 * limited to a very few such as STOP and SHUTDOWN. You will want to
 * check the documentation of each message to know whether it can
 * be sent over UDP or not.
 *
 * Note that the main reason why the UDP port is not allowed for most
 * messages is to send a reply you have to have TCP. This means responses
 * to those messages also need to be sent over TCP (because we could
 * not have sent an ACCEPT as a response to a CONNECT over a UDP
 * connection.)
 *
 * \param[in] connection  The connection that just sent us that message.
 * \param[in] message  The message were were just sent.
 * \param[in] udp  If true, this was a UDP message.
 */
void server::process_message(snap::snap_communicator::snap_connection::pointer_t connection
                                             , snap::snap_communicator_message const & message
                                             , bool udp)
{
    // messages being broadcast to us have a unique ID, if that ID is
    // one we already received we must ignore the message altogether;
    // also, a broadcast message has a timeout, we must ignore the
    // message if it already timed out
    //
    if(message.has_parameter("broadcast_msgid"))
    {
        // check whether the message already timed out
        //
        // this is a safety feature of our broadcasting capability
        // which should rarely be activated unless you have multiple
        // data center locations
        //
        time_t const timeout(message.get_integer_parameter("broadcast_timeout"));
        time_t const now(time(nullptr));
        if(timeout < now)
        {
            return;
        }

        // check whether we already received that message, if so ignore
        // the second instance (it should not happen with the list of
        // neighbors included in the message, but just in case...)
        //
        QString const broadcast_msgid(message.get_parameter("broadcast_msgid"));
        auto const received_it(f_received_broadcast_messages.find(broadcast_msgid));
        if(received_it != f_received_broadcast_messages.cend())     // message arrived again?
        {
            // note that although we include neighbors it is normal that
            // this happens in a cluster where some computers are not
            // aware of certain nodes; for example, if A sends a
            // message to B and C, both B and C know of a node D
            // which is unknown to A, then both B and C end up
            // forwarding that same message to D, so D will discard
            // the second instance it receives.
            //
            return;
        }
    }

    // if the destination server was specified, we have to forward
    // the message to that specific server
    //
    QString const server_name(message.get_server() == "." ? f_server_name : message.get_server());
    QString const service(message.get_service());
    QString const command(message.get_command());
    QString const sent_from_service(message.get_sent_from_service());

    if(f_debug_all_messages
    || (command != "UNLOCKED"
     && sent_from_service != "snaplock"
     && !sent_from_service.startsWith("lock_")
     && (command != "REGISTER"
         || !message.has_parameter("service")
         || !message.get_parameter("service").startsWith("lock_"))
     && command != "SNAPLOG"))
    {
        SNAP_LOG_TRACE("received command=[")(command)
                ("], server_name=[")(server_name)
                ("], service=[")(service)
                ("], message=[")(message.to_message())
                ("]");
    }

    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(connection));
    remote_communicator::pointer_t remote_communicator(std::dynamic_pointer_cast<remote_communicator>(connection));
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));

    // TODO: move all the command bodies to sub-funtions.

    // check whether this message is for us
    //
    if((server_name.isEmpty() || server_name == f_server_name || server_name == "*")    // match server
    && (service.isEmpty() || service == "snapcommunicator"))                            // and service?
    {
        if(f_shutdown)
        {
            // if the user sent us an UNREGISTER we should not generate a
            // QUITTING because the UNREGISTER is in reply to our STOP
            // TBD: we may want to implement the UNREGISTER in this
            //      situation?
            //
            if(!udp)
            {
                if(command != "UNREGISTER")
                {
                    // we are shutting down so just send a quick QUTTING reply
                    // letting the other process know about it
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("QUITTING");

                    verify_command(base, reply);
                    if(remote_communicator != nullptr)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }
                }

                // get rid of that connection now, we don't need any more
                // messages coming from it
                //
                f_communicator->remove_connection(connection);
            }
            //else -- UDP message arriving after f_shutdown are ignored
            return;
        }

        // this one is for us!
        switch(command[0].unicode())
        {
        case 'A':
            if(command == "ACCEPT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("ACCEPT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    // the type is mandatory in an ACCEPT message
                    //
                    if(!message.has_parameter("server_name")
                    || !message.has_parameter("my_address"))
                    {
                        SNAP_LOG_ERROR("ACCEPT was received without a \"server_name\" parameter, which is mandatory.");
                        return;
                    }
                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_REMOTE);
                    QString const & remote_server_name(message.get_parameter("server_name"));
                    base->set_server_name(remote_server_name);

                    // reply to a CONNECT, this was to connect to another
                    // snapcommunicator on another computer, retrieve the
                    // data from that remote computer
                    //
                    base->connection_started();
                    QString const his_address(message.get_parameter("my_address"));
                    base->set_my_address(his_address);

                    if(message.has_parameter("services"))
                    {
                        base->set_services(message.get_parameter("services"));
                    }
                    if(message.has_parameter("heard_of"))
                    {
                        base->set_services_heard_of(message.get_parameter("heard_of"));
                    }
                    if(message.has_parameter("neighbors"))
                    {
                        add_neighbors(message.get_parameter("neighbors"));
                    }

                    // we just got some new services information,
                    // refresh our cache
                    //
                    refresh_heard_of();

                    // also request the COMMANDS of this connection
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator != nullptr)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception(QString("message \"%1\" sent on a \"weird\" connection.").arg(command));
                    }

                    // if a local service was interested in this specific
                    // computer, then we have to start receiving LOADAVG
                    // messages from it
                    //
                    register_for_loadavg(his_address);

                    // now let local services know that we have a new
                    // remote connections (which may be of interest
                    // for that service--see snapmanagerdaemon)
                    //
                    // TODO: to be symmetrical, we should also have
                    //       a message telling us when a remote
                    //       connection goes down...
                    //
                    snap::snap_communicator_message new_remote_connection;
                    new_remote_connection.set_command("NEWREMOTECONNECTION");
                    new_remote_connection.set_service(".");
                    new_remote_connection.add_parameter("server_name", remote_server_name);
                    broadcast_message(new_remote_connection);
                    return;
                }
            }
            break;

        case 'C':
            if(command == "CLUSTERSTATUS")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("CLUSTERSTATUS is only accepted over a TCP connection.");
                    return;
                }

                if(base != nullptr)
                {
                    cluster_status(connection);
                    return;
                }
            }
            else if(command == "COMMANDS")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("COMMANDS is only accepted over a TCP connection.");
                    return;
                }

                if(base != nullptr)
                {
                    if(message.has_parameter("list"))
                    {
                        base->set_commands(message.get_parameter("list"));

                        // here we verify that a few commands are properly
                        // defined, for some becausesince we already sent
                        // them to that connection and thus it should
                        // understand them; and a few more that are very
                        // possibly going to be sent
                        //
                        if(f_server->is_debug())
                        {
                            bool ok(true);
                            if(!base->understand_command("HELP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand HELP.");
                                ok = false;
                            }
                            if(!base->understand_command("QUITTING"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand QUITTING.");
                                ok = false;
                            }
                            // on a remote we get ACCEPT instead of READY
                            if(remote_communicator != nullptr
                            || base->is_remote())
                            {
                                if(!base->understand_command("ACCEPT"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand ACCEPT.");
                                    ok = false;
                                }
                            }
                            else
                            {
                                if(!base->understand_command("READY"))
                                {
                                    SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand READY.");
                                    ok = false;
                                }
                            }
                            if(!base->understand_command("STOP"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand STOP.");
                                ok = false;
                            }
                            if(!base->understand_command("UNKNOWN"))
                            {
                                SNAP_LOG_FATAL("connection \"")(connection->get_name())("\" does not understand UNKNOWN.");
                                ok = false;
                            }
                            if(!ok)
                            {
                                // end the process so developers can fix their
                                // problems (this is only if --debug was
                                // specified)
                                //
                                throw snap::snap_exception(QString("Connection %1 does not implement some required commands.").arg(connection->get_name()));
                            }
                        }
                    }
                    else
                    {
                        SNAP_LOG_ERROR("COMMANDS was sent without a \"list\" parameter.");
                    }
                    return;
                }
            }
            else if(command == "CONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("CONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    // first we verify that we have a valid version to
                    // communicate between two snapcommunicators
                    //
                    if(!message.has_parameter("version")
                    || !message.has_parameter("my_address")
                    || !message.has_parameter("server_name"))
                    {
                        SNAP_LOG_ERROR("CONNECT was sent without a \"version\", or \"my_address\" parameter, both are mandatory.");
                        return;
                    }
                    if(message.get_integer_parameter("version") != snap::snap_communicator::VERSION)
                    {
                        SNAP_LOG_ERROR("CONNECT was sent with an incompatible version. Expected ")(snap::snap_communicator::VERSION)(", received ")(message.get_integer_parameter("version"));
                        return;
                    }

                    snap::snap_communicator_message reply;
                    snap::snap_communicator_message new_remote_connection;

                    QString const remote_server_name(message.get_parameter("server_name"));
                    snap::snap_communicator::snap_connection::vector_t const all_connections(f_communicator->get_connections());
                    snap::snap_communicator::snap_connection::pointer_t snap_conn(std::dynamic_pointer_cast<snap::snap_communicator::snap_connection>(base));
                    auto const & name_match(std::find_if(
                            all_connections.begin(),
                            all_connections.end(),
                            [remote_server_name, snap_conn](auto const & it)
                            {
                                // ignore ourselves
                                //
                                if(it == snap_conn)
                                {
                                    return false;
                                }
                                base_connection::pointer_t b(std::dynamic_pointer_cast<base_connection>(it));
                                if(!b)
                                {
                                    return false;
                                }
                                return remote_server_name == b->get_server_name();
                            }));

                    bool refuse(name_match != all_connections.end());
                    if(refuse)
                    {
                        SNAP_LOG_ERROR("CONNECT from \"")(remote_server_name)("\" but we already have another computer using that same name.");

                        reply.set_command("REFUSE");
                        reply.add_parameter("conflict", "name");

                        // we may also be shutting down
                        //
                        // Note: we cannot get here if f_shutdown is true...
                        //
                        if(f_shutdown)
                        {
                            reply.add_parameter("shutdown", "true");
                        }
                    }
                    else
                    {
                        base->set_server_name(remote_server_name);

                        // add neighbors with which the guys asking to
                        // connect can attempt to connect with...
                        //
                        if(!f_explicit_neighbors.isEmpty())
                        {
                            reply.add_parameter("neighbors", f_explicit_neighbors);
                        }

                        // Note: we cannot get here if f_shutdown is true...
                        //
                        refuse = f_shutdown;
                        if(refuse)
                        {
                            // okay, this guy wants to connect to us but we
                            // are shutting down, so refuse and put the shutdown
                            // flag to true
                            //
                            reply.set_command("REFUSE");
                            reply.add_parameter("shutdown", "true");
                        }
                        else
                        {
                            // cool, a remote snapcommunicator wants to connect
                            // with us, make sure we did not reach the maximum
                            // number of connections though...
                            //
                            refuse = f_communicator->get_connections().size() >= f_max_connections;
                            if(refuse)
                            {
                                // too many connections already, refuse this new
                                // one from a remove system
                                //
                                reply.set_command("REFUSE");
                            }
                            else
                            {
                                // set the connection type if we are not refusing it
                                //
                                base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_REMOTE);

                                // same as ACCEPT (see above) -- maybe we could have
                                // a sub-function...
                                //
                                base->connection_started();

                                if(message.has_parameter("services"))
                                {
                                    base->set_services(message.get_parameter("services"));
                                }
                                if(message.has_parameter("heard_of"))
                                {
                                    base->set_services_heard_of(message.get_parameter("heard_of"));
                                }
                                if(message.has_parameter("neighbors"))
                                {
                                    add_neighbors(message.get_parameter("neighbors"));
                                }

                                // we just got some new services information,
                                // refresh our cache
                                //
                                refresh_heard_of();

                                // the message expects the ACCEPT reply
                                //
                                reply.set_command("ACCEPT");
                                reply.add_parameter("server_name", f_server_name);
                                reply.add_parameter("my_address", QString::fromUtf8(f_my_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str()));

                                // services
                                //
                                if(!f_local_services.isEmpty())
                                {
                                    reply.add_parameter("services", f_local_services);
                                }

                                // heard of
                                //
                                if(!f_services_heard_of.isEmpty())
                                {
                                    reply.add_parameter("heard_of", f_services_heard_of);
                                }

                                QString const his_address(message.get_parameter("my_address"));
                                base->set_my_address(his_address);

                                // if a local service was interested in this specific
                                // computer, then we have to start receiving LOADAVG
                                // messages from it
                                //
                                register_for_loadavg(his_address);

                                // he is a neighbor too, make sure to add it
                                // in our list of neighbors (useful on a restart
                                // to connect quickly)
                                //
                                add_neighbors(his_address);

                                // since we are accepting a CONNECT we have to make
                                // sure we cancel the GOSSIP events to that remote
                                // connection; it won't hurt, but it is a waste if
                                // we do not need it
                                //
                                // Note: the name of the function is "GOSSIP"
                                //       received because if the "RECEIVED"
                                //       message was sent back from that remote
                                //       snapcommunicator then it means that
                                //       remote daemon received our GOSSIP message
                                //       and receiving the "CONNECT" message is
                                //       very similar to receiving the "RECEIVED"
                                //       message after a "GOSSIP"
                                //
                                f_remote_snapcommunicators->gossip_received(his_address);

                                // now let local services know that we have a new
                                // remote connections (which may be of interest
                                // for that service--see snapmanagerdaemon)
                                //
                                // TODO: to be symmetrical, we should also have
                                //       a message telling us when a remote
                                //       connection goes down...
                                //
                                new_remote_connection.set_command("NEWREMOTECONNECTION");
                                new_remote_connection.set_service(".");
                                new_remote_connection.add_parameter("server_name", remote_server_name);
                            }
                        }
                    }

                    //verify_command(base, reply); -- we do not yet have a list of commands understood by the other snapcommunicator daemon

                    // also request the COMMANDS of this connection with a HELP
                    // if the connection was not refused
                    //
                    snap::snap_communicator_message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- precisely
                    if(remote_communicator != nullptr)
                    {
                        remote_communicator->send_message(reply);
                        if(!refuse)
                        {
                            remote_communicator->send_message(help);
                            broadcast_message(new_remote_connection);
                        }
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                        if(!refuse)
                        {
                            service_conn->send_message(help);
                            broadcast_message(new_remote_connection);
                        }
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("CONNECT sent on a \"weird\" connection.");
                    }

                    // if not refused, then we may have a QUORUM now, check
                    // that; the function we call takes care of knowing
                    // whether we reach cluster status or not
                    //
                    if(!refuse)
                    {
                        cluster_status(nullptr);
                    }

                    // status changed for this connection
                    //
                    send_status(connection);
                    return;
                }
            }
            break;

        case 'D':
            if(command == "DISCONNECT")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("DISCONNECT is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    base->connection_ended();

                    // this has to be another snapcommunicator
                    // (i.e. an object that sent ACCEPT or CONNECT)
                    //
                    base_connection::connection_type_t const type(base->get_connection_type());
                    if(type == base_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                    {
                        // we must ignore and we do ignore connections with a
                        // type of "" since they represent an uninitialized
                        // connection item (unconnected)
                        //
                        base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_DOWN);

                        if(remote_communicator == nullptr)
                        {
                            // disconnecting means it is gone so we can remove
                            // it from the communicator since the other end
                            // will reconnect (we are never responsible
                            // for that in this case)
                            //
                            // Note: this one happens when the computer that
                            //       sent us a CONNECT later sends us the
                            //       DISCONNECT
                            //
                            f_communicator->remove_connection(connection);
                        }
                        else
                        {
                            // in this case we are in charge of attempting
                            // to reconnect until it works... however, it
                            // is likely that the other side just shutdown
                            // so we want to "induce a long enough pause"
                            // to avoid attempting to reconnect like crazy
                            //
                            remote_communicator->disconnect();
                            addr::addr const remote_addr(remote_communicator->get_address());
                            f_remote_snapcommunicators->shutting_down(remote_addr.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str());
                        }

                        // we just got some new services information,
                        // refresh our cache
                        //
                        refresh_heard_of();

                        if(!base->get_server_name().isEmpty())
                        {
                            snap::snap_communicator_message disconnected;
                            disconnected.set_command("DISCONNECTED");
                            disconnected.set_service(".");
                            disconnected.add_parameter("server_name", base->get_server_name());
                            broadcast_message(disconnected);
                        }

                        cluster_status(nullptr);
                    }
                    else
                    {
                        SNAP_LOG_ERROR("DISCONNECT was sent from a connection which is not of the right type (")
                                      (type == base_connection::connection_type_t::CONNECTION_TYPE_DOWN ? "down" : "client")
                                      (").");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);
                    return;
                }
            }
            break;

        case 'F':
            if(command == "FORGET")
            {
                // whenever computers connect between each others, their
                // IP address gets added to our list of neighbors; this
                // means that the IP address is now stuck in the
                // computer's brain "forever"
                //
                QString const forget_ip(message.get_parameter("ip"));

                // self is not a connection that get broadcast messages
                // for snapcommunicator, so we also call the remove_neighbor()
                // function now
                //
                remove_neighbor(forget_ip);

                // once you notice many connection errors to other computers
                // that have been removed from your cluster, you want the
                // remaining computers to forget about that IP address and
                // it is done by broadcasting a FORGET message to everyone
                //
                if(!message.has_parameter("broadcast_hops"))
                {
                    // this was sent directly to this instance only,
                    // make sure to broadcast the message instead
                    //
                    snap::snap_communicator_message forget;
                    forget.set_command("FORGET");
                    forget.set_server("*");
                    forget.set_service("snapcommunicator");
                    forget.add_parameter("ip", forget_ip);
                    broadcast_message(forget);
                }
                return;
            }
            break;

        case 'G':
            if(command == "GOSSIP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("GOSSIP is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    // we got a GOSSIP message, this one will have addresses
                    // with various neighbors; we have two modes:
                    //
                    // 1) my_address=... is defined -- in this case the
                    //    remote host sent us his address because he was
                    //    not sure whether we knew about him; add that
                    //    address as a neighbor and go on as normal
                    //
                    // 2) heard_of=... is defined -- in this case, the
                    //    remote host received a GOSSIP from any one
                    //    snapcommunicator and it is propagating the
                    //    message; check all the IPs in that list and
                    //    if all are present in our list of neighbors,
                    //    do nothing; if all are not present, proceed
                    //    as normal in regard to attempt connections
                    //    and also forward our own GOSSIP to others
                    //    since we just heard of some new neighbors!
                    //
                    //    Note that at this point we use the Flooding
                    //    scheme and we implemented the Eventual
                    //    Consistency (because at some point in time
                    //    we eventually have an exact result.)
                    //
                    // When using (2) we are using what is called
                    // Gossiping in Computer Science. At thist time
                    // we use what is called the Flooding Algorithm.
                    //
                    // https://en.wikipedia.org/wiki/Flooding_(computer_networking)
                    //
                    // See also doc/focs2003-gossip.pdf
                    //
                    // We add two important features: (a) the list of
                    // nodes we already sent the message to, in
                    // order to avoid sending it to the same node
                    // over and over again; and (b) a serial number
                    // to be able to identify the message.
                    //
                    // Two other features that could be added are:
                    // (c) counting hops, after X hops were reached,
                    // stop forwarding the message because we should
                    // already have reached all nodes; (d) a specific
                    // date when the message times out.
                    //
                    // The serial number is used to know whether we
                    // already received a certain message. These can
                    // expire after a while (we may actually want to
                    // implement (d) from the get go so we know
                    // exactly when such expires).
                    //
                    // Our GOSSIP has one advantage, it is used to
                    // connect all the snapcommunicators together
                    // once. After that, the GOSSIP messages stop,
                    // no matter what (i.e. if a new snapcommunicator
                    // daemon is started, then the GOSSIP restart
                    // for that instance, but that's it.)
                    //
                    // However, we also offer a way to broadcast
                    // messages and these happen all the time
                    // (i.e. think of the snaplock broadcast messages).
                    // In those cases, we do not need to use the same
                    // algorithm because at that point we are expected
                    // to have a complete list of all the
                    // snapcommunicators available.
                    //
                    // (TODO: only we may not be connected to all of them,
                    // so we need to keep track of the snapcommunicators
                    // we are not connected to and ask others to do some
                    // forwarding!)
                    //
                    if(message.has_parameter("my_address"))
                    {
                        // this is a "simple" GOSSIP of a snapcommunicator
                        // telling us it exists and expects a connection
                        // from us
                        //
                        // in this case we just reply with RECEIVED to
                        // confirm that we get the GOSSIP message
                        //
                        std::string const reply_to(message.get_parameter("my_address"));
                        add_neighbors(reply_to);
                        f_remote_snapcommunicators->add_remote_communicator(reply_to);

                        snap::snap_communicator_message reply;
                        reply.set_command("RECEIVED");
                        //verify_command(base, reply); -- in this case the remote snapcommunicator is not connected, so no HELP+COMMANDS and thus no verification possible
                        if(remote_communicator)
                        {
                            remote_communicator->send_message(reply);
                        }
                        else if(service_conn)
                        {
                            // Should this be an error instead since we only
                            // expect this message from remote snapcommunicators?
                            service_conn->send_message(reply);
                        }
                        else
                        {
                            // we have to have a remote or service connection here
                            //
                            throw snap::snap_exception("GOSSIP sent on a \"weird\" connection.");
                        }
                        return;
                    }
SNAP_LOG_ERROR("GOSSIP is not yet fully implemented.");
                    return;
                }
            }
            break;

        case 'H':
            if(command == "HELP")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("HELP is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    // reply with COMMANDS
                    //
                    snap::snap_communicator_message reply;
                    reply.set_command("COMMANDS");

                    // list of commands understood by snapcommunicator
                    reply.add_parameter("list", "ACCEPT,CLUSTERSTATUS,COMMANDS,CONNECT,DISCONNECT,FORGET,GOSSIP,HELP,LISTENLOADAVG,LOADAVG,LOG,PUBLIC_IP,QUITTING,REFUSE,REGISTER,REGISTERFORLOADAVG,RELOADCONFIG,SERVICES,SHUTDOWN,STOP,UNKNOWN,UNREGISTER,UNREGISTERFORLOADAVG");

                    //verify_command(base, reply); -- this verification does not work with remote snap communicator connections
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("HELP sent on a \"weird\" connection.");
                    }
                    return;
                }
            }
            break;

        case 'L':
            if(command == "LOADAVG")
            {
                save_loadavg(message);
                return;
            }
            else if(command == "LISTENLOADAVG")
            {
                listen_loadavg(message);
                return;
            }
            else if(command == "LOG")
            {
                SNAP_LOG_INFO("Logging reconfiguration.");
                snap::logging::reconfigure();
                return;
            }
            else if(command == "LISTSERVICES")
            {
                snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
                std::string list;
                std::for_each(
                        all_connections.begin(),
                        all_connections.end(),
                        [&list](auto const & c)
                        {
                            if(!list.empty())
                            {
                                list += ", ";
                            }
                            list += c->get_name();
                        });
                SNAP_LOG_INFO("current list of connections: ")(list);
                return;
            }
            break;

        case 'P':
            if(command == "PUBLIC_IP")
            {
                if(service_conn)
                {
                    snap::snap_communicator_message reply;
                    reply.set_command("SERVER_PUBLIC_IP");
                    reply.add_parameter("public_ip", QString::fromUtf8(f_public_ip.c_str()));
                    verify_command(base, reply);
                    service_conn->send_message(reply);
                    return;
                }
                else
                {
                    // we have to have a remote or service connection here
                    //
                    throw snap::snap_exception("PUBLIC_IP sent on a \"weird\" connection.");
                }
            }
            break;

        case 'Q':
            if(command == "QUITTING")
            {
                // if this becomes problematic, we may need to serialize
                // our messages to know which was ignored...
                //
                SNAP_LOG_INFO("Received a QUITTING as a reply to a message.");
                return;
            }
            break;

        case 'R':
            if(command == "REFUSE")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REFUSE is only accepted over a TCP connection.");
                    break;
                }

                // we were not connected so we do not have to
                // disconnect; mark that corresponding server
                // as too busy and try connecting again much
                // later...
                //
                addr::addr peer_addr;
                if(remote_communicator)
                {
                    peer_addr = remote_communicator->get_address();
                }
                //else if(service_conn) -- this should not happen
                //{
                //    peer_addr = service_conn->get_remote/peer_addr();
                //}
                else
                {
                    // we have to have a remote or service connection here
                    //
                    throw snap::snap_exception("REFUSE sent on a \"weird\" connection.");
                }
                //std::string const addr(peer_addr.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT));
                if(message.has_parameter("shutdown"))
                {
                    f_remote_snapcommunicators->shutting_down(peer_addr);
                }
                else
                {
                    f_remote_snapcommunicators->too_busy(peer_addr);
                }

                // we are responsible to try again later, so we do not
                // lose the connection, but we need to disconnect
                //
                //f_communicator->remove_connection(connection);
                remote_communicator->disconnect();
                return;
            }
            else if(command == "REGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR
                        << "REGISTER is only accepted over a TCP or Unix connection."
                        << SNAP_LOG_SEND;
                    break;
                }

                if(base)
                {
                    if(!message.has_parameter("service")
                    || !message.has_parameter("version"))
                    {
                        SNAP_LOG_ERROR
                            << "REGISTER was called without a \"service\" and/or a \"version\" parameter, both are mandatory."
                            << SNAP_LOG_SEND;
                        return;
                    }
                    if(!message.check_version_parameter())
                    {
                        SNAP_LOG_ERROR
                            << "REGISTER was called with an incompatible version; expected "
                            << ed::MESSAGE_VERSION
                            << ", received "
                            << message.get_message_version()
                            << "."
                            << SNAP_LOG_SEND;
                        return;
                    }
                    // the "service" parameter is the name of the service,
                    // now we can process messages for this service
                    //
                    std::string const service_name(message.get_parameter("service"));
                    connection->set_name(service_name);
                    if(service_conn)
                    {
                        service_conn->properly_named();
                    }

                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_LOCAL);

                    // connection is up now
                    //
                    base->connection_started();

                    // request the COMMANDS of this connection
                    //
                    ed::message help;
                    help.set_command("HELP");
                    //verify_command(base, help); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(help);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(help);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw unexpected_message_parameter("REGISTER sent on a \"weird\" connection (1).");
                    }

                    // tell the connection we are ready
                    // (many connections use that as a trigger to start work)
                    //
                    ed::message reply;
                    reply.set_command("READY");
                    //verify_command(base, reply); -- we cannot do that here since we did not yet get the COMMANDS reply
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw unexpected_message_parameter("REGISTER sent on a \"weird\" connection (2).");
                    }

                    // status changed for this connection
                    //
                    send_status(connection);

                    // if we have local messages that were cached, then
                    // forward them now
                    //
                    f_local_message_cache.process_messages(
                        [service_name, remote_communicator, service_conn](ed::message const & msg)
                        {
                            if(msg.get_service() != service_name)
                            {
                                return false;
                            }

                            if(remote_communicator)
                            {
                                remote_communicator->send_message(msg);
                            }
                            else if(service_conn)
                            {
                                service_conn->send_message(msg);
                            }
                            else
                            {
                                // we have to have a remote or service connection here
                                //
                                throw unexpected_message_parameter("REGISTER sent on a \"weird\" connection (3).");
                            }

                            return true;
                        });
                    return;
                }
            }
            else if(command == "REGISTERFORLOADAVG")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("REGISTERFORLOADAVG is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    base->set_wants_loadavg(true);
                    f_loadavg_timer->set_enable(true);
                    return;
                }
            }
            else if(command == "RELOADCONFIG")
            {
                // we need a full restart in this case (because when we
                // restart snapcommunicator it also automatically restart
                // all of its dependencies!)
                //
                // also if you are a programmer we cannot do a systemctl
                // restart so we just skip the feature...
                //
                f_force_restart = true;
                shutdown(false);
                return;
            }
            break;

        case 'S':
            if(command == "SHUTDOWN")
            {
                shutdown(true);
                return;
            }
            else if(command == "STOP")
            {
                shutdown(false);
                return;
            }
            else if(command == "SERVICESTATUS")
            {
                QString const & service_name(message.get_parameter("service"));
                if(service_name.isEmpty())
                {
                    SNAP_LOG_ERROR("The SERVICESTATUS service parameter cannot be an empty string.");
                    return;
                }
                snap::snap_communicator::snap_connection::vector_t const & named_connections(f_communicator->get_connections());
                auto const named_service(std::find_if(
                              named_connections.begin()
                            , named_connections.end()
                            , [service_name](auto const & named_connection)
                            {
                                return named_connection->get_name() == service_name;
                            }));
                if(named_service == named_connections.end())
                {
                    // service is totally unknown
                    //
                    // create a fake connection so we can call the
                    // send_status() function
                    //
                    snap::snap_communicator::snap_connection::pointer_t fake_connection(std::make_shared<snap::snap_communicator::snap_timer>(0));
                    fake_connection->set_name(service_name);
                    send_status(fake_connection, &connection);
                }
                else
                {
                    send_status(*named_service, &connection);
                }
                return;
            }
            break;

        case 'U':
            if(command == "UNKNOWN")
            {
                SNAP_LOG_ERROR("we sent command \"")
                              (message.get_parameter("command"))
                              ("\" to \"")
                              (connection->get_name())
                              ("\" which told us it does not know that command so we probably did not get the expected result.");
                return;
            }
            else if(command == "UNREGISTER")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTER is only accepted over a TCP connection.");
                    return;
                }

                if(base)
                {
                    if(!message.has_parameter("service"))
                    {
                        SNAP_LOG_ERROR("UNREGISTER was called without a \"service\" parameter, which is mandatory.");
                        return;
                    }
                    // also remove all the connection types
                    // an empty string represents an unconnected item
                    //
                    base->set_connection_type(base_connection::connection_type_t::CONNECTION_TYPE_DOWN);

                    // connection is down now
                    //
                    base->connection_ended();

                    // status changed for this connection
                    //
                    send_status(connection);

                    // now remove the service name
                    // (send_status() needs the name to still be in place!)
                    //
                    QString const save_name(connection->get_name());
                    connection->set_name("");

                    // get rid of that connection now (it is faster than
                    // waiting for the HUP because it will not be in the
                    // list of connections on the next loop.)
                    //
                    f_communicator->remove_connection(connection);

                    return;
                }
            }
            else if(command == "UNREGISTERFORLOADAVG")
            {
                if(udp)
                {
                    SNAP_LOG_ERROR("UNREGISTERFORLOADAVG is only accepted over a TCP connection.");
                    break;
                }

                if(base)
                {
                    base->set_wants_loadavg(false);
                    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
                    if(all_connections.end() == std::find_if(
                            all_connections.begin(),
                            all_connections.end(),
                            [](auto const & c)
                            {
                                base_connection::pointer_t b(std::dynamic_pointer_cast<base_connection>(c));
                                return b->wants_loadavg();
                            }))
                    {
                        // no more connection requiring LOADAVG messages
                        // so stop the timer
                        //
                        f_loadavg_timer->set_enable(false);
                    }
                    return;
                }
            }
            break;

        }

        // if they used a TCP connection to send this message, let the caller
        // know that we do not understand his message
        //
        if(!udp)
        {
            snap::snap_communicator_message reply;
            reply.set_command("UNKNOWN");
            reply.add_parameter("command", command);
            verify_command(base, reply);
            if(remote_communicator)
            {
                remote_communicator->send_message(reply);
            }
            else if(service_conn)
            {
                service_conn->send_message(reply);
            }
            else
            {
                // we have to have a remote or service connection here
                //
                throw snap::snap_exception("HELP sent on a \"weird\" connection.");
            }
        }

        // done
        SNAP_LOG_ERROR("unknown command \"")(command)("\" or not sent from what is considered the correct connection for that message.");
        return;
    }

    //
    // the message includes a service name, so we want to forward that
    // message to that service
    //
    // for that purpose we consider the following three lists:
    //
    // 1. we have the service in our local services, we must forward it
    //    to that connection; if the connection is not up and running yet,
    //    cache the information
    //
    // 2. the service is not one of ours, but we found a remote
    //    snapcommunicator server that says it is his, forward the
    //    message to that snapcommunicator instead
    //
    // 3. the service is in the "heard of" list of services, send that
    //    message to that snapcommunicator, it will then forward it
    //    to the correct server (or another proxy...)
    //
    // 4. the service cannot be found anywhere, we save it in our remote
    //    cache (i.e. because it will only be possible to send that message
    //    to a remote snapcommunicator and not to a service on this system)
    //

//SNAP_LOG_TRACE("---------------- got message for [")(server_name)("] / [")(service)("]");

    // broadcasting?
    //
    if(service == "*"
    || service == "?"
    || service == ".")
    {
        if(!server_name.isEmpty()
        && server_name != "*"
        && (service == "*" || service == "?"))
        {
            // do not send the message in this case!
            //
            // we cannot at the same time send it to this local server
            // and broadcast it to other servers... it is contradictory;
            // either set the server to "*" or empty, or do not broadcast
            //
            SNAP_LOG_ERROR("you cannot at the same time specify a server name (")(server_name)(") and \"*\" or \"?\" as the service.");
            return;
        }
        broadcast_message(message);
        return;
    }

    base_connection_vector_t accepting_remote_connections;
    bool const all_servers(server_name.isEmpty() || server_name == "*");
    {
        // service is local, check whether the service is registered,
        // if registered, forward the message immediately
        //
        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto const & nc : connections)
        {
            base_connection::pointer_t base_conn(std::dynamic_pointer_cast<base_connection>(nc));
            if(base_conn)
            {
                // verify that there is a server name in all connections
                // (if not we have a bug somewhere else)
                //
                if(base_conn->get_server_name().isEmpty())
                {
                    if(!f_server->is_debug())
                    {
                        // ignore in non-debug versions because a throw
                        // completely breaks snapcommunicator... and it
                        // is not that important at this point without
                        // a programmer debugging this software
                        //
                        continue;
                    }
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn)
                    {
                        throw std::runtime_error("server name missing in connection " + conn->get_name().toStdString() + "...");
                    }
                    switch(base_conn->get_connection_type())
                    {
                    case base_connection::connection_type_t::CONNECTION_TYPE_DOWN:
                        // not connected yet, forget about it
                        continue;

                    case base_connection::connection_type_t::CONNECTION_TYPE_LOCAL:
                        throw std::runtime_error("server name missing in connection \"local service\"...");

                    case base_connection::connection_type_t::CONNECTION_TYPE_REMOTE:
                        throw std::runtime_error("server name missing in connection \"remote snapcommunicator\"...");

                    }
                }

                if(all_servers
                || server_name == base_conn->get_server_name())
                {
                    service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                    if(conn)
                    {
                        if(conn->get_name() == service)
                        {
                            // we have such a service, just forward to it now
                            //
                            // TBD: should we remove the service name before forwarding?
                            //
                            try
                            {
                                verify_command(conn, message);
                                conn->send_message(message);
                            }
                            catch(std::runtime_error const & e)
                            {
                                // ignore the error because this can come from an
                                // external source (i.e. snapsignal) where an end
                                // user may try to break the whole system!
                                //
                                SNAP_LOG_DEBUG("snapcommunicator failed to send a message to connection \"")
                                              (conn->get_name())
                                              ("\" (error: ")
                                              (e.what())
                                              (")");
                            }
                            // we found a specific service to which we could
                            // forward the message so we can stop here
                            //
                            return;
                        }
                        else
                        {
                            // if not a local connection with the proper name,
                            // still send it to that connection but only if it
                            // is a remote connection
                            //
                            base_connection::connection_type_t const type(base_conn->get_connection_type());
                            if(type == base_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                            {
                                accepting_remote_connections.push_back(conn);
                            }
                        }
                    }
                    else
                    {
                        remote_snap_communicator_pointer_t remote_connection(std::dynamic_pointer_cast<remote_snapcommunicator>(nc));
                        // TODO: limit sending to remote only if they have that service?
                        //       (if we have the 'all_servers' set, otherwise it is not
                        //       required, for sure... also, if we have multiple remote
                        //       connections that support the same service we should
                        //       randomize which one is to receive that message--or
                        //       even better, check the current server load--but
                        //       seriously, if none of our direct connections know
                        //       of that service, we need to check for those that heard
                        //       of that service, and if that is also empty, send to
                        //       all... for now we send to all anyway)
                        //
                        if(remote_connection /*&& remote_connection->has_service(service)*/)
                        {
                            accepting_remote_connections.push_back(remote_connection);
                        }
                    }
                }
            }
        }

        auto transmission_report = [&message, &base, &remote_communicator, &service_conn]()
        {
            if(message.has_parameter("transmission_report"))
            {
                QString const report(message.get_parameter("transmission_report"));
                if(report == "failure")
                {
                    snap::snap_communicator_message reply;
                    reply.set_command("TRANSMISSIONREPORT");
                    reply.add_parameter("status", "failed");
                    //verify_command(base, reply);
                    if(remote_communicator)
                    {
                        remote_communicator->send_message(reply);
                    }
                    else if(service_conn)
                    {
                        service_conn->send_message(reply);
                    }
                    else
                    {
                        // we have to have a remote or service connection here
                        //
                        throw snap::snap_exception("No valid connection to send a reply.");
                    }
                }
            }
        };

        if((all_servers || server_name == f_server_name)
        && f_local_services_list.contains(service))
        {
            // its a service that is expected on this computer, but it is not
            // running right now... so cache the message
            //
            cache_message(message);
            transmission_report();
            return;
        }

        // if attempting to send to self, we cannot go on from here
        //
        if(server_name == f_server_name)
        {
            if(!service.startsWith("lock_"))
            {
                SNAP_LOG_DEBUG("received event \"")(command)("\" for local service \"")(service)("\", which is not currently registered. Dropping message.");
            }
            transmission_report();
            return;
        }
    }

    if(!accepting_remote_connections.empty())
    {
        broadcast_message(message, accepting_remote_connections);
    }
}


void server::broadcast_message(snap::snap_communicator_message const & message, base_connection_vector_t const & accepting_remote_connections)
{
    QString broadcast_msgid;
    QString informed_neighbors;
    int hops(0);
    time_t timeout(0);

    // note: the "broacast_msgid" is required when we end up sending that
    //       message forward to some other computers; so we have to go
    //       through that if() block; however, the timeout was already
    //       already checked, so we probably would not need it to do
    //       it again?
    //
    if(message.has_parameter("broadcast_msgid"))
    {
        // check whether the message already timed out
        //
        // this is a safety feature of our broadcasting capability
        // which should rarely be activated unless you have multiple
        // data center locations
        //
        timeout = message.get_integer_parameter("broadcast_timeout");
        time_t const now(time(nullptr));
        if(timeout < now)
        {
            return;
        }

        // check whether we already received that message, if so ignore
        // the second instance (it should not happen with the list of
        // neighbors included in the message, but just in case...)
        //
        broadcast_msgid = message.get_parameter("broadcast_msgid");
        auto const received_it(f_received_broadcast_messages.find(broadcast_msgid));
        if(received_it != f_received_broadcast_messages.cend())     // message arrived again?
        {
            // note that although we include neighbors it is normal that
            // this happens in a cluster where some computers are not
            // aware of certain nodes; for example, if A sends a
            // message to B and C, both B and C know of a node D
            // which is unknown to A, then both B and C will end
            // up forward that same message to D, so D will discard
            // the second instance it receives.
            //
            return;
        }

        // delete "received messages" that have now timed out (because
        // such are not going to be forwarded since we check the timeout
        // of a message early and prevent the broadcasting in that case)
        //
        // XXX: I am thinking that this loop should probably be run before
        //      the "broadcast_timeout" test above...
        //
        for(auto it(f_received_broadcast_messages.cbegin()); it != f_received_broadcast_messages.cend(); )
        {
            if(it->second < now)
            {
                // this one timed out, we do not need to keep it in this
                // list, so erase
                //
                it = f_received_broadcast_messages.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // add the new message after we check for timed out entries
        // so that way we avoid going through this new entry within
        // the previous loop
        //
        f_received_broadcast_messages[broadcast_msgid] = timeout;

        // Note: we skip the canonicalization on this list of neighbors
        //       because we assume only us (snapcommunicator) handles
        //       that message and we know that it is already
        //       canonicalized here
        //
        informed_neighbors = message.get_parameter("broadcast_informed_neighbors");

        // get the number of hops this message already performed
        //
        hops = message.get_integer_parameter("broadcast_hops");
    }

    snap::snap_string_list informed_neighbors_list;
    if(!informed_neighbors.isEmpty())
    {
        informed_neighbors_list = snap::split_string(informed_neighbors, ',');
    }

    // we always broadcast to all local services
    snap::snap_communicator::snap_connection::vector_t broadcast_connection;

    if(accepting_remote_connections.empty())
    {
        QString destination("?");
        QString const service(message.get_service());
        if(service != "."
        && service != "?"
        && service != "*")
        {
            destination = message.get_server();
            if(destination.isEmpty())
            {
                destination = "?";
            }
        }
        else
        {
            destination = service;
        }
        bool const all(hops < 5 && destination == "*");
        bool const remote(hops < 5 && (all || destination == "?"));

        snap::snap_communicator::snap_connection::vector_t const & connections(f_communicator->get_connections());
        for(auto const & nc : connections)
        {
            // try for a service or snapcommunicator that connected to us
            //
            service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
            remote_snap_communicator_pointer_t remote_communicator;
            if(!conn)
            {
                remote_communicator = std::dynamic_pointer_cast<remote_snapcommunicator>(nc);
            }
            bool broadcast(false);
            if(conn)
            {
                switch(conn->get_address().get_network_type())
                {
                case addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                    // these are localhost services, avoid sending the
                    // message is the destination does not know the
                    // command
                    //
                    if(conn->understand_command(message.get_command())) // destination: "*" or "?" or "."
                    {
                        //verify_command(conn, message); -- we reach this line only if the command is understood, it is therefore good
                        conn->send_message(message);
                    }
                    break;

                case addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                    // these are computers within the same local network (LAN)
                    // we forward messages if at least 'remote' is true
                    //
                    broadcast = remote; // destination: "*" or "?"
                    break;

                case addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                    // these are computers in another data center
                    // we forward messages only when 'all' is true
                    //
                    broadcast = all; // destination: "*"
                    break;

                default:
                    // unknown/unexpected type of IP address, totally ignore
                    break;

                }
            }
            else if(remote_communicator)
            {
                // another snapcommunicator that connected to us
                //
                switch(remote_communicator->get_address().get_network_type())
                {
                case addr::addr::network_type_t::NETWORK_TYPE_LOOPBACK:
                    {
                        static bool warned(false);
                        if(!warned)
                        {
                            warned = true;
                            SNAP_LOG_WARNING("remote snap communicator was connected on a LOOPBACK IP address...");
                        }
                    }
                    break;

                case addr::addr::network_type_t::NETWORK_TYPE_PRIVATE:
                    // these are computers within the same local network (LAN)
                    // we forward messages if at least 'remote' is true
                    //
                    broadcast = remote; // destination: "*" or "?"
                    break;

                case addr::addr::network_type_t::NETWORK_TYPE_PUBLIC:
                    // these are computers in another data center
                    // we forward messages only when 'all' is true
                    //
                    broadcast = all; // destination: "*"
                    break;

                default:
                    // unknown/unexpected type of IP address, totally ignore
                    break;

                }
            }
            if(broadcast)
            {
                // get the IP address of the remote snapcommunicator
                //
                QString const address(QString::fromUtf8((conn
                            ? conn->get_address()
                            : remote_communicator->get_address()).to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY).c_str()));
                if(!informed_neighbors_list.contains(address))
                {
                    // not in the list of informed neighbors, add it and
                    // keep nc in a list that we can use to actually send
                    // the broadcast message
                    //
                    informed_neighbors_list << address;
                    broadcast_connection.push_back(nc);
                }
            }
        }
    }
    else
    {
        // we already have a list, copy that list only as it is already
        // well defined
        //
        std::for_each(
            accepting_remote_connections.begin(),
            accepting_remote_connections.end(),
            [&broadcast_connection, &informed_neighbors_list](auto const & nc)
            {
                // the dynamic_cast<>() should always work in this direction
                //
                service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(nc));
                if(conn)
                {
                    QString const address(QString::fromUtf8(conn->get_address().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY).c_str()));
                    if(!informed_neighbors_list.contains(address))
                    {
                        // not in the list of informed neighbors, add it and
                        // keep conn in a list that we can use to actually
                        // send the broadcast message
                        //
                        informed_neighbors_list << address;
                        broadcast_connection.push_back(conn);
                    }
                }
                else
                {
                    remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(nc));
                    if(remote_communicator)
                    {
                        QString const address(QString::fromUtf8(remote_communicator->get_address().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY).c_str()));
                        if(!informed_neighbors_list.contains(address))
                        {
                            // not in the list of informed neighbors, add it and
                            // keep conn in a list that we can use to actually
                            // send the broadcast message
                            //
                            informed_neighbors_list << address;
                            broadcast_connection.push_back(remote_communicator);
                        }
                    }
                }
            });
    }

    if(!broadcast_connection.empty())
    {
        // we are broadcasting now (Gossiping a regular message);
        // for the gossiping to work, we include additional
        // information in the message
        //
        QString const originator(QString::fromUtf8(f_my_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_BRACKETS).c_str()));
        if(!informed_neighbors_list.contains(originator))
        {
            // include self since we already know of the message too!
            // (no need for others to send it back to us)
            //
            informed_neighbors_list << originator;
        }

        // message is 'const', so we need to create a copy
        snap::snap_communicator_message broadcast_msg(message);

        // generate a unique broadcast message identifier if we did not
        // yet have one, it is very important to NOT generate a new message
        // in a many to many broadcasting system because you must block
        // duplicates here
        //
        ++g_broadcast_sequence;
        if(broadcast_msgid.isEmpty())
        {
            broadcast_msgid = QString("%1-%2").arg(f_server_name).arg(g_broadcast_sequence);
        }
        broadcast_msg.add_parameter("broadcast_msgid", broadcast_msgid);

        // increase the number of hops; if we reach the limit, we still
        // want to forward the message, the destination will not forward
        // (broadcast) more, but it will possibly send that to its own
        // services
        //
        broadcast_msg.add_parameter("broadcast_hops", hops + 1);

        // mainly noise at this point, but I include the originator so
        // we can track that back if needed for debug purposes
        //
        broadcast_msg.add_parameter("broadcast_originator", originator);

        // define a timeout if this is the originator
        //
        if(timeout == 0)
        {
            // give message 10 seconds to arrive to any and all destinations
            timeout = time(nullptr) + 10;
        }
        broadcast_msg.add_parameter("broadcast_timeout", timeout);

        // note that we currently define the list of neighbors BEFORE
        // sending the message (anyway the send_message() just adds the
        // message to a memory cache at this point, so whether it will
        // be sent is not known until later.)
        //
        broadcast_msg.add_parameter("broadcast_informed_neighbors", informed_neighbors_list.join(","));

        for(auto const & bc : broadcast_connection)
        {
            service_connection::pointer_t conn(std::dynamic_pointer_cast<service_connection>(bc));
            if(conn)
            {
                conn->send_message(broadcast_msg);
            }
            else
            {
                remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(bc));
                if(remote_communicator) // this should always be true, but to be double sure...
                {
                    remote_communicator->send_message(broadcast_msg);
                }
            }
        }
    }
}


/** \brief Send the current status of a client to connections.
 *
 * Some connections (at this time only the snapwatchdog) may be interested
 * by the STATUS event. Any connection that understands the STATUS
 * event will be sent that event whenever the status of a connection
 * changes (specifically, on a REGISTER and on an UNREGISTER or equivalent.)
 *
 * \param[in] client  The client that just had its status changed.
 * \param[in] reply_connection  If not nullptr, the connection where the
 *                              STATUS message gets sent.
 */
void server::send_status(
              snap::snap_communicator::snap_connection::pointer_t connection
            , snap::snap_communicator::snap_connection::pointer_t * reply_connection)
{
    snap::snap_communicator_message reply;
    reply.set_command("STATUS");
    reply.add_parameter("cache", "no");

    // the name of the service is the name of the connection
    //
    reply.add_parameter("service", connection->get_name());

    base_connection::pointer_t base_connection(std::dynamic_pointer_cast<base_connection>(connection));
    if(base_connection != nullptr)
    {
        // include the server name
        //
        QString const server_name(base_connection->get_server_name());
        if(!server_name.isEmpty())
        {
            reply.add_parameter("server_name", server_name);
        }

        // check whether the connection is now up or down
        //
        base_connection::connection_type_t const type(base_connection->get_connection_type());
        reply.add_parameter("status", type == base_connection::connection_type_t::CONNECTION_TYPE_DOWN ? "down" : "up");

        // get the time when it was considered up
        //
        time_t const up_since(base_connection->get_connection_started());
        if(up_since != -1)
        {
            reply.add_parameter("up_since", up_since);
        }

        // get the time when it was considered down (if not up yet, this will be skipped)
        //
        time_t const down_since(base_connection->get_connection_ended());
        if(down_since != -1)
        {
            reply.add_parameter("down_since", down_since);
        }
    }

    if(reply_connection != nullptr)
    {
        service_connection::pointer_t sc(std::dynamic_pointer_cast<service_connection>(*reply_connection));
        if(sc)
        {
            // if the verify_command() fails then it means the caller has to
            // create a handler for the STATUS message
            //
            verify_command(sc, reply);
            sc->send_message(reply);
        }
    }
    else
    {
        // we have the message, now we need to find the list of connections
        // interested by the STATUS event
        //
        // TODO: use the broadcast_message() function instead? (with service set to ".")
        //
        snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
        for(auto const & conn : all_connections)
        {
            service_connection::pointer_t sc(std::dynamic_pointer_cast<service_connection>(conn));
            if(sc == nullptr)
            {
                // not a service_connection, ignore (i.e. servers)
                //
                continue;
            }

            if(sc->understand_command("STATUS"))
            {
                // send that STATUS message
                //
                //verify_command(sc, reply); -- we reach this line only if the command is understood
                sc->send_message(reply);
            }
        }
    }
}


/** \brief Check our current cluster status.
 *
 * We received or lost a connection with a remote computer and
 * need to determine (again) whether we are part of a cluster
 * or not.
 *
 * This function is also called when we receive the CLUSTERSTATUS
 * which is a query to know now what the status of the cluster is.
 * This is generally sent by daemons who need to know and may have
 * missed our previous broadcasts.
 *
 * \param[in] reply_connection  A connection to reply to directly.
 */
void server::cluster_status(snap::snap_communicator::snap_connection::pointer_t reply_connection)
{
    // the count_live_connections() counts all the other snapcommunicators,
    // not ourself, this is why we have a +1 here (it is very important
    // if you have a single computer like many developers would have when
    // writing code and testing quickly.)
    //
    size_t const count(f_remote_snapcommunicators->count_live_connections() + 1);

    // calculate the quorum, minimum number of computers that have to be
    // interconnected to be able to say we have a live cluster
    //
    size_t const total_count(f_all_neighbors.size());
    size_t const quorum(total_count / 2 + 1);
    bool modified = false;

    QString const new_status(count >= quorum ? "CLUSTERUP" : "CLUSTERDOWN");
    if(new_status != f_cluster_status
    || f_total_count_sent != total_count
    || reply_connection != nullptr)
    {
        if(reply_connection == nullptr)
        {
            f_cluster_status = new_status;
            modified = true;
        }

        // send the results to either the requesting connection or broadcast
        // the status to everyone
        //
        snap::snap_communicator_message cluster_status_msg;
        cluster_status_msg.set_command(new_status);
        cluster_status_msg.set_service(".");
        cluster_status_msg.add_parameter("neighbors_count", total_count);
        if(reply_connection != nullptr)
        {
            // reply to a direct CLUSTERSTATUS
            //
            service_connection::pointer_t r(std::dynamic_pointer_cast<service_connection>(reply_connection));
            if(r->understand_command(cluster_status_msg.get_command()))
            {
                r->send_message(cluster_status_msg);
            }
        }
        else
        {
            broadcast_message(cluster_status_msg);
        }
    }

    QString const new_complete(count == total_count ? "CLUSTERCOMPLETE" : "CLUSTERINCOMPLETE");
    if(new_complete != f_cluster_complete
    || f_total_count_sent != total_count
    || reply_connection != nullptr)
    {
        if(reply_connection == nullptr)
        {
            f_cluster_complete = new_complete;
            modified = true;
        }

        // send the results to either the requesting connection or broadcast
        // the complete to everyone
        //
        snap::snap_communicator_message cluster_complete_msg;
        cluster_complete_msg.set_command(new_complete);
        cluster_complete_msg.set_service(".");
        cluster_complete_msg.add_parameter("neighbors_count", total_count);
        if(reply_connection != nullptr)
        {
            // reply to a direct CLUSTERSTATUS
            //
            service_connection::pointer_t r(std::dynamic_pointer_cast<service_connection>(reply_connection));
            if(r->understand_command(cluster_complete_msg.get_command()))
            {
                r->send_message(cluster_complete_msg);
            }
        }
        else
        {
            broadcast_message(cluster_complete_msg);
        }
    }

    if(reply_connection == nullptr)
    {
        f_total_count_sent = total_count;
    }

    if(modified)
    {
        std::ofstream status_file;
        status_file.open(g_status_filename);
        if(status_file.is_open())
        {
            status_file << f_cluster_status << std::endl
                        << f_cluster_complete << std::endl;
        }
    }

    SNAP_LOG_INFO("cluster status is \"")
                 (new_status)
                 ("\" and \"")
                 (new_complete)
                 ("\" (count: ")
                 (count)
                 (", total count: ")
                 (total_count)
                 (", quorum: ")
                 (quorum)
                 (")");
}


/** \brief Request LOADAVG messages from a snapcommunicator.
 *
 * This function gets called whenever a local service sends us a
 * request to listen to the LOADAVG messages of a specific
 * snapcommunicator.
 *
 * \param[in] message  The LISTENLOADAVG message.
 */
void server::listen_loadavg(snap::snap_communicator_message const & message)
{
    QString const ips(message.get_parameter("ips"));

    snap::snap_string_list ip_list(ips.split(","));

    // we have to save those as IP addresses since the remote
    // snapcommunicators come and go and we have to make sure
    // that all get our REGISERFORLOADAVG message when they
    // come back after a broken link
    //
    for(auto const & ip : ip_list)
    {
        if(!f_registered_neighbors_for_loadavg.contains(ip))
        {
            // add this one, it was not there yet
            //
            f_registered_neighbors_for_loadavg[ip] = true;

            register_for_loadavg(ip);
        }
    }
}


void server::register_for_loadavg(QString const & ip)
{
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    auto const & it(std::find_if(
            all_connections.begin(),
            all_connections.end(),
            [ip](auto const & connection)
            {
                remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(connection));
                if(remote_communicator)
                {
                    return remote_communicator->get_my_address() == ip;
                }
                else
                {
                    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
                    if(service_conn)
                    {
                        return service_conn->get_my_address() == ip;
                    }
                }

                return false;
            }));

    if(it != all_connections.end())
    {
        // there is such a connection, send it a request for
        // LOADAVG message
        //
        snap::snap_communicator_message register_message;
        register_message.set_command("REGISTERFORLOADAVG");

        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(*it));
        if(remote_communicator)
        {
            remote_communicator->send_message(register_message);
        }
        else
        {
            service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(*it));
            if(service_conn)
            {
                service_conn->send_message(register_message);
            }
        }
    }
}


void server::save_loadavg(snap::snap_communicator_message const & message)
{
    QString const avg_str(message.get_parameter("avg"));
    QString const my_address(message.get_parameter("my_address"));
    QString const timestamp_str(message.get_parameter("timestamp"));

    snap::loadavg_item item;

    // Note: we do not use the port so whatever number here is fine
    addr::addr a(addr::string_to_addr(my_address.toUtf8().data(), "127.0.0.1", 4040, "tcp"));
    a.set_port(4040); // actually force the port so in effect it is ignored
    a.get_ipv6(item.f_address);

    bool ok(false);
    item.f_avg = avg_str.toFloat(&ok);
    if(!ok
    || item.f_avg < 0.0)
    {
        return;
    }

    item.f_timestamp = timestamp_str.toLongLong(&ok, 10);
    if(!ok
    || item.f_timestamp < SNAP_UNIX_TIMESTAMP(2016, 1, 1, 0, 0, 0))
    {
        return;
    }

    snap::loadavg_file file;
    file.load();
    file.add(item);
    file.save();
}


void server::process_load_balancing()
{
    std::ifstream in;
    in.open("/proc/loadavg", std::ios::in | std::ios::binary);
    if(in.is_open())
    {
        std::string avg_str;
        for(;;)
        {
            char c;
            in.read(&c, 1);
            if(in.fail())
            {
                SNAP_LOG_ERROR("error reading the /proc/loadavg data.");
                return;
            }
            if(std::isspace(c))
            {
                // we only read the first number (1 min. load avg.)
                break;
            }
            avg_str += c;
        }

        // we really only need the first number, we would not know what
        // to do with the following ones at this time...
        // (although that could help know whether the load average is
        // going up or down, but it's not that easy, really.)
        //
        // we divide by the number of processors because each computer
        // could have a different number of processors and a load
        // average of 1 on a computer with 16 processors really
        // represents 1/16th of the machine capacity.
        //
        float const avg(std::stof(avg_str) / f_number_of_processors);

        // TODO: see whether the current epsilon is good enough
        if(std::fabs(f_last_loadavg - avg) < 0.1f)
        {
            // do not send if it did not change lately
            return;
        }
        f_last_loadavg = avg;

        snap::snap_communicator_message load_avg;
        load_avg.set_command("LOADAVG");
        load_avg.add_parameter("avg", QString("%1").arg(avg));
        load_avg.add_parameter("my_address", QString::fromUtf8(f_my_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str()));
        load_avg.add_parameter("timestamp", QString("%1").arg(time(nullptr)));

        snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
        std::for_each(
                all_connections.begin(),
                all_connections.end(),
                [load_avg](auto const & connection)
                {
                    base_connection::pointer_t base(std::dynamic_pointer_cast<base_connection>(connection));
                    if(base
                    && base->wants_loadavg())
                    {
                        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(connection));
                        if(remote_communicator)
                        {
                            remote_communicator->send_message(load_avg);
                        }
                        else
                        {
                            service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
                            if(service_conn)
                            {
                                service_conn->send_message(load_avg);
                            }
                        }
                    }
                });
    }
    else
    {
        SNAP_LOG_ERROR("error opening file \"/proc/loadavg\".");
    }
}


/** \brief Return the list of services offered on this computer.
 */
QString server::get_local_services() const
{
    return f_local_services;
}


/** \brief Return the list of services we heard of.
 */
QString server::get_services_heard_of() const
{
    return f_services_heard_of;
}


/** \brief Add neighbors to this communicator server.
 *
 * Whenever a snap communicator connects to another snap communicator
 * server, it is given a list of neighbors. These are added using
 * this function. In the end, all servers are expected to have a
 * complete list of all the neighbors.
 *
 * \todo
 * Make this list survive restarts of the snap communicator server.
 *
 * \param[in] new_neighbors  The list of new neighbors
 */
void server::add_neighbors(std::string const & new_neighbors)
{
    SNAP_LOG_DEBUG
        << "Add neighbors: "
        << new_neighbors
        << SNAP_LOG_SEND;

    // first time initialize and read the cache file
    //
    read_neighbors();

    sorted_list_of_strings_t list;
    snapdev::tokenize_string(new_neighbors, list, { "," });
    if(!list.empty())
    {
        bool changed(false);
        for(auto const & s : list)
        {
            if(f_all_neighbors.insert(s).second)
            {
                changed = true;

                // in case we are already running we want to also add
                // the corresponding connection
                //
                f_remote_snapcommunicators->add_remote_communicator(s);
            }
        }

        // if the map changed, then save the change in the cache
        //
        // TODO: we may be able to optimize this by not saving on each and
        //       every call; although since it should remain relatively
        //       small, we should be fine (yes, 8,000 computers is still
        //       a small file in this cache.)
        //
        if(changed)
        {
            save_neighbors();
        }
    }
}


/** \brief Remove a neighbor from our list of neighbors.
 *
 * This function removes a neighbor from the cache of this machine. If
 * the neighbor is also defined in the configuration file, such as
 * /etc/snapwebsites/snapcommunicator.conf, then the IP will not be
 * forgotten any time soon.
 *
 * \param[in] neighbor  The neighbor to be removed.
 */
void server::remove_neighbor(QString const & neighbor)
{
    SNAP_LOG_DEBUG("Forgetting neighbor: ")(neighbor)(f_all_neighbors.contains(neighbor) ? " (exists)" : "");

    // remove the IP from the neighbors.txt file if still present there
    //
    if(f_all_neighbors.contains(neighbor))
    {
        f_all_neighbors.remove(neighbor);
        save_neighbors();
    }

    // make sure we stop all gossiping toward that address
    //
    f_remote_snapcommunicators->gossip_received(neighbor);

    // also remove the remote connection otherwise it will send that
    // info in broadcast messages and the neighbor resaved in those
    // other platforms neighbors.txt files
    //
    f_remote_snapcommunicators->forget_remote_connection(neighbor);
}


/** \brief Read the list of neighbors from disk.
 *
 * The first time we deal with our list of neighbors we need to call this
 * file to make sure we get that list ready as expected, which is with
 * all the IP:port previously saved in the neighbors.txt file.
 */
void server::read_neighbors()
{
    if(f_neighbors_cache_filename.isEmpty())
    {
        // get the path to the dynamic snapwebsites data files
        //
        // TODO: rename the variable so it does not say "cache", putting
        //       this file under /var/cache/snapwebsites is not a good
        //       idea since it can get deleted and the system connections
        //       are likely to break as a result... we already moved the
        //       data to /var/lib/snapwebsites instead, but did not change
        //       the variable names
        //
        f_neighbors_cache_filename = f_server->get_parameter("data_path");
        if(f_neighbors_cache_filename.isEmpty())
        {
            f_neighbors_cache_filename = "/var/lib/snapwebsites";
        }
        f_neighbors_cache_filename += "/neighbors.txt";

        QFile cache(f_neighbors_cache_filename);
        if(cache.open(QIODevice::ReadOnly))
        {
            char buf[1024];
            for(;;)
            {
                qint64 const r(cache.readLine(buf, sizeof(buf)));
                if(r < 0)
                {
                    break;
                }
                if(r > 0
                && buf[0] != '#')
                {
                    QString const line(QString::fromUtf8(buf, r).trimmed());
                    f_all_neighbors[line] = true;

                    // in case we are already running we want to also add
                    // the corresponding connection
                    //
                    f_remote_snapcommunicators->add_remote_communicator(line);
                }
            }
        }
        else
        {
            SNAP_LOG_DEBUG("neighbor file \"")(f_neighbors_cache_filename)("\" could not be read.");
        }
    }
}


/** \brief Save the current list of neighbors to disk.
 *
 * Whenever the list of neighbors changes, this function gets called
 * so the changes can get save on disk and reused on a restart.
 */
void server::save_neighbors()
{
    if(f_neighbors_cache_filename.isEmpty())
    {
        throw std::logic_error("Somehow save_neighbors() was called when f_neighbors_cache_filename was not set yet.");
    }

    QFile cache(f_neighbors_cache_filename);
    if(cache.open(QIODevice::WriteOnly))
    {
        for(sorted_list_of_strings_t::const_iterator n(f_all_neighbors.begin());
                                                     n != f_all_neighbors.end();
                                                     ++n)
        {
            cache.write(n.key().toUtf8());
            cache.putChar('\n');
        }
    }
    else
    {
        SNAP_LOG_ERROR("could not open cache file \"")(f_neighbors_cache_filename)("\" for writing.");
    }
}


/** \brief The list of services we know about from other snapcommunicators.
 *
 * This function gathers the list of services that this snapcommunicator
 * heard of. This means, the list of all the services offered by other
 * snapcommunicators, heard of or not, minus our own services (because
 * these other servers will return our own services as heard of!)
 */
void server::refresh_heard_of()
{
    // reset the list
    f_services_heard_of_list.clear();

    // first gather all the services we have access to
    snap::snap_communicator::snap_connection::vector_t const & all_connections(f_communicator->get_connections());
    for(auto const & connection : all_connections)
    {
        service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
        if(!c)
        {
            // not a service_connection, ignore (i.e. servers)
            continue;
        }

        // get list of services and heard of services
        c->get_services(f_services_heard_of_list);
        c->get_services_heard_of(f_services_heard_of_list);
    }

    // now remove services we are in control of
    for(sorted_list_of_strings_t::const_iterator it(f_local_services_list.begin());
                                                 it != f_local_services_list.end();
                                                 ++it)
    {
        f_services_heard_of_list.remove(it.key());
    }

    // generate a string we can send in a CONNECT or an ACCEPT
    f_services_heard_of.clear();
    for(sorted_list_of_strings_t::const_iterator it(f_services_heard_of_list.begin());
                                                 it != f_services_heard_of_list.end();
                                                 ++it)
    {
        f_services_heard_of += it.key() + ",";
    }
    if(!f_services_heard_of.isEmpty())
    {
        // remove the ending ","
        f_services_heard_of.remove(f_services_heard_of.length() - 1, 1);
    }

    // done
}


/** \brief This snapcommunicator received the SHUTDOWN or a STOP command.
 *
 * This function processes the SHUTDOWN or STOP commands. It is a bit of
 * work since we have to send a message to all connections and the message
 * vary depending on the type of connection.
 *
 * \param[in] quitting  Do a full shutdown (true) or just a stop (false).
 */
void server::shutdown(bool quitting)
{
    // from now on, we are shutting down; use this flag to make sure we
    // do not accept any more REGISTER, CONNECT and other similar
    // messages
    //
    f_shutdown = true;

    SNAP_LOG_DEBUG("shutting down snapcommunicator (")(quitting ? "QUIT" : "STOP")(")");

    // all gossiping can stop at once, since we cannot recognize those
    // connections in the list returned by f_communicator, we better
    // do that cleanly ahead of time
    //
    if(f_remote_snapcommunicators)
    {
        f_remote_snapcommunicators->stop_gossiping();
    }

    // DO NOT USE THE REFERENCE -- we need a copy of the vector
    // because the loop below uses remove_connection() on the
    // original vector!
    //
    snap::snap_communicator::snap_connection::vector_t const all_connections(f_communicator->get_connections());
    for(auto const & connection : all_connections)
    {
        // a remote communicator for which we initiated a new connection?
        //
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(connection));
        if(remote_communicator)
        {

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a "reply"

            // remote communicators are just timers and can be removed
            // as is, no message are sent there (no interface to do so anyway)
            //
            snap::snap_communicator_message reply;

            // a remote snapcommunicator server needs to also
            // shutdown so duplicate that message there
            if(quitting)
            {
                // SHUTDOWN means we shutdown the entire cluster!!!
                //
                reply.set_command("SHUTDOWN");
            }
            else
            {
                // STOP means we do not shutdown the entire cluster
                // so here we use DISCONNECT instead
                //
                reply.set_command("DISCONNECT");
            }

            // we know this a remote snapcommunicator, no need to verify, and
            // we may not yet have received the ACCEPT message
            //verify_command(remote_communicator, reply);
            remote_communicator->send_message(reply);

            // we are quitting so we want the permanent connection
            // to exit ASAP, by marking as done, it will stop as
            // soon as the message is written to the socket
            //
            remote_communicator->mark_done(true);
        }
        else
        {
            // a standard service connection or a remote snapcommunicator server?
            //
            service_connection::pointer_t c(std::dynamic_pointer_cast<service_connection>(connection));
            if(c)
            {
                base_connection::connection_type_t const type(c->get_connection_type());
                if(type == service_connection::connection_type_t::CONNECTION_TYPE_DOWN)
                {
                    // not initialized, just get rid of that one
                    f_communicator->remove_connection(c);
                }
                else
                {
                    snap::snap_communicator_message reply;
                    if(type == service_connection::connection_type_t::CONNECTION_TYPE_REMOTE)
                    {

// TODO: if the remote communicator IP address is the same as the
//       STOP, DISCONNECT, or SHUTDOWN message we just received,
//       then we have to just disconnect (HUP) instead of sending
//       a reply

                        // a remote snapcommunicator server needs to also
                        // shutdown so duplicate that message there
                        if(quitting)
                        {
                            // SHUTDOWN means we shutdown the entire cluster!!!
                            reply.set_command("SHUTDOWN");
                        }
                        else
                        {
                            // DISCONNECT means only we are going down
                            reply.set_command("DISCONNECT");
                        }

                        verify_command(c, reply);
                        c->send_message(reply);

                        // we cannot yet remove the connection from the
                        // communicator or the message would never be sent...
                        //
                        // the remote connections are expected to disconnect
                        // us when they receive a DISCONNECT, but really we
                        // disconnect ourselves as soon as we sent the
                        // message, no need to wait any longer
                        //
                        connection->mark_done();
                    }
                    else
                    {
                        // a standard client (i.e. pagelist, images, etc.)
                        // may want to know when it gets disconnected
                        // from the snapcommunicator...
                        //
                        if(c->understand_command("DISCONNECTING"))
                        {
                            // close connection as soon as the message was
                            // sent (i.e. we are "sending the last message")
                            //
                            connection->mark_done();

                            reply.set_command("DISCONNECTING");
                            c->send_message(reply);
                        }
                        else if(c->has_output())
                        {
                            // we just sent some data to that connection
                            // so we do not want to kill it immediately
                            //
                            // instead we mark it done so once the write
                            // buffer gets empty, the connection gets
                            // removed (See process_empty_buffer())
                            //
                            connection->mark_done();
                        }
                        else
                        {
                            // that local connection does not understand
                            // DISCONNECTING and has nothing more in its
                            // buffer, so just remove it immediately
                            //
                            // we will not accept new local connections since
                            // we also remove the f_local_listener connection
                            //
                            f_communicator->remove_connection(connection);
                        }
                    }
                }
            }
            // else -- ignore the main TCP and UDP servers which we
            //         handle below
        }
    }

    // remove the two main servers; we will not respond to any more
    // requests anyway
    //
    f_communicator->remove_connection(f_interrupt);         // TCP/IP
    f_communicator->remove_connection(f_local_listener);    // TCP/IP
    f_communicator->remove_connection(f_listener);          // TCP/IP
    f_communicator->remove_connection(f_ping);              // UDP/IP
    f_communicator->remove_connection(f_loadavg_timer);     // load balancer timer

//#ifdef _DEBUG
    {
        snap::snap_communicator::snap_connection::vector_t const all_connections_remaining(f_communicator->get_connections());
        for(auto const & connection : all_connections_remaining)
        {
            SNAP_LOG_DEBUG("Connection still left after the shutdown() call: \"")(connection->get_name())("\"");
        }
    }
//#endif
}


void server::process_connected(snap::snap_communicator::snap_connection::pointer_t connection)
{
    snap::snap_communicator_message connect;
    connect.set_command("CONNECT");
    //connect.add_version_parameter("version", snap::snap_communicator::VERSION);
    connect.add_parameter("version", snap::snap_communicator::VERSION);
    connect.add_parameter("my_address", QString::fromUtf8(f_my_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT).c_str()));
    connect.add_parameter("server_name", f_server_name);
    if(!f_explicit_neighbors.isEmpty())
    {
        connect.add_parameter("neighbors", f_explicit_neighbors);
    }
    if(!f_local_services.isEmpty())
    {
        connect.add_parameter("services", f_local_services);
    }
    if(!f_services_heard_of.isEmpty())
    {
        connect.add_parameter("heard_of", f_services_heard_of);
    }
    service_connection::pointer_t service_conn(std::dynamic_pointer_cast<service_connection>(connection));
    if(service_conn)
    {
        service_conn->send_message(connect);
    }
    else
    {
        remote_snap_communicator_pointer_t remote_communicator(std::dynamic_pointer_cast<remote_snapcommunicator>(connection));
        if(remote_communicator)
        {
            remote_communicator->send_message(connect);
        }
    }

    // status changed for this connection
    //
    send_status(connection);
}



} // sc namespace
// vim: ts=4 sw=4 et

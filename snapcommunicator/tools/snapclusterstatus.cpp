// Snap Websites Server -- check the current status of the cluster
// Copyright (c) 2019-2022  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include "version.h"


// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_communicator_dispatcher.h>
#include <snapwebsites/snap_config.h>


// snapdev lib
//
#include <snapdev/not_reached.h>

// advgetopt lib
//
#include <advgetopt/exception.h>


// last include
//
#include <snapdev/poison.h>



namespace
{


class snapcluster;

class snapcluster_messenger
    : public snap::snap_communicator::snap_tcp_client_message_connection
{
public:
    typedef std::shared_ptr<snapcluster_messenger>     pointer_t;

                                snapcluster_messenger(snapcluster * sl, std::string const & addr, int port);
                                snapcluster_messenger(snapcluster_messenger const & rhs) = delete;
    virtual                     ~snapcluster_messenger() override {}

    snapcluster_messenger &     operator = (snapcluster_messenger const & rhs) = delete;

protected:
    // this is owned by a snaplock function so no need for a smart pointer
    // (and it would create a loop)
    snapcluster *               f_snapcluster = nullptr;
};


snapcluster_messenger::snapcluster_messenger(snapcluster * sl, std::string const & addr, int port)
    : snap_tcp_client_message_connection(addr, port)
    , f_snapcluster(sl)
{
    set_name("snapcluster messenger");
}



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snapcluster
    : public snap::snap_communicator::connection_with_send_message
    , public snap::dispatcher<snapcluster>
    , public std::enable_shared_from_this<snapcluster>
{
public:
    typedef std::shared_ptr<snapcluster>      pointer_t;

                                snapcluster(int argc, char * argv[]);
                                snapcluster(snapcluster const & rhs) = delete;
    virtual                     ~snapcluster() {}

    snapcluster &               operator = (snapcluster const & rhs) = delete;

    void                        run();

    // implementation of connection_with_send_message
    virtual bool                send_message(snap::snap_communicator_message const & message, bool cache = false) override;

    // implementation of snap::snapcommunicator::connection_with_send_message
    virtual void                ready(snap::snap_communicator_message & message) override; // no "msg_" because that's in connection_with_send_message
    virtual void                stop(bool quitting) override; // no "msg_" because that's in connection_with_send_message

private:
    void                        done(snap::snap_communicator_message & message);

    // messages handled by the dispatcher
    // (see also ready() and stop() above)
    //
    void                        msg_cluster_status(snap::snap_communicator_message & message);
    void                        msg_cluster_complete(snap::snap_communicator_message & message);

    static snap::dispatcher<snapcluster>::dispatcher_match::vector_t const
                                        g_snapcluster_service_messages;

    advgetopt::getopt                   f_opt;
    snap::snap_config                   f_config;
    QString                             f_communicator_addr = QString("localhost");
    int                                 f_communicator_port = 4040;
    snap::snap_communicator::pointer_t  f_communicator = snap::snap_communicator::pointer_t();
    snapcluster_messenger::pointer_t    f_messenger = snapcluster_messenger::pointer_t();
    QString                             f_cluster_status = QString();       // UP or DOWN
    QString                             f_cluster_complete = QString();     // COMPLETE or INCOMPLETE
    size_t                              f_neighbors_count = 0;
};
#pragma GCC diagnostic pop



/** \brief List of snapcluster commands.
 *
 * The following table defines the commands understood by snapcluster,
 * which are pretty limited, mainly we want to gather the status from
 * the snapcommunicator process.
 */
snap::dispatcher<snapcluster>::dispatcher_match::vector_t const snapcluster::g_snapcluster_service_messages =
{
    {
        "CLUSTERUP"
      , &snapcluster::msg_cluster_status
    },
    {
        "CLUSTERDOWN"
      , &snapcluster::msg_cluster_status
    },
    {
        "CLUSTERCOMPLETE"
      , &snapcluster::msg_cluster_complete
    },
    {
        "CLUSTERINCOMPLETE"
      , &snapcluster::msg_cluster_complete
    }
};



advgetopt::option const g_options[] =
{
    {
        'c',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "config",
        nullptr,
        "path to the snapcommunicator configuration file",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_END,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    }
};




// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPCOMMUNICATOR_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) "
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop







snapcluster::snapcluster(int argc, char * argv[])
    : dispatcher(this, g_snapcluster_service_messages)
    , f_opt(g_options_environment, argc, argv)
    , f_config("snapcommunicator")
{
    // --help
    if(f_opt.is_defined("help"))
    {
        f_opt.usage();
        exit(1);
        snap::NOT_REACHED();
    }

    // --version
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPCOMMUNICATOR_VERSION_STRING << std::endl;
        exit(1);
        snap::NOT_REACHED();
    }

    add_snap_communicator_commands();

    // read the configuration file
    //
    if(f_opt.is_defined("config"))
    {
        f_config.set_configuration_path(f_opt.get_string("config"));
    }

    tcp_client_server::get_addr_port(
                  QString::fromUtf8(f_config("snapcommunicator", "local_listen").c_str())
                , f_communicator_addr
                , f_communicator_port
                , "tcp");
}


void snapcluster::run()
{
    f_communicator = snap::snap_communicator::instance();

    f_messenger.reset(new snapcluster_messenger(this
                                              , f_communicator_addr.toUtf8().data()
                                              , f_communicator_port));
    f_messenger->set_dispatcher(shared_from_this());
    f_communicator->add_connection(f_messenger);

    // our messenger here is a direct connection (not a permanent one) so
    // we have to REGISTER immediately (if it couldn't connect we already
    // threw so this works)
    //
    snap::snap_communicator_message register_snapcluster;
    register_snapcluster.set_command("REGISTER");
    register_snapcluster.add_parameter("service", "snapcluster");
    register_snapcluster.add_parameter("version", snap::snap_communicator::VERSION);
    f_messenger->send_message(register_snapcluster);

    f_communicator->run();
}


bool snapcluster::send_message(snap::snap_communicator_message const & message, bool cache)
{
    return f_messenger->send_message(message, cache);
}


void snapcluster::ready(snap::snap_communicator_message & message)
{
    snap::NOT_USED(message);

    snap::snap_communicator_message clusterstatus_message;
    clusterstatus_message.set_command("CLUSTERSTATUS");
    clusterstatus_message.set_service("snapcommunicator");
    send_message(clusterstatus_message);
}


void snapcluster::stop(bool quitting)
{
    snap::NOT_USED(quitting);

    if(f_messenger != nullptr)
    {
        f_communicator->remove_connection(f_messenger);
        f_messenger.reset();
    }
}


void snapcluster::msg_cluster_status(snap::snap_communicator_message & message)
{
    f_cluster_status = message.get_command();
    done(message);
}


void snapcluster::msg_cluster_complete(snap::snap_communicator_message & message)
{
    f_cluster_complete = message.get_command();
    done(message);
}


void snapcluster::done(snap::snap_communicator_message & message)
{
    if(f_cluster_status.isEmpty()
    || f_cluster_complete.isEmpty())
    {
        // not quite done yet...
        return;
    }

    f_neighbors_count = message.get_integer_parameter("neighbors_count");

    // got our info!
    //
    std::cout << "              Status: " << f_cluster_status          << std::endl
              << "            Complete: " << f_cluster_complete        << std::endl
              << "Computers in Cluster: " << f_neighbors_count         << std::endl
              << " Quorum of Computers: " << f_neighbors_count / 2 + 1 << std::endl;

    // we're done, remove the messenger which is enough for the
    // snap_communicator::run() to return
    //
    stop(false);
}



}
// no name namespace


int main(int argc, char * argv[])
{
    try
    {
        snapcluster::pointer_t cluster(std::make_shared<snapcluster>(argc, argv));
        cluster->run();
        return 0;
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}


// vim: ts=4 sw=4 et

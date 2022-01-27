// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/** \file
 * \brief Implementation of the snaplogger daemon.
 *
 * This source file is the implementation of the main snaplogger daemon
 * object. This object manages all the connections and takes care of
 * logging the messages it receives from remote hosts.
 */

// self
//
#include    "snaplogger/daemon/snaploggerd.h"

#include    "snaplogger/daemon/version.h"



// advgetopt lib
//
#include    "advgetopt/exception.h"


// snaplogger lib
//
#include    "snaplogger/options.h"


// libaddr lib
//
#include    "libaddr/addr_parser.h"


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_daemon
{


namespace
{

const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("controller-listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("")
        , advgetopt::Help("an IP:Port to connect to the \"snapcommunicator\" RCP service")
    ),
    advgetopt::define_option(
          advgetopt::Name("logrotate-listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:4988")
        , advgetopt::Help("the host to listen on for the logrotate LOG message")
    ),
    advgetopt::define_option(
          advgetopt::Name("logrotate-secret-code")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("")
        , advgetopt::Help("a secret code to be used along the logrotate-listen option; use empty to not have to use a secret code")
    ),
    advgetopt::define_option(
        advgetopt::Name("tcp-listen")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
          , advgetopt::GETOPT_FLAG_REQUIRED>())
      , advgetopt::Help("The server TCP connection listening for LOGGER messages.")
    ),
    advgetopt::define_option(
        advgetopt::Name("udp-listen")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
          , advgetopt::GETOPT_FLAG_REQUIRED>())
      , advgetopt::Help("The server UDP connection listening for LOGGER messages.")
    ),
    advgetopt::define_option(
          advgetopt::Name("udp-listen-secret-code")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("")
        , advgetopt::Help("a secret code to be used along the udp-listen option; use empty to not have to use a secret code")
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
    "/etc/snaploggerd/snaploggerd.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snaploggerd",
    .f_group_name = "snaploggerd",
    .f_options = g_options,
    .f_options_files_directory = "/etc/snaploggerd/snaploggerd.d",
    .f_environment_variable_name = "SNAPLOGGERD",
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPLOGGERD_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = "Copyright (c) 2021-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop


}
// no name namepsace


snaploggerd::snaploggerd(int argc, char * argv[])
    : f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/snaploggerd/logger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 0);
    }

    // TODO: implement the controller listener
    //       we first need a replacement to the snapcontroller daemon
    {
        std::string controller_listen(f_opts.get_string("controller-listen"));
        addr::addr const listen(addr::string_to_addr(
              controller_listen
            , "127.0.0.1"
            , DEFAULT_CONTROLLER_PORT
            , "udp"));
    }

    std::string logrotate_listen(f_opts.get_string("logrotate-listen"));
    if(!logrotate_listen.empty())
    {
        addr::addr const listen(addr::string_to_addr(
              logrotate_listen
            , "127.0.0.1"
            , DEFAULT_LOGROTATE_PORT
            , "udp"));
        f_logrotate_connection = std::make_shared<ed::logrotate_udp_messenger>(
              listen
            , f_opts.get_string("logrotate-secret-code"));
        f_communicator->add_connection(f_logrotate_connection);
    }
}


bool snaploggerd::init()
{
    std::string const tcp_listen(f_opts.get_string("tcp-listen"));
    if(!tcp_listen.empty())
    {
        addr::addr const listen(addr::string_to_addr(
              tcp_listen
            , "127.0.0.1"
            , DEFAULT_TCP_PORT
            , "tcp"));
        f_tcp_server = std::make_shared<tcp_logger_server>(listen);
        f_communicator->add_connection(f_tcp_server);
    }

    std::string const udp_listen(f_opts.get_string("udp-listen"));
    if(!udp_listen.empty())
    {
        addr::addr const listen(addr::string_to_addr(
              udp_listen
            , "127.0.0.1"
            , DEFAULT_UDP_PORT
            , "udp"));
        f_udp_server = std::make_shared<udp_logger_server>(
              listen
            , f_opts.get_string("udp-listen-secret-code"));
        f_communicator->add_connection(f_udp_server);
    }

    return true;
}


int snaploggerd::run()
{
    f_communicator->run();

    return 0;
}


} // snaplogger namespace
// vim: ts=4 sw=4 et

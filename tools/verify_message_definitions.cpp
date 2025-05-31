// Snap Websites Server -- to send SIGINT signal to stop a daemon
// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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


// eventdispatcher
//
#include    <eventdispatcher/message_definition.h>
#include    <eventdispatcher/version.h>


// cppprocess
//
#include    <cppprocess/process_list.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/options.h>
#include    <advgetopt/exception.h>


// snaplogger
//
#include    <snaplogger/logger.h>
#include    <snaplogger/options.h>


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/stringize.h>


// C++
//
#include    <iostream>


// C
//
#include    <signal.h>
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



advgetopt::option const g_options[] =
{
    // `--service` is not required because systemd removes the parameter
    // altogether when $MAINPID is empty (even with the quotes)
    //
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("show commands and their parameters as the list of commands is being processed.")
    ),
    advgetopt::define_option(
          advgetopt::Name("commands")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_DEFAULT_OPTION
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("list of one or more message commands to verify.")
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



advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "verify-message-definitions",
    .f_group_name = "eventdispatcher",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "VERIFY_MESSAGE_DEFINITIONS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = EVENTDISPATCHER_VERSION_STRING,
    .f_license = "GNU GPL v2 or newer",
    .f_copyright = "Copyright (c) 2011-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};



}
// no name namespace




int main(int argc, char *argv[])
{
    try
    {
        advgetopt::getopt opts(g_options_environment);
        snaplogger::add_logger_options(opts);
        ed::add_message_definition_options(opts);
        opts.finish_parsing(argc, argv);
        if(!snaplogger::process_logger_options(opts, "/etc/eventdispatcher/logger", std::cout, false))
        {
            throw advgetopt::getopt_exit("logger options generated an error.", 0);
        }
        ed::process_message_definition_options(opts);
        snaplogger::logger::get_instance()->set_fatal_error_severity(snaplogger::severity_t::SEVERITY_WARNING);

        // make sure there is at least one command
        //
        if(!opts.is_defined("commands"))
        {
            std::cerr << "verify-message-definitions: error: at least one message name needs to be specified." << std::endl;
            exit(1);
        }
        std::size_t const size(opts.size("commands"));

        bool const verbose(opts.is_defined("verbose"));

        for(std::size_t idx(0); idx < size; ++idx)
        {
            std::string name(opts.get_string("commands", idx));

            // because it's much easier to pass full paths from cmake,
            // I also apply a basename
            //
            name = snapdev::pathinfo::basename(name, ".conf");

            ed::message_definition::pointer_t def(ed::get_message_definition(name));
            if(verbose)
            {
                std::cout << "--- command: " << def->f_command << " ---\n";
                // TODO: display parameters with their flags/type...
            }
        }

        return 0;
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "verify-message-definitions: exception: " << e.what() << std::endl;
    }
    catch(...)
    {
        // clean error on exception
        std::cerr << "verify-message-definitions: an unknown exception occurred.\n";
    }
    return 1;
}


// vim: ts=4 sw=4 et

// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief A tool one can use to send logs from the command line.
 *
 * At times it is useful to send logs to the Snap! Logger system from the
 * command line or a script. This tool is here for that exact purpose.
 * It sends the logs using the parameters you provide on the command line.
 * It first loads defaults from configuration files that you can overwrite
 * with command line parameters.
 */


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/version.h>


// snaplogger
//
#include    <snaplogger/logger.h>
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// getopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/exception.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// snapdev
//
#include    <snapdev/stringize.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("components")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("define the name of one or more component the log pertains to.")
    ),
    advgetopt::define_option(
          advgetopt::Name("fields")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("a list of name=<value> fields")
    ),
    advgetopt::define_option(
          advgetopt::Name("filename")
        , advgetopt::ShortName('f')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("the name of the file where the log comes from.")
    ),
    advgetopt::define_option(
          advgetopt::Name("function")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("specify the name of a function in link with the log message")
    ),
    advgetopt::define_option(
          advgetopt::Name("line")
        , advgetopt::ShortName('l')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("the line where the error/issue occurred.")
    ),
    advgetopt::define_option(
          advgetopt::Name("message")
        , advgetopt::ShortName('m')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_DEFAULT_OPTION
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("the message to log (you may also use --message ... after a --fields or --components)")
    ),
    advgetopt::define_option(
          advgetopt::Name("severity")
        , advgetopt::ShortName('s')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("error")
        , advgetopt::Help("define the log message severity (default: \"error\")")
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
    "/etc/snaplogger/snaplog.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snaplog",
    .f_group_name = "snaplogger",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPLOG",
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
    .f_copyright = "Copyright (c) 2012-" SNAPDEV_STRINGIZE(UTC_BUILD_YEAR) "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop

}
// noname namespace






class snaplog
{
public:
                                    snaplog(int argc, char * argv[]);

    int                             run();

private:
    advgetopt::getopt               f_opt;
};








snaplog::snaplog(int argc, char * argv[])
    : f_opt(g_options_environment)
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(
              f_opt
            , "/etc/snaplogger/logger"
            , std::cout
            , false))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 0);
    }
}


int snaplog::run()
{
    std::string const severity_name(f_opt.get_string("severity"));
    snaplogger::severity::pointer_t sev(snaplogger::get_severity(severity_name));

    std::string filename;
    if(f_opt.is_defined("filename"))
    {
        filename = f_opt.get_string("filename");
    }

    std::string function;
    if(f_opt.is_defined("function"))
    {
        function = f_opt.get_string("function");
    }

    int line(0);
    if(f_opt.is_defined("line"))
    {
        line = f_opt.get_long("line");
    }

    snaplogger::message msg(sev->get_severity());

    int comp_max(f_opt.size("components"));
    for(int comp(0); comp < comp_max; ++comp)
    {
        std::string const c(f_opt.get_string("components", comp));
        snaplogger::component::pointer_t component(snaplogger::get_component(c));
        msg.add_component(component);
    }

    if(comp_max == 0)
    {
        msg.add_component(snaplogger::g_normal_component);
        msg.add_component(snaplogger::get_component("snaplog"));
    }

    int field_max(f_opt.size("fields"));
    for(int field(0); field < field_max; ++field)
    {
        std::string const f(f_opt.get_string("fields", field));
        std::string::size_type pos(f.find('='));
        std::string name;
        std::string value;
        if(pos == std::string::npos)
        {
            name = f;
        }
        else
        {
            name = f.substr(0, pos);
            value = f.substr(pos + 1);
        }
        msg.add_field(name, value);
    }

    if(f_opt.is_defined("message"))
    {
        msg << f_opt.get_string("message");
    }
    else
    {
        msg << "snaplog: log message.";
    }

    snaplogger::send_message(msg);

    // TODO: make sure that once the log was sent that we exit... if it fails
    //       it never exits at the moment (i.e. add a timer)
    //
    ed::communicator::instance()->run();

    return 0;
}





int main(int argc, char * argv[])
{
    try
    {
        snaplog s(argc, argv);
        return s.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(0);
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred (1): " << e.what() << std::endl;
        SNAP_LOG_FATAL
            << "an exception occurred (1): "
            << e.what()
            << SNAP_LOG_SEND;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred (2)." << std::endl;
        SNAP_LOG_FATAL
            << "an unknown exception occurred (2)."
            << SNAP_LOG_SEND;
        exit(2);
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et

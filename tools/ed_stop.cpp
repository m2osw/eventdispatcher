// Snap Websites Server -- to send SIGINT signal to stop a daemon
// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/version.h>


// cppprocess
//
#include    <cppprocess/process_list.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/options.h>
#include    <advgetopt/exception.h>


// snapdev
//
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
          advgetopt::Name("service")
        , advgetopt::ShortName('s')
        , advgetopt::Flags(advgetopt::command_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("PID (only digits) or name of the service to stop.")
    ),
    advgetopt::define_option(
          advgetopt::Name("timeout")
        , advgetopt::ShortName('t')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("60")
        , advgetopt::Help("number of seconds to wait for the process to die.")
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



// until we have C++20 remove warnings this way
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_snapstop_options_environment =
{
    .f_project_name = "ed-stop",
    .f_group_name = "eventdispatcher",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "ED_STOP",
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
#pragma GCC diagnostic pop




}
// no name namespace




int main(int argc, char *argv[])
{
    try
    {
        advgetopt::getopt opt(g_snapstop_options_environment, argc, argv);

        // make sure it is defined
        //
        if(!opt.is_defined("service"))
        {
            std::cerr << "ed-stop: error: --service parameter is mandatory." << std::endl;
            exit(1);
        }

        std::string const & service(opt.get_string("service"));
        if(service.empty())
        {
            // this happens when $MAINPID is not defined in the .service
            // as in:
            //
            //    ExecStop=/usr/bin/ed-stop --timeout 300 --service "$MAINPID"
            //
            // we just ignore this case silently; it means that the backend
            // is for sure not running anyway
            //
            //std::cerr << "ed-stop: error: --service parameter can't be empty." << std::endl;
            exit(0);
        }

        pid_t service_pid(-1);
        for(char const * s(service.c_str()); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                service_pid = -1;
                break;
            }
            if(service_pid == -1)
            {
                // we start with -1 so we have to have a special case...
                //
                service_pid = *s - '0';
            }
            else
            {
                service_pid = service_pid * 10 + *s - '0';
            }
        }
        if(service_pid == 0)
        {
            std::cerr << "ed-stop: error: --service 0 is not valid." << std::endl;
            exit(1);
        }

        if(service_pid < 0)
        {
            // get PID using systemctl
            //
            std::string cmd("systemctl show --property MainPID --value ");
            cmd += service;
            FILE * pid(popen(cmd.c_str(), "r"));
            if(pid == nullptr)
            {
                std::cerr
                    << "ed-stop: error: server named \""
                    << service
                    << "\" not found.\n";
                exit(1);
            }
            char buf[32];
            size_t sz(fread(buf, 1, sizeof(buf), pid));
            if(sz == 0)
            {
                std::cerr
                    << "ed-stop: error: could not read PID of service named \""
                    << service
                    << "\".\n";
                exit(1);
            }
            if(sz >= sizeof(buf))
            {
                std::cerr
                    << "ed-stop: error: the read PID of service named \""
                    << service
                    << "\" looks too long.\n";
                exit(1);
            }
            buf[sz] = '\0';

            service_pid = 0;
            for(char const * s(buf); *s != '\0'; ++s)
            {
                if(*s < '0' || *s > '9')
                {
                    std::cerr
                        << "ed-stop: error: the PID of \""
                        << service
                        << "\" returned by systemctl, \""
                        << buf
                        << "\", is not a valid number.\n";
                    exit(1);
                }
                service_pid = service_pid * 10 + *s - '0';
            }

            if(service_pid == 0)
            {
                // the server was not found or it is not running
                // we're done here
                //
                // TODO: find a way to generate an error in case the service
                //       was not found (i.e. misspelled, not installed, etc.)
                //
                exit(0);
            }
        }

        // verify that we have a process with that PID
        //
        if(!cppprocess::is_running(service_pid))
        {
            if(errno == EPERM)
            {
                std::cerr << "ed-stop: error: not permitted to send signal to --service " << service_pid << ". Do nothing." << std::endl;
            }
            else if(errno == ENOENT)
            {
                std::cerr << "ed-stop: error: --service " << service_pid << " is not running. Do nothing." << std::endl;
            }
            else
            {
                int const e(errno);
                std::cerr << "ed-stop: error: " << strerror(e) << ". Do nothing.\n";
            }
            exit(1);
        }

        // First try with a SIGINT which is a soft interruption; it will
        // not hurt whatever the process is currently doing and as soon as
        // possible it will be asked to stop as if it received the STOP
        // command in a message
        //
        // sending the signal worked, wait for the process to die
        //
        long timeout(opt.get_long("timeout"));
        if(timeout < 10)
        {
            // enforce a minimum of 10 seconds
            //
            timeout = 10;
        }
        else if(timeout > 3600)
        {
            // wait at most 1 hour (wow!)
            //
            timeout = 3600;
        }

        if(!cppprocess::is_running(service_pid, SIGINT, timeout))
        {
            // the process is dead now
            //
            exit(0);
        }

        // the SIGINT did not work, try again with SIGTERM
        //
        // this is not caught and transformed to a soft STOP, so it should
        // nearly never fail to stop the process very quickly...
        //
        // Note: we want to send SIGTERM ourselves because systemd really
        //       only offers two means of shutting down: (1) a signal of
        //       our choice, and (2) the SIGKILL after that;
        //
        //       although SIGTERM kills the process immediately, it still
        //       sends a message to the log file, which makes it useful
        //       for us to see how many times the SIGINT failed
        //

        // should we have another timeout option for this one?
        //
        // TODO: as with the other one we want to keep trying obtaining
        //       the flock() and have a SIGALRM for the timeout...
        //
        if(!cppprocess::is_running(service_pid, SIGTERM, 10))
        {
            // the process is dead now
            //
            exit(0);
        }

        // it timed out!?
        //
        std::cerr
            << "ed-stop: kill() had no effect on \""
            << service
            << "\" within the timeout period." << std::endl;
        exit(0);
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "ed-stop: exception: " << e.what() << std::endl;
    }
    catch(...)
    {
        // clean error on exception
        std::cerr << "ed-stop: an unknown exception occurred.\n";
    }
    exit(1);
}


// vim: ts=4 sw=4 et

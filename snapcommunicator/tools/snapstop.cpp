// Snap Websites Server -- to send SIGINT signal to stop a daemon
// Copyright (c) 2011-2020  Made to Order Software Corp.  All Rights Reserved
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



advgetopt::option const g_snapstop_options[] =
{
    { // `--service` is not required because systemd removes the parameter altogether when $MAINPID is empty (even with the quotes)
        's',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "service",
        nullptr,
        "PID (only digits) or name of the service to stop.",
        nullptr
    },
    {
        '\0',
        advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::GETOPT_FLAG_REQUIRED | advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "timeout",
        "60",
        "number of seconds to wait for the process to die, default is 60 seconds.",
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
advgetopt::options_environment const g_snapstop_options_environment =
{
    .f_project_name = "snapwebsites",
    .f_group_name = nullptr,
    .f_options = g_snapstop_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAPSTOP_OPTIONS",
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPWEBSITES_VERSION_STRING,
    .f_license = "GNU GPL v2",
    .f_copyright = "Copyright (c) 2011-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    //.f_build_date = UTC_BUILD_DATE,
    //.f_build_time = UTC_BUILD_TIME
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
            std::cerr << "snapstop: error: --service parameter is mandatory." << std::endl;
            exit(1);
        }

        std::string const & service(opt.get_string("service"));
        if(service.empty())
        {
            // this happens when $MAINPID is not defined in the .service
            // as in:
            //
            //    ExecStop=/usr/bin/snapstop --timeout 300 --service "$MAINPID"
            //
            // we just ignore this case silently; it means that the backend
            // is for sure not running anyway
            //
            //std::cerr << "snapstop: error: --service parameter can't be empty." << std::endl;
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

        if(service_pid <= 0)
        {
            if(service_pid == 0)
            {
                std::cerr << "snapstop: error: --service 0 is not valid." << std::endl;
                exit(1);
            }

            // TODO: load PID from a PID file for the named service...
            //
            std::cerr << "snapstop: error: --service <name> not supported yet, this will require us to create a corresponding pid." << std::endl;
            exit(1);
        }

        // verify that we have a process with that PID
        //
        if(kill(service_pid, 0) != 0)
        {
            if(errno == EPERM)
            {
                std::cerr << "snapstop: error: not permitted to send signal to --service " << service_pid << ". Do nothing." << std::endl;
            }
            else
            {
                std::cerr << "snapstop: error: --service " << service_pid << " is not running. Do nothing." << std::endl;
            }
            exit(1);
        }

        // First try with a SIGINT which is a soft interruption; it will
        // not hurt whatever the process is currently doing and as soon as
        // possible it will be asked to stop as if it received the STOP
        // command in a message
        //
        {
            int const r(kill(service_pid, SIGINT));
            if(r != 0)
            {
                perror("snapstop: kill() failed: ");
                exit(1);
            }

            // the signal worked worked, now wait for some time for the process to die
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
            time_t const time_limit(time(nullptr) + timeout);

            // TODO: once we have PID files that are locked by the process
            //       until it dies, we can actually do an flock(), once we
            //       obtain the lock we can quit, the process is dead and
            //       we can use a SIGALRM for the timeout and try SIGTERM.
            //
            do
            {
                // the kill() function returns immediately so we have to
                // sleep otherwise it would loop very quickly...
                //
                // (I do not know of a way to poll() on a dying process
                // unless it is your direct child or we have a lock...)
                //
                sleep(1);

                if(kill(service_pid, 0) != 0)
                {
                    // the process is dead now
                    //
                    exit(0);
                }
            }
            while(time_limit > time(nullptr));
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
        {
            int const r(kill(service_pid, SIGTERM));
            if(r != 0)
            {
                perror("snapstop: kill() failed: ");
                exit(1);
            }

            // should we have another timeout option for this one?
            //
            // TODO: as with the other one we want to keep trying obtaining
            //       the flock() and have a SIGALRM for the timeout...
            //
            time_t const term_time_limit(time(nullptr) + 10);

            do
            {
                // the kill() function returns immediately so we have to
                // sleep otherwise it would loop very quickly...
                //
                // (I do not know of a way to poll() on a dying process
                // unless it is your direct child or we have a lock...)
                //
                sleep(1);

                if(kill(service_pid, 0) != 0)
                {
                    // the process is dead now
                    //
                    exit(0);
                }
            }
            while(term_time_limit > time(nullptr));
        }

        // it timed out!?
        //
        std::cerr << "snapstop: kill() had no effect within the timeout period." << std::endl;
        return 0;
    }
    catch( advgetopt::getopt_exit const & except )
    {
        return except.code();
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapstop: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et

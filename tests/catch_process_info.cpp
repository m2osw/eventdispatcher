// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "catch_main.h"


// cppprocess lib
//
#include    <cppprocess/process_info.h>


// C lib
//
#include    <sys/resource.h>
#include    <sys/times.h>


// last include
//
#include    <snapdev/poison.h>



CATCH_TEST_CASE("Process Info", "[process]")
{
    CATCH_START_SECTION("check ourselves")
    {
        // information about ourselves
        //
        cppprocess::process_info info(getpid());

        CATCH_REQUIRE(info.get_pid() == getpid());
        CATCH_REQUIRE(info.get_ppid() == getppid());
        CATCH_REQUIRE(info.get_pgid() == getpgid(getpid()));

        rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        rusage cusage;
        getrusage(RUSAGE_CHILDREN, &cusage);

        tms process_times = {};
        times(&process_times);

        {
            char ** args = SNAP_CATCH2_NAMESPACE::g_argv;
            CATCH_REQUIRE(info.get_name() == "unittest");
            CATCH_REQUIRE(args[0] == info.get_command());
            CATCH_REQUIRE(info.get_basename() == "unittest");

            int idx = 1;
            for(; args[idx] != nullptr; ++idx)
            {
                CATCH_REQUIRE(info.get_arg(idx) == args[idx]);
            }
            CATCH_REQUIRE(static_cast<std::size_t>(idx) == info.get_args_size());
        }

        CATCH_REQUIRE(info.get_state() == cppprocess::process_state_t::PROCESS_STATE_RUNNING);

        {
            unsigned long long utime(0);
            unsigned long long stime(0);
            unsigned long long cutime(0);
            unsigned long long cstime(0);
            info.get_times(utime, stime, cutime, cstime);

//std::cerr << "times -- " << utime
//<< ", " << stime
//<< ", " << cutime
//<< ", " << cstime
//<< "\n";
//std::cerr << "direct times -- " << process_times.tms_utime
//<< ", " << process_times.tms_stime
//<< ", " << process_times.tms_cutime
//<< ", " << process_times.tms_cstime
//<< "\n";

            std::int64_t const ut(utime - process_times.tms_utime);
            CATCH_REQUIRE(labs(ut) <= 2);

            std::int64_t const st(stime - process_times.tms_stime);
            CATCH_REQUIRE(labs(st) <= 2);

            std::int64_t const cut(cutime - process_times.tms_cutime);
            CATCH_REQUIRE(labs(cut) <= 2);

            std::int64_t const cst(cstime - process_times.tms_cstime);
            CATCH_REQUIRE(labs(cst) <= 2);
        }

        // info says 20, getpriority() says 0
        //CATCH_REQUIRE(info.get_priority() == getpriority(PRIO_PROCESS, 0));
        CATCH_REQUIRE(info.get_nice() == nice(0));

        {
            std::uint64_t pf_major(0);
            std::uint64_t pf_minor(0);
            info.get_page_faults(pf_major, pf_minor);

            // WARNING
            // the following are rather random... as we add more tests
            // this can increase
            //
            std::int64_t const maj(pf_major - usage.ru_majflt);
            CATCH_REQUIRE(labs(maj) < 100);

            std::int64_t const min(pf_minor - usage.ru_minflt);
            CATCH_REQUIRE(labs(min) < 100);
        }

        // TODO: see how to use the rusage data to more or less match these
        CATCH_REQUIRE(info.get_total_size() != 0);
        CATCH_REQUIRE(info.get_rss_size() != 0);

        {
            int maj(0);
            int min(0);
            info.get_tty(maj, min);
            // how do we compare these maj:min with our tty?
        }
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

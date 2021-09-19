// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    <cppprocess/process.h>


// snapdev lib
//
#include    <snapdev/file_contents.h>


// C++ lib
//
#include    <fstream>


// C lib
//
//#include    <sys/resource.h>
//#include    <sys/times.h>


// last include
//
#include    <snapdev/poison.h>



CATCH_TEST_CASE("Process", "[process]")
{
    CATCH_START_SECTION("simple cat")
    {
        cppprocess::process p("simple-cat");

        CATCH_REQUIRE(p.get_name() == "simple-cat");

        CATCH_REQUIRE_FALSE(p.get_forced_environment());
        p.set_forced_environment(true);
        CATCH_REQUIRE(p.get_forced_environment());
        p.set_forced_environment(false);
        CATCH_REQUIRE_FALSE(p.get_forced_environment());

        CATCH_REQUIRE(p.get_command() == std::string());
        p.set_command("cat");
        CATCH_REQUIRE(p.get_command() == "cat");

        CATCH_REQUIRE(p.get_arguments().empty());
        p.add_argument("/proc/self/comm");
        CATCH_REQUIRE(p.get_arguments().size() == 1);

        CATCH_REQUIRE(p.get_environ().empty());

        CATCH_REQUIRE(p.get_input().empty());
        CATCH_REQUIRE(p.get_binary_input().empty());
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        CATCH_REQUIRE_FALSE(p.get_capture_output());
        p.set_capture_output();
        CATCH_REQUIRE(p.get_capture_output());

        CATCH_REQUIRE(p.get_output().empty());
        CATCH_REQUIRE(p.get_binary_output().empty());
        CATCH_REQUIRE(p.get_output_pipe() == nullptr);
        CATCH_REQUIRE(p.get_next_processes().empty());

        CATCH_REQUIRE(p.get_error().empty());
        CATCH_REQUIRE(p.get_binary_error().empty());
        CATCH_REQUIRE(p.get_error_pipe() == nullptr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(p.get_input().empty());
        CATCH_REQUIRE(p.get_binary_input().empty());
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        CATCH_REQUIRE(p.get_error().empty());
        CATCH_REQUIRE(p.get_binary_error().empty());
        CATCH_REQUIRE(p.get_error_pipe() == nullptr);

        CATCH_REQUIRE(p.get_output() == "cat\n");
        CATCH_REQUIRE(p.get_trimmed_output() == "cat");

        cppprocess::buffer_t const output(p.get_binary_output());
        CATCH_REQUIRE(output.size() == 4);
        CATCH_REQUIRE(output[0] == 'c');
        CATCH_REQUIRE(output[1] == 'a');
        CATCH_REQUIRE(output[2] == 't');
        CATCH_REQUIRE(output[3] == '\n');
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("simple logger, we pipe the input as the message")
    {
        cppprocess::process p("in-logger");

        CATCH_REQUIRE(p.get_name() == "in-logger");

        CATCH_REQUIRE(p.get_command() == std::string());
        p.set_command("logger");
        CATCH_REQUIRE(p.get_command() == "logger");

        CATCH_REQUIRE(p.get_arguments().empty());

        CATCH_REQUIRE(p.get_environ().empty());

        CATCH_REQUIRE(p.get_input().empty());
        CATCH_REQUIRE(p.get_binary_input().empty());
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        p.add_input("Event Dispatcher Process Test\n");

        CATCH_REQUIRE(p.get_input() == std::string("Event Dispatcher Process Test\n"));
        CATCH_REQUIRE(p.get_binary_input().size() == 30);
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        CATCH_REQUIRE_FALSE(p.get_capture_output());
        CATCH_REQUIRE(p.get_output().empty());
        CATCH_REQUIRE(p.get_binary_output().empty());
        CATCH_REQUIRE(p.get_output_pipe() == nullptr);
        CATCH_REQUIRE(p.get_next_processes().empty());

        CATCH_REQUIRE(p.get_error().empty());
        CATCH_REQUIRE(p.get_binary_error().empty());
        CATCH_REQUIRE(p.get_error_pipe() == nullptr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("in | sed | out")
    {
        cppprocess::process p("in-sed-out");

        CATCH_REQUIRE(p.get_name() == "in-sed-out");

        CATCH_REQUIRE_FALSE(p.get_forced_environment());
        p.set_forced_environment(true);
        CATCH_REQUIRE(p.get_forced_environment());
        p.set_forced_environment(false);
        CATCH_REQUIRE_FALSE(p.get_forced_environment());

        CATCH_REQUIRE(p.get_command() == std::string());
        p.set_command("sed");
        CATCH_REQUIRE(p.get_command() == "sed");

        CATCH_REQUIRE(p.get_arguments().empty());
        p.add_argument("-e");
        p.add_argument("s/Hello/Hi/");
        p.add_argument("-");
        CATCH_REQUIRE(p.get_arguments().size() == 3);

        CATCH_REQUIRE(p.get_environ().empty());

        CATCH_REQUIRE(p.get_input().empty());
        CATCH_REQUIRE(p.get_binary_input().empty());
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        p.add_input("Hello  World!\n");

        CATCH_REQUIRE(p.get_input() == std::string("Hello  World!\n"));
        CATCH_REQUIRE(p.get_binary_input().size() == 14);
        CATCH_REQUIRE(p.get_input_pipe() == nullptr);

        CATCH_REQUIRE_FALSE(p.get_capture_output());
        p.set_capture_output();
        CATCH_REQUIRE(p.get_capture_output());

        CATCH_REQUIRE(p.get_output().empty());
        CATCH_REQUIRE(p.get_binary_output().empty());
        CATCH_REQUIRE(p.get_output_pipe() == nullptr);
        CATCH_REQUIRE(p.get_next_processes().empty());

        CATCH_REQUIRE(p.get_error().empty());
        CATCH_REQUIRE(p.get_binary_error().empty());
        CATCH_REQUIRE(p.get_error_pipe() == nullptr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(p.get_output() == "Hi  World!\n");
        CATCH_REQUIRE(p.get_trimmed_output(true) == "Hi World!");

        cppprocess::buffer_t const output(p.get_binary_output());
        CATCH_REQUIRE(output.size() == 11);
        CATCH_REQUIRE(output[ 0] == 'H');
        CATCH_REQUIRE(output[ 1] == 'i');
        CATCH_REQUIRE(output[ 2] == ' ');
        CATCH_REQUIRE(output[ 3] == ' ');
        CATCH_REQUIRE(output[ 4] == 'W');
        CATCH_REQUIRE(output[ 5] == 'o');
        CATCH_REQUIRE(output[ 6] == 'r');
        CATCH_REQUIRE(output[ 7] == 'l');
        CATCH_REQUIRE(output[ 8] == 'd');
        CATCH_REQUIRE(output[ 9] == '!');
        CATCH_REQUIRE(output[10] == '\n');
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("ls unknown-file, expect an error")
    {
        cppprocess::process p("ls-unknown-file");

        CATCH_REQUIRE(p.get_name() == "ls-unknown-file");

        p.set_command("ls");
        CATCH_REQUIRE(p.get_command() == "ls");

        p.add_argument("unknown-file");
        CATCH_REQUIRE(p.get_arguments().size() == 1);

        CATCH_REQUIRE(p.get_environ().empty());

        CATCH_REQUIRE_FALSE(p.get_capture_error());
        p.set_capture_error();
        CATCH_REQUIRE(p.get_capture_error());

        CATCH_REQUIRE(p.get_error().empty());
        CATCH_REQUIRE(p.get_binary_error().empty());
        CATCH_REQUIRE(p.get_error_pipe() == nullptr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code != 0);

        CATCH_REQUIRE(p.get_output().empty());

        CATCH_REQUIRE(!p.get_error().empty());
        // the error message can change under our feet so at this time I
        // don't compare anything
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cat | tr")
    {
        cppprocess::process::pointer_t tr(std::make_shared<cppprocess::process>("tr"));
        tr->set_command("tr");
        tr->add_argument("TASP");
        tr->add_argument("tasp");
        tr->set_capture_output();

        cppprocess::process p("cat");
        p.set_command("cat");
        p.add_argument("-");
        p.add_input("Test A Simple Pipeline\n");
        p.add_next_process(tr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(tr->get_output() == "test a simple pipeline\n");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("file based: cat | tr")
    {
        // Equivalent to:
        //
        //    cat - < input.data | tr TASP tasp > output.data
        //
        std::string & tmpdir(SNAP_CATCH2_NAMESPACE::g_tmp_dir());
        std::string const input_filename(tmpdir + "/input.data");
        std::string const output_filename(tmpdir + "/output.data");
        {
            std::ofstream input_data(input_filename);
            input_data << "Test A Simple Pipeline\n";
        }

        cppprocess::process::pointer_t tr(std::make_shared<cppprocess::process>("tr"));
        tr->set_command("tr");
        tr->add_argument("TASP");
        tr->add_argument("tasp");
        tr->set_output_filename(output_filename);

        // we could directly cat the file here, obviously but we want
        // to test the `< <filename>` functionality
        //
        cppprocess::process p("cat");
        p.set_command("cat");
        p.add_argument("-");
        p.set_input_filename(input_filename);
        p.add_next_process(tr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        snap::file_contents output(output_filename);
        CATCH_REQUIRE(output.read_all());
        CATCH_REQUIRE(output.contents() == "test a simple pipeline\n");
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

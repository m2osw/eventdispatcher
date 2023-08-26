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


// cppprocess
//
#include    <cppprocess/io_capture_pipe.h>
#include    <cppprocess/io_data_pipe.h>
#include    <cppprocess/io_input_file.h>
#include    <cppprocess/io_output_file.h>
#include    <cppprocess/process.h>


// snapdev
//
#include    <snapdev/file_contents.h>


// C++
//
#include    <fstream>


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

        CATCH_REQUIRE(p.get_input_io() == nullptr);
        CATCH_REQUIRE(p.get_output_io() == nullptr);
        CATCH_REQUIRE(p.get_error_io() == nullptr);

        cppprocess::io_capture_pipe::pointer_t capture(std::make_shared<cppprocess::io_capture_pipe>());
        p.set_output_io(capture);
        CATCH_REQUIRE(p.get_output_io() == capture);

        CATCH_REQUIRE(p.get_next_processes().empty());

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(p.get_input_io() == nullptr);
        CATCH_REQUIRE(p.get_output_io() == capture);
        CATCH_REQUIRE(p.get_error_io() == nullptr);

        CATCH_REQUIRE(capture->get_output() == "cat\n");
        CATCH_REQUIRE(capture->get_trimmed_output() == "cat");

        cppprocess::buffer_t const output(capture->get_binary_output());
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

        CATCH_REQUIRE(p.get_input_io() == nullptr);
        CATCH_REQUIRE(p.get_output_io() == nullptr);
        CATCH_REQUIRE(p.get_error_io() == nullptr);

        cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
        CATCH_REQUIRE_FALSE(input->is_writer());
        input->add_input("Event Dispatcher Process Test\n");
        CATCH_REQUIRE(input->is_writer());

        CATCH_REQUIRE(input->get_input() == std::string("Event Dispatcher Process Test\n"));
        CATCH_REQUIRE(input->get_binary_input().size() == 30);

        p.set_input_io(input);
        CATCH_REQUIRE(p.get_input_io() == input);

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

        CATCH_REQUIRE(p.get_input_io() == nullptr);
        CATCH_REQUIRE(p.get_output_io() == nullptr);
        CATCH_REQUIRE(p.get_error_io() == nullptr);

        cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
        CATCH_REQUIRE_FALSE(input->is_writer());
        input->add_input("Hello  World!\n");
        CATCH_REQUIRE(input->is_writer());

        CATCH_REQUIRE(input->get_input() == std::string("Hello  World!\n"));
        CATCH_REQUIRE(input->get_binary_input().size() == 14);

        p.set_input_io(input);
        CATCH_REQUIRE(p.get_input_io() == input);

        cppprocess::io_capture_pipe::pointer_t capture(std::make_shared<cppprocess::io_capture_pipe>());
        p.set_output_io(capture);
        CATCH_REQUIRE(p.get_output_io() == capture);

        CATCH_REQUIRE(capture->get_output().empty());
        CATCH_REQUIRE(capture->get_trimmed_output().empty());
        CATCH_REQUIRE(capture->get_binary_output().empty());
        CATCH_REQUIRE(p.get_next_processes().empty());

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(capture->get_output() == "Hi  World!\n");
        CATCH_REQUIRE(capture->get_trimmed_output(true) == "Hi World!");

        cppprocess::buffer_t const output(capture->get_binary_output());
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

        cppprocess::io_capture_pipe::pointer_t error(std::make_shared<cppprocess::io_capture_pipe>());
        p.set_error_io(error);
        CATCH_REQUIRE(p.get_error_io() == error);

        CATCH_REQUIRE(error->get_output().empty());
        CATCH_REQUIRE(error->get_trimmed_output().empty());
        CATCH_REQUIRE(error->get_binary_output().empty());

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code != 0);

        CATCH_REQUIRE(p.get_output_io() == nullptr);
        CATCH_REQUIRE(p.get_error_io() == error);

        CATCH_REQUIRE_FALSE(error->get_output().empty());
        // the error message can change under our feet so at this time
        // do not compare to a specific message
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("cat | tr")
    {
        cppprocess::process::pointer_t tr(std::make_shared<cppprocess::process>("tr"));
        tr->set_command("tr");
        tr->add_argument("TASP");
        tr->add_argument("tasp");

        cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
        CATCH_REQUIRE_FALSE(input->is_writer());
        input->add_input("Test A Simple Pipeline\n");
        CATCH_REQUIRE(input->is_writer());

        cppprocess::io_capture_pipe::pointer_t capture(std::make_shared<cppprocess::io_capture_pipe>());
        tr->set_output_io(capture);
        CATCH_REQUIRE(tr->get_output_io() == capture);

        cppprocess::process p("cat");
        p.set_command("cat");
        p.add_argument("-");
        p.set_input_io(input);
        CATCH_REQUIRE(p.get_input_io() == input);
        p.add_next_process(tr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        CATCH_REQUIRE(capture->get_output() == "test a simple pipeline\n");
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

        cppprocess::io_output_file::pointer_t output(std::make_shared<cppprocess::io_output_file>(output_filename));
        output->set_truncate(true);
        tr->set_output_io(output);

        // we could directly cat the file here, obviously but we want
        // to test the `< <filename>` functionality
        //
        cppprocess::process p("cat");
        p.set_command("cat");
        p.add_argument("-");

        cppprocess::io_input_file::pointer_t input(std::make_shared<cppprocess::io_input_file>(input_filename));
        p.set_input_io(input);

        p.add_next_process(tr);

        CATCH_REQUIRE(p.start() == 0);

        int const code(p.wait());
        CATCH_REQUIRE(code == 0);

        snapdev::file_contents final_output(output_filename);
        CATCH_REQUIRE(final_output.read_all());
        CATCH_REQUIRE(final_output.contents() == "test a simple pipeline\n");
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

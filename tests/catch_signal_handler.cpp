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

// test standalone header
//
#include    <eventdispatcher/signal_handler.h>


// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/dispatcher.h>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{



int g_expected_sig = -1;

bool signal_handler_callback(
      ed::signal_handler::callback_id_t callback_id
    , int callback_sig
    , siginfo_t const * info
    , ucontext_t const * ucontext)
{
    CATCH_REQUIRE(g_expected_sig == callback_sig);
    g_expected_sig = -1;

std::cerr << "--- signal_handler_callback() was called... id: "
<< callback_id
<< ", sig: " << callback_sig
<< ", info->si_pid: " << info->si_pid
<< ", ucontext->uc_sigmask: " << std::boolalpha << sigismember(&ucontext->uc_sigmask, SIGINT)
<< "\n";

    // call other callbacks
    //
    return true;
}


bool extra_signal_handler_callback(
      ed::signal_handler::callback_id_t callback_id
    , int callback_sig
    , siginfo_t const * info
    , ucontext_t const * ucontext)
{
std::cerr << "--- extra_signal_handler_callback() was called... id: "
<< callback_id
<< ", sig: " << callback_sig
<< ", info->si_pid: " << info->si_pid
<< ", ucontext->uc_sigmask: " << std::boolalpha << sigismember(&ucontext->uc_sigmask, SIGINT)
<< "\n";

    // call other callbacks
    //
    return true;
}


ed::signal_handler::pointer_t get_signal_handler()
{
    static bool created(false);

    if(!created)
    {
        created = true;

        ed::signal_handler::pointer_t sh(ed::signal_handler::create_instance(
              ed::signal_handler::DEFAULT_SIGNAL_TERMINAL
            , ed::signal_handler::DEFAULT_SIGNAL_IGNORE
            , 123
            , SIGTERM
            , &signal_handler_callback));

        CATCH_REQUIRE(sh != nullptr);
        CATCH_REQUIRE(sh == ed::signal_handler::get_instance());
    }

    return ed::signal_handler::get_instance();
}


} // no name namespace



CATCH_TEST_CASE("signal_handler_name", "[signal_handler]")
{
    CATCH_START_SECTION("signal_handler_name: verify signal names")
    {
        for(int sig(-10); sig < 74; ++sig)
        {
            char const * system_name(sigabbrev_np(sig));
            if(system_name == nullptr)
            {
                system_name = "UNKNOWN";
            }

            char const * eventdispatcher_name(ed::signal_handler::get_signal_name(sig));
            if(eventdispatcher_name == nullptr)
            {
                eventdispatcher_name = "UNKNOWN";
            }

//std::cerr << "--- compare sig=" << sig << " -> \"" << system_name << "\" vs \"" << eventdispatcher_name << "\".\n";
            CATCH_CHECK(strcmp(system_name, eventdispatcher_name) == 0);
        }
    }
    CATCH_END_SECTION()
}

CATCH_TEST_CASE("signal_handler", "[signal_handler]")
{
    CATCH_START_SECTION("signal_handler: Create Signal Handler connection")
    {
        ed::signal_handler::pointer_t sh(get_signal_handler());

        // TODO: to test this properly we need to verify that the callback
        //       get called after an add and not called anymore after a
        //       remove
        //
        sh->add_callback(444, SIGILL, &extra_signal_handler_callback);
        sh->remove_callback(444);

        CATCH_CHECK(sh->get_show_stack() == ed::signal_handler::DEFAULT_SHOW_STACK);
        sh->set_show_stack(ed::signal_handler::SIGNAL_INTERRUPT);
        CATCH_CHECK(sh->get_show_stack() == ed::signal_handler::SIGNAL_INTERRUPT);
        sh->set_show_stack(ed::signal_handler::DEFAULT_SHOW_STACK);
        CATCH_CHECK(sh->get_show_stack() == ed::signal_handler::DEFAULT_SHOW_STACK);
    }
    CATCH_END_SECTION()

// Catch2 v3 is compiled with the POSIX signal handler always in place...
// The problem is that each time a test is run, the Catch2 signal is setup
// and then released, so calling get_signal_handler() is already not
// compatible
//
//    CATCH_START_SECTION("signal_handler: Test sending a SIGTERM and see we capture it")
//    {
//        ed::signal_handler::pointer_t sh(get_signal_handler());
//
//        g_expected_sig = SIGTERM;
//        kill(getpid(), SIGTERM);
//        CATCH_REQUIRE(g_expected_sig == -1);
//    }
//    CATCH_END_SECTION()
}


CATCH_TEST_CASE("signal_handler_errors", "[timer][error]")
{
    CATCH_START_SECTION("signal_handler_errors: create_instance() can only be called once")
    {
        ed::signal_handler::pointer_t sh(get_signal_handler());
        CATCH_REQUIRE(sh != nullptr);

        // calling the create_instance() a second time is an error
        //
        CATCH_REQUIRE_THROWS_MATCHES(
              ed::signal_handler::create_instance(
                      ed::signal_handler::DEFAULT_SIGNAL_TERMINAL
                    , ed::signal_handler::DEFAULT_SIGNAL_IGNORE
                    , 321
                    , SIGPIPE
                    , &signal_handler_callback)
            , ed::initialization_error
            , Catch::Matchers::ExceptionMessage(
                  "event_dispatcher_exception: signal_handler::create_instance() must be called once before signal_handler::get_instance() ever gets called."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("signal_handler_errors: invalid signal number & callback pointer")
    {
        ed::signal_handler::pointer_t sh(get_signal_handler());

        CATCH_REQUIRE_THROWS_MATCHES(
              sh->add_callback(555, 123, &extra_signal_handler_callback)
            , ed::invalid_signal
            , Catch::Matchers::ExceptionMessage(
                  "event_dispatcher_exception: signal_handler::add_callback() called with invalid signal number 123."));

        CATCH_REQUIRE_THROWS_MATCHES(
              sh->add_callback(555, SIGBUS, nullptr)
            , ed::invalid_callback
            , Catch::Matchers::ExceptionMessage(
                  "event_dispatcher_exception: signal_handler::add_callback() called with nullptr as the callback."));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et

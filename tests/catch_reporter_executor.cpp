// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// this diagnostic has to be turned off "globally" so the catch2 does not
// generate the warning on the floating point == operator
//
#pragma GCC diagnostic ignored "-Wfloat-equal"

// self
//
#include    "catch_main.h"


// reporter
//
#include    <eventdispatcher/reporter/executor.h>

#include    <eventdispatcher/reporter/parser.h>
#include    <eventdispatcher/reporter/variable_floating_point.h>
#include    <eventdispatcher/reporter/variable_integer.h>
#include    <eventdispatcher/reporter/variable_string.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



// `call()` -- DONE
// `exit()` -- PARTIAL
// `goto()` -- DONE
// `if()` -- DONE
// `label()` -- DONE
// `return()` -- DONE
// `run()` -- DONE
// `sleep()` -- DONE

constexpr char const * const g_program_sleep_func =
    "call(label: func_sleep_1s)\n"
    "exit()\n"
    "label(name: func_sleep_1s)\n"
    "sleep(seconds: 2.5)\n"
    "return()\n"
;

constexpr char const * const g_program_start_thread =
    "set_variable(name: test, value: 33)\n"
    "run()\n"
    "set_variable(name: runner, value: 6.07)\n"
;

constexpr char const * const g_program_start_thread_twice =
    "set_variable(name: test, value: 33)\n"
    "run()\n"
    "set_variable(name: runner, value: 6.07)\n"
    "run()\n" // second run() is forbidden
;

constexpr char const * const g_program_accept_one_message =
    "run()\n"
    "listen(address: <127.0.0.1:20002>)\n"
    "label(name: wait_message)\n"
    "clear_message()\n"
    "wait(timeout: 10.0)\n" // first wait reacts on connect(), second wait receives the REGISTER message
    "has_message()\n"
    "if(false: wait_message)\n"
    "verify_message(command: REGISTER)\n"
    "send_message(command: READY)\n"
    "wait(timeout: 10.0, mode: drain)\n"
    "disconnect()\n"
    "exit()\n"
;



struct expected_trace_t
{
    SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t const
                        f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL;
    char const * const  f_name = nullptr;
};


constexpr expected_trace_t const g_verify_starting_thread[] =
{
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "run",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "run",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_BEFORE_CALL,
        .f_name = "set_variable",
    },
    {
        .f_reason = SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t::CALLBACK_REASON_AFTER_CALL,
        .f_name = "set_variable",
    },
    {}
};


class trace
{
public:
    trace(expected_trace_t const * expected_trace)
        : f_expected_trace(expected_trace)
    {
    }

    trace(trace const &) = delete;

    ~trace()
    {
        // make sure we reached the end of the list
        //
        CATCH_REQUIRE(f_expected_trace[f_pos].f_name == nullptr);
    }

    trace operator = (trace const &) = delete;

    void callback(SNAP_CATCH2_NAMESPACE::reporter::state & s, SNAP_CATCH2_NAMESPACE::reporter::callback_reason_t reason)
    {
        // here we can be in the thread so DO NOT USE CATCH_... macros
        //
        if(f_expected_trace[f_pos].f_name == nullptr)
        {
            throw std::runtime_error(
                  "got more calls ("
                + std::to_string(f_pos + 1)
                + ") to tracer than expected.");
        }

        if(f_expected_trace[f_pos].f_reason != reason)
        {
            throw std::runtime_error(
                  "unexpected reason at position "
                + std::to_string(f_pos)
                + " (got "
                + std::to_string(static_cast<int>(reason))
                + ", expected "
                + std::to_string(static_cast<int>(f_expected_trace[f_pos].f_reason))
                + ").");
        }

        SNAP_CATCH2_NAMESPACE::reporter::statement::pointer_t stmt(s.get_running_statement());
        std::string const & name(stmt->get_instruction()->get_name());
//std::cerr << "--------------------- at pos " << f_pos << " found reason " << static_cast<int>(reason) << " + name " << name << "\n";
        if(f_expected_trace[f_pos].f_name != name)
        {
            throw std::runtime_error(
                  "unexpected instruction at position "
                + std::to_string(f_pos)
                + " (got "
                + name
                + ", expected "
                + f_expected_trace[f_pos].f_name
                + ").");
        }

        ++f_pos;
    }

private:
    int                         f_pos = 0;
    expected_trace_t const *    f_expected_trace = nullptr;
};


class messenger_responder
    : public ed::tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<messenger_responder> pointer_t;

    messenger_responder(
              addr::addr const & a
            , ed::mode_t mode
            , int sequence)
        : tcp_client_permanent_message_connection(
              a
            , mode
            , ed::DEFAULT_PAUSE_BEFORE_RECONNECTING
            , true
            , "responder")  // service name
        , f_sequence(sequence)
    {
        set_name("messenger_responder");    // connection name
    }

    virtual void process_connected() override
    {
        // always register at the time we connect
        //
        tcp_client_permanent_message_connection::process_connected();
        register_service();
    }

    void process_message(ed::message & msg)
    {
        snapdev::NOT_USED(msg);
//std::cerr << "got message! " << msg << "\n";
        remove_from_communicator();
        f_timer->remove_from_communicator();
    }

    void set_timer(ed::connection::pointer_t done_timer)
    {
        f_timer = done_timer;
    }

private:
    // the sequence & step define the next action
    //
    int         f_sequence = 0;
    int         f_step = 0;
    ed::connection::pointer_t
                f_timer = ed::connection::pointer_t();
};


class messenger_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<messenger_timer>        pointer_t;

    messenger_timer(messenger_responder::pointer_t m)
        : timer(10'000'000)
        , f_messenger(m)
    {
        set_name("messenger_timer");
    }

    void process_timeout()
    {
        remove_from_communicator();
        f_messenger->remove_from_communicator();
        f_timed_out = true;
    }

    bool timed_out_prima() const
    {
        return f_timed_out;
    }

private:
    messenger_responder::pointer_t      f_messenger = messenger_responder::pointer_t();
    bool                                f_timed_out = false;
};






} // no name namespace



CATCH_TEST_CASE("reporter_executor", "[executor][reporter]")
{
    CATCH_START_SECTION("verify sleep in a function")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_sleep_func", g_program_sleep_func));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 5);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        snapdev::timespec_ex const start(snapdev::now());
        e->start();
        e->run();
        snapdev::timespec_ex end(snapdev::now());
        end -= start;
        CATCH_REQUIRE(end.tv_sec >= 2); // we slept for 2.5 seconds, so we expect at least start + 2 seconds
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify starting the thread")
    {
        trace tracer(g_verify_starting_thread);

        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread", g_program_start_thread));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());

        // use std::bind() to avoid copies of the tracer object
        //
        s->set_trace_callback(std::bind(&trace::callback, &tracer, std::placeholders::_1, std::placeholders::_2));

        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 3);

        // before we run the script, there are no such variables
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("test"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();

        var = s->get_variable("test");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 33);

        var = s->get_variable("runner");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner");
        CATCH_REQUIRE(var->get_type() == "floating_point");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == 6.07);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("verify starting the thread")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_start_thread_twice", g_program_start_thread_twice));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 4);

        // before we run the script, there are no such variables
        //
        SNAP_CATCH2_NAMESPACE::reporter::variable::pointer_t var(s->get_variable("test"));
        CATCH_REQUIRE(var == nullptr);
        var = s->get_variable("runner");
        CATCH_REQUIRE(var == nullptr);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        e->run();
        CATCH_REQUIRE_THROWS_MATCHES(
              e->stop()
            , std::runtime_error
            , Catch::Matchers::ExceptionMessage(
                      "run() instruction found when already running in the background."));

        var = s->get_variable("test");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "test");
        CATCH_REQUIRE(var->get_type() == "integer");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_integer>(var)->get_integer() == 33);

        var = s->get_variable("runner");
        CATCH_REQUIRE(var != nullptr);
        CATCH_REQUIRE(var->get_name() == "runner");
        CATCH_REQUIRE(var->get_type() == "floating_point");
        CATCH_REQUIRE(std::static_pointer_cast<SNAP_CATCH2_NAMESPACE::reporter::variable_floating_point>(var)->get_floating_point() == 6.07);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("reporter_executor_message", "[executor][reporter]")
{
    CATCH_START_SECTION("send/receive one message")
    {
        SNAP_CATCH2_NAMESPACE::reporter::lexer::pointer_t l(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::lexer>("program_accept_one_message", g_program_accept_one_message));
        SNAP_CATCH2_NAMESPACE::reporter::state::pointer_t s(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::state>());
        SNAP_CATCH2_NAMESPACE::reporter::parser::pointer_t p(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::parser>(l, s));
        p->parse_program();

        CATCH_REQUIRE(s->get_statement_size() == 12);

        SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
        e->start();
        addr::addr a;
        sockaddr_in ip = {
            .sin_family = AF_INET,
            .sin_port = htons(20002),
            .sin_addr = {
                .s_addr = htonl(0x7f000001),
            },
            .sin_zero = {},
        };
        a.set_ipv4(ip);
        messenger_responder::pointer_t messenger(std::make_shared<messenger_responder>(a, ed::mode_t::MODE_PLAIN, 1));
        ed::communicator::instance()->add_connection(messenger);
        messenger_timer::pointer_t timer(std::make_shared<messenger_timer>(messenger));
        ed::communicator::instance()->add_connection(timer);
        messenger->set_timer(timer);

        e->run();

        // if we exited because of our timer, then the test did not pass
        //
        CATCH_REQUIRE_FALSE(timer->timed_out_prima());
    }
    CATCH_END_SECTION()
}


//CATCH_TEST_CASE("reporter_executor_error", "[executor][reporter][error]")
//{
//    CATCH_START_SECTION("statement without instruction")
//    {
//    }
//    CATCH_END_SECTION()
//}


// vim: ts=4 sw=4 et

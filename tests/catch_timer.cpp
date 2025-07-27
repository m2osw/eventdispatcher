// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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

// test standalone header
//
#include    <eventdispatcher/timer.h>


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



// use a timer to test various general connection function
//
class timer_test
    : public ed::timer
{
public:
    typedef std::shared_ptr<timer_test>        pointer_t;

                    timer_test();

    virtual void    process_timeout() override;
    virtual void    connection_added() override;
    virtual void    connection_removed() override;

    void            set_expect_timeout(bool expect_timeout);
    void            set_expect_add(bool expect_add);
    bool            get_expect_add() const;
    void            set_expect_remove(bool expect_remove);
    bool            get_expect_remove() const;

private:
    bool            f_expect_timeout = false;
    bool            f_expect_add = false;
    bool            f_expect_remove = false;
};



timer_test::timer_test()
    : timer(1'000'000)  // 1s
{
    set_name("timer");
}


void timer_test::process_timeout()
{
    if(!f_expect_timeout)
    {
        throw std::runtime_error("unexpectedly got process_timeout() called.");
    }
    f_expect_timeout = false;

    remove_from_communicator();
}


void timer_test::connection_added()
{
    if(!f_expect_add)
    {
        throw std::runtime_error("unexpectedly got added to communicator.");
    }
    f_expect_add = false;

    connection::connection_added();
}


void timer_test::connection_removed()
{
    if(!f_expect_remove)
    {
        throw std::runtime_error("unexpectedly got removed to communicator.");
    }
    f_expect_remove = false;

    connection::connection_removed();
}


void timer_test::set_expect_timeout(bool expect_timeout)
{
    f_expect_timeout = expect_timeout;
}


void timer_test::set_expect_add(bool expect_add)
{
    f_expect_add = expect_add;
}


bool timer_test::get_expect_add() const
{
    return f_expect_add;
}


void timer_test::set_expect_remove(bool expect_remove)
{
    f_expect_remove = expect_remove;
}


bool timer_test::get_expect_remove() const
{
    return f_expect_remove;
}



} // no name namespace



CATCH_TEST_CASE("timer", "[timer]")
{
    CATCH_START_SECTION("Timer connection")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        // pretend we add a timer, nullptr is ignored
        //
        CATCH_REQUIRE_FALSE(communicator->add_connection(timer_test::pointer_t()));
        CATCH_REQUIRE(communicator->get_connections().empty());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        CATCH_REQUIRE(t->get_name() == "timer");

        t->set_name("my-timer");
        CATCH_REQUIRE(t->get_name() == "my-timer");

        CATCH_REQUIRE_FALSE(t->is_listener());
        CATCH_REQUIRE_FALSE(t->is_signal());
        CATCH_REQUIRE_FALSE(t->is_reader());
        CATCH_REQUIRE_FALSE(t->is_writer());
        CATCH_REQUIRE(t->get_socket() == -1);
        CATCH_REQUIRE(t->valid_socket());

        CATCH_REQUIRE(t->is_enabled());
        t->set_enable(false);
        CATCH_REQUIRE_FALSE(t->is_enabled());
        t->set_enable(true);
        CATCH_REQUIRE(t->is_enabled());

        CATCH_REQUIRE(t->get_priority() == ed::EVENT_DEFAULT_PRIORITY);
        t->set_priority(33);
        CATCH_REQUIRE(t->get_priority() == 33);

        // make sure the sorting works as expected
        {
            timer_test::pointer_t t2(std::make_shared<timer_test>());
            CATCH_REQUIRE(ed::connection::compare(t, t2));
            CATCH_REQUIRE_FALSE(ed::connection::compare(t2, t));

            t->set_priority(145);
            CATCH_REQUIRE(t->get_priority() == 145);

            CATCH_REQUIRE_FALSE(ed::connection::compare(t, t2));
            CATCH_REQUIRE(ed::connection::compare(t2, t));
        }

        CATCH_REQUIRE(t->get_event_limit() == 5); // TODO: make 5 a constant
        t->set_event_limit(10);
        CATCH_REQUIRE(t->get_event_limit() == 10);

        CATCH_REQUIRE(t->get_processing_time_limit() == 500'000); // TODO: make 500_000 a constant
        t->set_processing_time_limit(1'200'999);
        CATCH_REQUIRE(t->get_processing_time_limit() == 1'200'999);

        CATCH_REQUIRE(t->get_timeout_delay() == 1'000'000);
        t->set_timeout_delay(5'000'000);
        CATCH_REQUIRE(t->get_timeout_delay() == 5'000'000);
        snapdev::timespec_ex const duration(11, 345'678'183);
        t->set_timeout_delay(duration);
        CATCH_REQUIRE(t->get_timeout_delay() == 11'345'678);

        snapdev::timespec_ex const date(snapdev::now() + snapdev::timespec_ex(30, 500'000'000));
        t->set_timeout_date(date);
        CATCH_REQUIRE(t->get_timeout_date() == date.to_usec());

        // make sure it works even though it does nothing
        //
        t->non_blocking();
        t->keep_alive();

        CATCH_REQUIRE_FALSE(t->is_done());
        t->mark_done();
        CATCH_REQUIRE(t->is_done());
        t->mark_not_done();
        CATCH_REQUIRE_FALSE(t->is_done());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("Timer add/remove connection")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        t->set_expect_add(true);
        CATCH_REQUIRE(communicator->add_connection(t));
        CATCH_REQUIRE_FALSE(t->get_expect_add());
        CATCH_REQUIRE_FALSE(communicator->add_connection(t)); // duplicate is ignored
        ed::connection::vector_t const & connections(communicator->get_connections());
        CATCH_REQUIRE(connections.size() == 1);
        CATCH_REQUIRE(connections[0] == t);

        t->set_expect_remove(true);
        communicator->remove_connection(t);
        CATCH_REQUIRE_FALSE(t->get_expect_remove());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("Timer add connection, remove on process_error()")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        t->set_expect_add(true);
        CATCH_REQUIRE(communicator->add_connection(t));
        CATCH_REQUIRE_FALSE(t->get_expect_add());

        t->set_expect_remove(true);
        t->process_error();
        CATCH_REQUIRE_FALSE(t->get_expect_remove());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("Timer add connection, expect process_timeout()")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        t->set_expect_add(true);
        CATCH_REQUIRE(communicator->add_connection(t));
        CATCH_REQUIRE_FALSE(t->get_expect_add());

        snapdev::timespec_ex const start(snapdev::now());
        t->set_expect_timeout(true);
        t->set_expect_remove(true);
        communicator->run();
        t->set_expect_timeout(false);
        CATCH_REQUIRE_FALSE(t->get_expect_remove());
        snapdev::timespec_ex const end(snapdev::now());
        snapdev::timespec_ex const duration(end - start);
        CATCH_REQUIRE(duration.tv_sec >= 1);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("Timer add connection, remove on process_hup()")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        t->set_expect_add(true);
        CATCH_REQUIRE(communicator->add_connection(t));
        CATCH_REQUIRE_FALSE(t->get_expect_add());

        t->set_expect_remove(true);
        t->process_hup();
        CATCH_REQUIRE_FALSE(t->get_expect_remove());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("Timer add connection, remove on process_invalid()")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        timer_test::pointer_t t(std::make_shared<timer_test>());

        t->set_expect_add(true);
        CATCH_REQUIRE(communicator->add_connection(t));
        CATCH_REQUIRE_FALSE(t->get_expect_add());

        t->set_expect_remove(true);
        t->process_invalid();
        CATCH_REQUIRE_FALSE(t->get_expect_remove());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("timer_errors", "[timer][error]")
{
    CATCH_START_SECTION("timer: invalid priority (too small)")
    {
        timer_test::pointer_t t(std::make_shared<timer_test>());
        for(ed::priority_t p(ed::EVENT_MIN_PRIORITY - 100); p < ed::EVENT_MIN_PRIORITY; ++p)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  t->set_priority(p)
                , ed::parameter_error
                , Catch::Matchers::ExceptionMessage(
                      "parameter_error: connection::set_priority(): priority out of range, this instance of connection accepts priorities between "
                    + std::to_string(ed::EVENT_MIN_PRIORITY)
                    + " and "
                    + std::to_string(ed::EVENT_MAX_PRIORITY)
                    + "."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("timer: invalid priority (too large)")
    {
        timer_test::pointer_t t(std::make_shared<timer_test>());
        for(ed::priority_t p(ed::EVENT_MAX_PRIORITY + 1); p < ed::EVENT_MAX_PRIORITY + 100; ++p)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  t->set_priority(p)
                , ed::parameter_error
                , Catch::Matchers::ExceptionMessage(
                      "parameter_error: connection::set_priority(): priority out of range, this instance of connection accepts priorities between "
                    + std::to_string(ed::EVENT_MIN_PRIORITY)
                    + " and "
                    + std::to_string(ed::EVENT_MAX_PRIORITY)
                    + "."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("timer: invalid timeout delay (too small)")
    {
        timer_test::pointer_t t(std::make_shared<timer_test>());
        for(std::int64_t us(-100); us < 10; ++us)
        {
            if(us == -1)
            {
                // -1 is similar to turning off the timer
                //
                continue;
            }

            CATCH_REQUIRE_THROWS_MATCHES(
                  t->set_timeout_delay(us)
                , ed::parameter_error
                , Catch::Matchers::ExceptionMessage(
                      "parameter_error: connection::set_timeout_delay(): timeout_us parameter cannot be less than 10 unless it is exactly -1, "
                    + std::to_string(us)
                    + " is not valid."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("timer: invalid timeout date (too small)")
    {
        timer_test::pointer_t t(std::make_shared<timer_test>());
        for(std::int64_t date(-100); date < -1; ++date)
        {
            CATCH_REQUIRE_THROWS_MATCHES(
                  t->set_timeout_date(date)
                , ed::parameter_error
                , Catch::Matchers::ExceptionMessage(
                      "parameter_error: connection::set_timeout_date(): date_us parameter cannot be less than -1, "
                    + std::to_string(date)
                    + " is not valid."));
        }
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et

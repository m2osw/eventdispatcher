// Copyright (c) 2012-2026  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/communicator.h>


// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/tcp_client_buffer_connection.h>
#include    <eventdispatcher/tcp_server_client_buffer_connection.h>
#include    <eventdispatcher/tcp_server_connection.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// C
//
//#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{



// create a TCP buffer connection so we can verify that processing
// happens as expected
//
class line_test
    : public ed::tcp_client_buffer_connection
{
public:
    typedef std::shared_ptr<line_test>          pointer_t;

                    line_test(addr::addr const & address);
                    ~line_test();

    virtual void    connection_added() override;
    virtual void    connection_removed() override;

    // tcp_client_buffer_connection implementation
    //
    virtual void    process_line(std::string const & line);

    void            set_expect_add(bool expect_add);
    bool            get_expect_add() const;
    void            set_expect_remove(bool expect_remove);
    bool            get_expect_remove() const;

private:
    bool            f_expect_add = false;
    bool            f_expect_remove = false;
};


class line_server;


class line_server_client
    : public ed::tcp_server_client_buffer_connection
{
public:
    typedef std::shared_ptr<line_server_client>        pointer_t;

                            line_server_client(ed::tcp_bio_client::pointer_t c, line_server * server);
                            line_server_client(line_server_client const & rhs) = delete;
    line_server_client &    operator = (line_server_client const & rhs) = delete;

    // connection implementation
    //
    virtual void            process_hup() override;
    virtual void            process_line(std::string const & line) override;

private:
    line_server *           f_server = nullptr;
};


class line_server
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<line_server>        pointer_t;

    enum class test_t
    {
        TEST_ONE_LINE,      // this sends just one line, no yielding happening
        TEST_THREE_LINES,   // this sends 3 lines, the main process yields 3 times before stopping
    };

                    line_server(addr::addr const & address, test_t test);
                    ~line_server();

    line_server::test_t
                    get_test() const;
    void            done();

    // connection implementation
    //
    virtual void    process_accept() override;

private:
    test_t          f_test = test_t::TEST_ONE_LINE;
};








line_test::line_test(addr::addr const & address)
    : tcp_client_buffer_connection(
          address
        , ed::mode_t::MODE_PLAIN
        , false
        , ed::tcp_bio_options())
{
    set_name("line_test");
    set_event_limit(1);     // this is what will trigger the yield
    std::cout << "--- client ready..." << std::endl;
}


line_test::~line_test()
{
    std::cout << "-- line_test() deleted!" << std::endl;
}


void line_test::connection_added()
{
    if(!f_expect_add)
    {
        throw std::runtime_error("unexpectedly got added to communicator.");
    }
    f_expect_add = false;

    connection::connection_added();
}


void line_test::connection_removed()
{
    if(!f_expect_remove)
    {
        throw std::runtime_error("unexpectedly got removed from communicator.");
    }
    f_expect_remove = false;

    std::cout << "--- removing client connection..." << std::endl;
    connection::connection_removed();
    std::cout << "--- client connection removed..." << std::endl;
}


void line_test::process_line(std::string const & line)
{
    std::cout << "--- got line: [" << line << "]" << std::endl;

    if(line == "DONE")
    {
        // signal telling us we're done
        //
        f_expect_remove = true;
        ed::communicator::instance()->remove_connection(shared_from_this());
        std::cout << "--- remove_connection() returned..." << std::endl;
    }
    std::cout << "--- now return from process_line()?" << std::endl;
}


void line_test::set_expect_add(bool expect_add)
{
    f_expect_add = expect_add;
}


bool line_test::get_expect_add() const
{
    return f_expect_add;
}


void line_test::set_expect_remove(bool expect_remove)
{
    f_expect_remove = expect_remove;
}


bool line_test::get_expect_remove() const
{
    return f_expect_remove;
}





line_server_client::line_server_client(ed::tcp_bio_client::pointer_t c, line_server * server)
    : tcp_server_client_buffer_connection(c)
    , f_server(server)
{
    set_name("line_server_client");
    switch(f_server->get_test())
    {
    case line_server::test_t::TEST_ONE_LINE:
        {
            char const * done("DONE\n");
            write(done, strlen(done));
        }
        break;

    case line_server::test_t::TEST_THREE_LINES:
        {
            char const * three("START\nGOING\nDONE\n");
            write(three, strlen(three));
        }
        break;

    }
    std::cerr << "-- sent message(s)..." << std::endl;
}


//void line_server_client::process_write()
//{
//}


void line_server_client::process_hup()
{
    ed::tcp_server_client_buffer_connection::process_hup();

    // make sure the server is gone too
    //
    f_server->done();
}


void line_server_client::process_line(std::string const & line)
{
    std::cout << "-- received unexpected line [" << line << "]\n";
    CATCH_FAIL("process line from line_server_client called unexpectedly.");
}





line_server::line_server(addr::addr const & address, test_t test)
    : tcp_server_connection(address, std::string(), std::string())
    , f_test(test)
{
    set_name("line_server");
    std::cout << "--- server ready..." << std::endl;
}


line_server::~line_server()
{
}


line_server::test_t line_server::get_test() const
{
    return f_test;
}


void line_server::done()
{
    ed::communicator::instance()->remove_connection(shared_from_this());
}


void line_server::process_accept()
{
    std::cerr << "-- received connection..." << std::endl;
    ed::tcp_bio_client::pointer_t s(accept());
    CATCH_REQUIRE(s != nullptr);

    line_server_client::pointer_t server_client(std::make_shared<line_server_client>(s, this));
    CATCH_REQUIRE(server_client != nullptr);

    ed::communicator::instance()->add_connection(server_client);
}



} // no name namespace



CATCH_TEST_CASE("yield", "[yield][process_line]")
{
    CATCH_START_SECTION("yield: verify without yield")
    {
        std::cerr << "-- test started..." << std::endl;
        ed::communicator::pointer_t communicator(ed::communicator::instance());
        CATCH_REQUIRE(communicator->get_connections().empty());

        // create server to listen for connections
        //
        std::cerr << "-- created server..." << std::endl;
        addr::addr const server_address(addr::string_to_addr("127.0.0.1:0"));
        line_server::pointer_t s(std::make_shared<line_server>(server_address, line_server::test_t::TEST_ONE_LINE));
        CATCH_REQUIRE(s->get_name() == "line_server");
        communicator->add_connection(s);

        // create client which connects that server
        //
        {
            std::cerr << "-- created client..." << std::endl;
            addr::addr const client_address(s->get_address());
            line_test::pointer_t l(std::make_shared<line_test>(client_address));
            CATCH_REQUIRE(l->get_name() == "line_test");
            l->set_expect_add(true);
            communicator->add_connection(l);
            CATCH_REQUIRE_FALSE(l->get_expect_add());
        }

        // enter processing loop
        //
        std::cerr << "-- start loop..." << std::endl;
        communicator->run();
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("yield: verify with yield")
    {
        std::cerr << "-- test started..." << std::endl;
        ed::communicator::pointer_t communicator(ed::communicator::instance());
        CATCH_REQUIRE(communicator->get_connections().empty());

        // create server to listen for connections
        //
        std::cerr << "-- created server..." << std::endl;
        addr::addr const server_address(addr::string_to_addr("127.0.0.1:0"));
        line_server::pointer_t s(std::make_shared<line_server>(server_address, line_server::test_t::TEST_THREE_LINES));
        CATCH_REQUIRE(s->get_name() == "line_server");
        communicator->add_connection(s);

        // create client which connects that server
        //
        {
            std::cerr << "-- created client..." << std::endl;
            addr::addr const client_address(s->get_address());
            line_test::pointer_t l(std::make_shared<line_test>(client_address));
            CATCH_REQUIRE(l->get_name() == "line_test");
            l->set_expect_add(true);
            communicator->add_connection(l);
            CATCH_REQUIRE_FALSE(l->get_expect_add());
        }

        // enter processing loop
        //
        std::cerr << "-- start loop..." << std::endl;
        communicator->run();
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et

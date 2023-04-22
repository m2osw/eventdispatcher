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


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/local_stream_client_permanent_message_connection.h>
#include    <eventdispatcher/local_stream_server_client_message_connection.h>
#include    <eventdispatcher/local_stream_server_connection.h>
#include    <eventdispatcher/dispatcher.h>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



class unix_server;



class unix_client
    : public ed::local_stream_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<unix_client>        pointer_t;

                    unix_client(addr::addr_unix const & address);

    void            send_hello();
    void            msg_hi(ed::message & msg);

private:
    ed::dispatcher::pointer_t
                    f_dispatcher = ed::dispatcher::pointer_t();
};


class unix_server_client
    : public ed::local_stream_server_client_message_connection
{
public:
    typedef std::shared_ptr<unix_server_client>        pointer_t;

                            unix_server_client(snapdev::raii_fd_t s, unix_server * server);
                            unix_server_client(unix_server_client const & rhs) = delete;
    unix_server_client &    operator = (unix_server_client const & rhs) = delete;

    void                    msg_hello(ed::message & msg);
    void                    msg_down(ed::message & msg);

    // connection implementation
    //
    void                    process_hup();

private:
    unix_server *           f_server = nullptr;
    ed::dispatcher::pointer_t
                            f_dispatcher = ed::dispatcher::pointer_t();
};


class unix_server
    : public ed::local_stream_server_connection
{
public:
    typedef std::shared_ptr<unix_server>        pointer_t;

                    unix_server(addr::addr_unix const & address);
                    ~unix_server();

    void            done();

    // connection implementation
    //
    virtual void    process_accept() override;

private:
};











unix_client::unix_client(addr::addr_unix const & address)
    : local_stream_client_permanent_message_connection(address)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("unix-client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("HI", &unix_client::msg_hi),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
}


void unix_client::send_hello()
{
    // send the HELLO message, since we're not going to be connected yet
    // we ask for the permanent connection to cache the message
    //
    ed::message hello;
    hello.set_command("HELLO");
    send_message(hello, true);
}


void unix_client::msg_hi(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "HI");

    ed::message down;
    down.set_command("DOWN");
    send_message(down);

    mark_done(true);
    //ed::communicator::instance()->remove_connection(shared_from_this());
}





unix_server_client::unix_server_client(snapdev::raii_fd_t s, unix_server * server)
    : local_stream_server_client_message_connection(std::move(s))
    , f_server(server)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("unix-server-client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("HELLO", &unix_server_client::msg_hello),
        DISPATCHER_MATCH("DOWN",  &unix_server_client::msg_down),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
}


void unix_server_client::msg_hello(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "HELLO");
    snapdev::NOT_USED(msg);

    ed::message hi;
    hi.set_command("HI");
    send_message(hi);
}


void unix_server_client::msg_down(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "DOWN");

    ed::communicator::instance()->remove_connection(shared_from_this());
}


void unix_server_client::process_hup()
{
    // make sure the server is gone too
    f_server->done();
}








unix_server::unix_server(addr::addr_unix const & address)
    : local_stream_server_connection(address)
{
}


unix_server::~unix_server()
{
}


void unix_server::done()
{
    ed::communicator::instance()->remove_connection(shared_from_this());
}


void unix_server::process_accept()
{
    snapdev::raii_fd_t s(accept());
    CATCH_REQUIRE(s != nullptr);

    unix_server_client::pointer_t server_client(std::make_shared<unix_server_client>(std::move(s), this));
    CATCH_REQUIRE(server_client != nullptr);

    ed::communicator::instance()->add_connection(server_client);
}






} // no name namespace



CATCH_TEST_CASE("local_stream_messaging", "[local-stream]")
{
    CATCH_START_SECTION("Create a Server, Client, Connect & Send Messages")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        std::string name("test-unix-stream");
        unlink(name.c_str());
        addr::addr_unix server_address(name);
        unix_server::pointer_t server(std::make_shared<unix_server>(server_address));
        communicator->add_connection(server);

        addr::addr_unix client_address(name);
        unix_client::pointer_t client(std::make_shared<unix_client>(client_address));
        communicator->add_connection(client);

        client->send_hello();

        communicator->run();
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

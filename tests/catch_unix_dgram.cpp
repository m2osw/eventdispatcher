// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/local_dgram_server_message_connection.h>


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



class unix_dgram_server;



// to receive datagram, we need to create a server, so event the client
// is a server... (if you want back and forth communication over datagram
// that is)
//
class unix_dgram_client
    : public ed::local_dgram_server_message_connection
{
public:
    typedef std::shared_ptr<unix_dgram_client>        pointer_t;

                    unix_dgram_client(addr::addr_unix const & address);

    void            set_server_address(addr::addr_unix const & server_address);

    void            send_hello();
    void            msg_hi(ed::message & msg);
    void            msg_reply_with_unknown(ed::message & msg);

private:
    ed::dispatcher::pointer_t
                    f_dispatcher = ed::dispatcher::pointer_t();
    addr::addr_unix f_server_address = addr::addr_unix();
};




class unix_dgram_server
    : public ed::local_dgram_server_message_connection
{
public:
    typedef std::shared_ptr<unix_dgram_server>        pointer_t;

                    unix_dgram_server(addr::addr_unix const & address);
                    ~unix_dgram_server();

    void            set_client_address(addr::addr_unix const & server_address);
    void            done();

    void            msg_hello(ed::message & msg);
    void            msg_down(ed::message & msg);
    void            msg_reply_with_unknown(ed::message & msg);

    // connection implementation
    //
    //virtual void    process_accept() override;

private:
    ed::dispatcher::pointer_t
                    f_dispatcher = ed::dispatcher::pointer_t();
    addr::addr_unix f_client_address = addr::addr_unix();
};











unix_dgram_client::unix_dgram_client(addr::addr_unix const & address)
    : local_dgram_server_message_connection(
              address
            , false
            , true
            , true)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("unix-dgram-client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("HI",  &unix_dgram_client::msg_hi),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
}


void unix_dgram_client::set_server_address(addr::addr_unix const & server_address)
{
    f_server_address = server_address;
}


void unix_dgram_client::send_hello()
{
    // send the HELLO message, since we're not going to be connected yet
    // we ask for the permanent connection to cache the message
    //
    ed::message hello;
    hello.set_command("HELLO");
    send_message(f_server_address, hello);
}


void unix_dgram_client::msg_hi(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "HI");

    ed::message down;
    down.set_command("DOWN");
    send_message(f_server_address, down);

    // we can immediately remove the connection since the send_message()
    // is immediate in case of UDP
    //
    ed::communicator::instance()->remove_connection(shared_from_this());
}


void unix_dgram_client::msg_reply_with_unknown(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}













unix_dgram_server::unix_dgram_server(addr::addr_unix const & address)
    : local_dgram_server_message_connection(
              address
            , false
            , true
            , true)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("unix-dgram-server");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("HELLO", &unix_dgram_server::msg_hello),
        DISPATCHER_MATCH("DOWN",  &unix_dgram_server::msg_down),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
}


unix_dgram_server::~unix_dgram_server()
{
}


void unix_dgram_server::set_client_address(addr::addr_unix const & server_address)
{
    f_client_address = server_address;
}


void unix_dgram_server::done()
{
    ed::communicator::instance()->remove_connection(shared_from_this());
}


void unix_dgram_server::msg_hello(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "HELLO");
    snapdev::NOT_USED(msg);

    ed::message hi;
    hi.set_command("HI");
    send_message(f_client_address, hi);
}


void unix_dgram_server::msg_down(ed::message & msg)
{
    CATCH_REQUIRE(msg.get_command() == "DOWN");

    ed::communicator::instance()->remove_connection(shared_from_this());
}


void unix_dgram_server::msg_reply_with_unknown(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}






} // no name namespace



CATCH_TEST_CASE("local_dgram_messaging", "[local-dgram]")
{
    CATCH_START_SECTION("Create a Server, Client, Connect & Send Messages")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        std::string server_name("test-unix-dgram-server");
        unlink(server_name.c_str());
        addr::addr_unix server_address(server_name);

        std::string client_name("test-unix-dgram-client");
        unlink(client_name.c_str());
        addr::addr_unix client_address(client_name);

        unix_dgram_server::pointer_t server(std::make_shared<unix_dgram_server>(server_address));
        server->set_client_address(client_address);
        communicator->add_connection(server);

        unix_dgram_client::pointer_t client(std::make_shared<unix_dgram_client>(client_address));
        client->set_server_address(server_address);
        communicator->add_connection(client);

        client->send_hello();

        communicator->run();
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

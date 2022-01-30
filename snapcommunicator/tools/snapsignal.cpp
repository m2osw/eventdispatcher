// Snap Websites Server -- to send UDP signals to backends
// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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


// snapwebsites lib
//
#include <snapwebsites/snapwebsites.h>
#include <snapwebsites/snap_config.h>


// snapdev lib
//
#include <snapdev/not_reached.h>


// last include
//
#include <snapdev/poison.h>




int main(int argc, char *argv[])
{
    try
    {
        // create a server object
        snap::server::pointer_t s( snap::server::instance() );
        s->setup_as_backend();

        // parse the command line arguments (this also brings in the .conf params)
        s->config(argc, argv);

        // Now create the qt application instance
        //
        s->prepare_qtapp(argc, argv);

        // get the message (Excuse the naming convension...)
        //
        QString const msg(s->get_parameter("__BACKEND_URI"));

        // a UDP message can include a secret code, by default it is going
        // to use the one defined in /etc/snapwebsites/snapcommunicator.conf
        //
        std::string secret_code(s->get_parameter("SECRETCODE").toUtf8().data());      // -p SECRETCODE=123
        if(secret_code.empty())
        {
            snap::snap_config const config("snapcommunicator");
            secret_code = config.get_parameter("signal_secret");
        }

        // the message is expected to be a complete message as defined in
        // our snap_communicator system, something like:
        //
        //    <service>/<COMMAND> param=value;...
        //
        snap::snap_communicator_message message;
        message.from_message(msg);

        // get the snap communicator signal IP and port (UDP)
        //
        QString addr("127.0.0.1");
        int port(4041);
        snap::snap_config config("snapcommunicator");
        tcp_client_server::get_addr_port(QString::fromUtf8(config.get_parameter("signal").c_str()), addr, port, "udp");

        // now send the message
        //
        snap::snap_communicator::snap_udp_server_message_connection::send_message(
                      addr.toUtf8().data()
                    , port
                    , message
                    , secret_code);

        // exit via the server so the server can clean itself up properly
        s->exit(0);
        snap::NOT_REACHED();

        return 0;
    }
    catch(std::exception const & e)
    {
        // clean error on exception
        std::cerr << "snapsignal: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et

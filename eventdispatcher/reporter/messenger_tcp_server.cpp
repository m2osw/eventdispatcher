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

// self
//
#include    "messenger_tcp_server.h"

#include    "messenger_tcp_client.h"
#include    "state.h"


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



messenger_tcp_server::messenger_tcp_server(
          state * s
        , addr::addr const & a)
    : tcp_server_connection(a, std::string(), std::string())
    , f_state(s)
{
}


void messenger_tcp_server::process_accept()
{
    ed::tcp_bio_client::pointer_t client(accept());
    if(client == nullptr)
    {
        throw std::runtime_error("accept() failed to return a pointer.");
    }

    messenger_tcp_client::pointer_t service(std::make_shared<messenger_tcp_client>(f_state, client));
    f_state->add_connection(service);
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

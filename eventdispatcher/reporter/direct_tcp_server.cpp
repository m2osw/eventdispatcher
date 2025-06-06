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

// self
//
#include    "direct_tcp_server.h"

#include    "direct_tcp_server_client.h"
#include    "state.h"


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



direct_tcp_server::direct_tcp_server(
          state * s
        , addr::addr const & a)
    : tcp_server_connection(a, std::string(), std::string())
    , f_state(s)
{
    set_name("drct_tcp_server");
    keep_alive();
}


void direct_tcp_server::process_accept()
{
    // make sure lower level has a chance to capture the event
    //
    tcp_server_connection::process_accept();

    ed::tcp_bio_client::pointer_t client(accept());
    if(client == nullptr)
    {
        throw std::runtime_error("accept() failed to return a pointer."); // LCOV_EXCL_LINE
    }

    direct_tcp_server_client::pointer_t service(std::make_shared<direct_tcp_server_client>(f_state, client));
    f_state->add_connection(service);
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

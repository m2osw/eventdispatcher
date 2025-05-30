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
#include    "direct_tcp_server_client.h"

#include    "state.h"


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



direct_tcp_server_client::direct_tcp_server_client(
          state * s
        , ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_connection(client)
    , f_state(s)
{
    set_name("drct_tcp_client");
    non_blocking();
}


void direct_tcp_server_client::process_read()
{
    // read one buffer at a time and save it in the state
    //
    if(valid_socket())
    {
        connection_data_t::value_type buf[1024 * 4];
        for(;;)
        {
            errno = 0;
            ssize_t const r(read(buf, sizeof(buf)));
            if(r > 0)
            {
                connection_data_pointer_t data(std::make_shared<connection_data_t>(buf, buf + r));
                f_state->add_data(data);
            }
            else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                //
                break;
            }
            else //if(r < 0)
            {
                // LCOV_EXCL_START
                // TODO: do something about the error
                //
                int const e(errno);
                SNAP_LOG_ERROR
                    << "an error occurred while reading from socket (errno: "
                    << e
                    << " -- "
                    << strerror(e)
                    << ")."
                    << SNAP_LOG_SEND;
                process_error();
                return;
                // LCOV_EXCL_STOP
            }
        }
    }

    // process next level too
    //
    tcp_server_client_connection::process_read();
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

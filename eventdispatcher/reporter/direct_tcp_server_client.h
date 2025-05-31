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
#pragma once

// eventdispatcher
//
#include    <eventdispatcher/tcp_server_client_connection.h>


// snapcatch2
//
#include    <catch2/snapcatch2.hpp>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class state;


class direct_tcp_server_client
    : public ed::tcp_server_client_connection
{
public:
    typedef std::shared_ptr<direct_tcp_server_client>        pointer_t;

                            direct_tcp_server_client(
                                  state * s
                                , ed::tcp_bio_client::pointer_t client);
                            direct_tcp_server_client(direct_tcp_server_client const &) = delete;

    direct_tcp_server_client &
                            operator = (direct_tcp_server_client const &) = delete;

    // tcp_server_client_connection implementation
    virtual void            process_read() override;

private:
    state *                 f_state = nullptr;
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

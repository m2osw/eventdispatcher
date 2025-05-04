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
#pragma once

// eventdispatcher
//
#include    <eventdispatcher/tcp_server_connection.h>


// snapcatch2
//
#include    <catch2/snapcatch2.hpp>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class state;


class direct_tcp_server
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<direct_tcp_server>        pointer_t;

                            direct_tcp_server(
                                  state * s
                                , addr::addr const & a);
                            direct_tcp_server(direct_tcp_server const &) = delete;
    direct_tcp_server &     operator = (direct_tcp_server const &) = delete;

    // ed::connection implementation
    virtual void            process_accept() override;

private:
    state *                 f_state = nullptr;
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

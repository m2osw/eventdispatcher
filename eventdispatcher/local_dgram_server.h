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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    <eventdispatcher/local_dgram_base.h>



namespace ed
{



class local_dgram_server
    : public local_dgram_base
{
public:
    typedef std::shared_ptr<local_dgram_server>     pointer_t;

                        local_dgram_server(
                                  addr::unix const & address
                                , bool sequential
                                , bool close_on_exec
                                , bool force_reuse_addr);

    int                 recv(char * msg, size_t max_size);
    int                 timed_recv(char * msg, size_t const max_size, int const max_wait_ms);
    std::string         timed_recv(int const bufsize, int const max_wait_ms);

private:
};



} // namespace ed
// vim: ts=4 sw=4 et

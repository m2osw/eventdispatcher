// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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


// C++ lib
//
#include    <memory>



namespace ed
{




// TODO: assuming that bio_client with MODE_PLAIN works the same way
//       as a basic tcp_client, we should remove this class
class tcp_client
{
public:
    typedef std::shared_ptr<tcp_client>     pointer_t;

                        tcp_client(std::string const & addr, int port);
                        tcp_client(tcp_client const & src) = delete;
    tcp_client &        operator = (tcp_client const & rhs) = delete;
                        ~tcp_client();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    int                 f_socket = -1;
    int                 f_port = -1;
    std::string         f_addr = std::string();
};



} // namespace tcp_client_server
// vim: ts=4 sw=4 et

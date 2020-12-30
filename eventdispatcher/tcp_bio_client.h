// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/tcp_bio_options.h"


// C++ lib
//
#include    <memory>



namespace ed
{



namespace detail
{
class tcp_bio_client_impl;
}



class tcp_bio_server;



// Create/manage certificates details:
// https://help.ubuntu.com/lts/serverguide/certificates-and-security.html
class tcp_bio_client
{
public:
    typedef std::shared_ptr<tcp_bio_client>     pointer_t;

    enum class mode_t
    {
        MODE_PLAIN,             // avoid SSL/TLS
        MODE_SECURE,            // WARNING: may return a non-verified connection
        MODE_ALWAYS_SECURE      // fails if cannot be 100% secure
    };

                        tcp_bio_client(
                                  std::string const & addr
                                , int port
                                , mode_t mode = mode_t::MODE_PLAIN
                                , tcp_bio_options const & opt = tcp_bio_options());
                        tcp_bio_client(tcp_bio_client const & src) = delete;
    virtual             ~tcp_bio_client();

    tcp_bio_client &    operator = (tcp_bio_client const & rhs) = delete;

    void                close();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char * buf, size_t size);
    int                 read_line(std::string & line);
    int                 write(char const * buf, size_t size);

private:
    friend class tcp_bio_server;

                        tcp_bio_client();

    std::shared_ptr<detail::tcp_bio_client_impl>
                        f_impl = std::shared_ptr<detail::tcp_bio_client_impl>();
};



} // namespace ed
// vim: ts=4 sw=4 et

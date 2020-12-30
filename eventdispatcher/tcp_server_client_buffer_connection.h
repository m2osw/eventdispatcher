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
#include    "eventdispatcher/tcp_server_client_connection.h"



namespace ed
{



class tcp_server_client_buffer_connection
    : public tcp_server_client_connection
{
public:
    typedef std::shared_ptr<tcp_server_client_buffer_connection>    pointer_t;

                                tcp_server_client_buffer_connection(tcp_bio_client::pointer_t client);

    bool                        has_input() const;
    bool                        has_output() const;

    // connection implementation
    virtual bool                is_writer() const override;

    // tcp_server_client_connection implementation
    virtual ssize_t             write(void const * data, size_t const length) override;
    virtual void                process_read() override;
    virtual void                process_write() override;
    virtual void                process_hup() override;

    // new callback
    virtual void                process_line(std::string const & line) = 0;

private:
    std::string                 f_line = std::string();
    std::vector<char>           f_output = std::vector<char>();
    size_t                      f_position = 0;
};



} // namespace ed
// vim: ts=4 sw=4 et

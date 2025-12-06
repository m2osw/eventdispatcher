// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Declaration of a UDP appender to send messages to a server.
 *
 * This file declares the UDP appender one can use to send messages to
 * a snaploggerd server. The UDP protocol is very light and can be used
 * without the need to support acknowledgements.
 */

// self
//
#include    "base_network_appender.h"





namespace snaplogger_network
{


class udp_appender
    : public base_network_appender
{
public:
    typedef std::shared_ptr<udp_appender>      pointer_t;

                        udp_appender(std::string const & name);
    virtual             ~udp_appender() override;

    // appender implementation
    //
    virtual void        set_config(advgetopt::getopt const & params) override;

protected:
    virtual bool        process_message(
                                  snaplogger::message const & msg
                                , std::string const & formatted_message) override;

private:
    std::string         f_secret_code = std::string();
};


} // namespace snaplogger_network
// vim: ts=4 sw=4 et

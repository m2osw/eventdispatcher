// TCP Client & Server -- classes to ease handling sockets
// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "eventdispatcher/utils.h"


//// addr lib
////
//#include "libaddr/addr.h"
//
//// C++ lib
////
//#include <stdexcept>
//#include <memory>
//
//// C lib
////
//#include <arpa/inet.h>
//
//// OpenSSL lib
////
//// BIO versions of the TCP client/server
//// TODO: move to an impl
//#include <openssl/bio.h>
//#include <openssl/err.h>
//#include <openssl/ssl.h>

namespace ed
{





void        cleanup();
void        cleanup_on_thread_exit();



//bool is_ipv4(char const * ip);
//bool is_ipv6(char const * ip);
//void get_addr_port(QString const & addr_port, QString & addr, int & port, char const * protocol);


} // namespace ed
// vim: ts=4 sw=4 et

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

/** \file
 * \brief Various helper functions.
 *
 * These functions are useful for our event dispatcher environment.
 */


// self
//
#include    "eventdispatcher/utils.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Get the current date.
 *
 * This function retrieves the current date and time with a precision
 * of microseconds.
 *
 * \todo
 * This is also defined in snap_child::get_current_date() so we should
 * unify that in some way...
 *
 * \return The time in microseconds.
 */
std::int64_t get_current_date()
{
    timeval tv;
    if(gettimeofday(&tv, nullptr) != 0)
    {
        int const err(errno);
        SNAP_LOG_FATAL
            << "gettimeofday() failed with errno: "
            << err
            << " ("
            << strerror(err)
            << ")";
        throw event_dispatcher_runtime_error("gettimeofday() failed");
    }

    return static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1000000)
         + static_cast<int64_t>(tv.tv_usec);
}



/** \brief Get the current date.
 *
 * This function retrieves the current date and time with a precision
 * of nanoseconds.
 *
 * \return The time in nanoseconds.
 */
std::int64_t get_current_date_ns()
{
    timespec ts;
    if(clock_gettime(CLOCK_REALTIME_COARSE, &ts) != 0)
    {
        int const err(errno);
        SNAP_LOG_FATAL
            << "clock_gettime() failed with errno: "
            << err
            << " ("
            << strerror(err)
            << ")";
        throw event_dispatcher_runtime_error("clock_gettime() failed");
    }

    return static_cast<int64_t>(ts.tv_sec) * static_cast<int64_t>(1000000000)
         + static_cast<int64_t>(ts.tv_nsec);
}









// ========================= HELPER FUNCTIONS TO DELETE ======================



///** \brief Check wether a string represents an IPv4 address.
// *
// * This function quickly checks whether the specified string defines a
// * valid IPv4 address. It supports all classes (a.b.c.d, a.b.c., a.b, a)
// * and all numbers can be in decimal, hexadecimal, or octal.
// *
// * \note
// * The function can be called with a null pointer in which case it
// * immediate returns false.
// *
// * \param[in] ip  A pointer to a string holding an address.
// *
// * \return true if the \p ip string represents an IPv4 address.
// *
// * \sa snap_inet::inet_pton_v6()
// */
//bool is_ipv4(char const * ip)
//{
//    if(ip == nullptr)
//    {
//        return false;
//    }
//
//    // we must have (1) a number then (2) a dot or end of string
//    // with a maximum of 4 numbers and 3 dots
//    //
//    int64_t addr[4];
//    size_t pos(0);
//    for(;; ++ip, ++pos)
//    {
//        if(*ip < '0' || *ip > '9' || pos >= sizeof(addr) / sizeof(addr[0]))
//        {
//            // not a valid number
//            return false;
//        }
//        int64_t value(0);
//
//        // number, may be decimal, octal, or hexadecimal
//        if(*ip == '0')
//        {
//            if(ip[1] == 'x' || ip[1] == 'X')
//            {
//                // expect hexadecimal
//                bool first(true);
//                for(ip += 2;; ++ip, first = false)
//                {
//                    if(*ip >= '0' && *ip <= '9')
//                    {
//                        value = value * 16 + *ip - '0';
//                    }
//                    else if(*ip >= 'a' && *ip <= 'f')
//                    {
//                        value = value * 16 + *ip - 'a' + 10;
//                    }
//                    else if(*ip >= 'A' && *ip <= 'F')
//                    {
//                        value = value * 16 + *ip - 'A' + 10;
//                    }
//                    else
//                    {
//                        if(first)
//                        {
//                            // not even one digit, not good
//                            return false;
//                        }
//                        // not valid hexadecimal, may be '.' or '\0' (tested below)
//                        break;
//                    }
//                    if(value >= 0x100000000)
//                    {
//                        // too large even if we have no dots
//                        return false;
//                    }
//                }
//            }
//            else
//            {
//                // expect octal
//                for(++ip; *ip >= '0' && *ip <= '8'; ++ip)
//                {
//                    value = value * 8 + *ip - '0';
//                    if(value >= 0x100000000)
//                    {
//                        // too large even if we have no dots
//                        return false;
//                    }
//                }
//            }
//        }
//        else
//        {
//            // expect decimal
//            for(; *ip >= '0' && *ip <= '9'; ++ip)
//            {
//                value = value * 10 + *ip - '0';
//                if(value >= 0x100000000)
//                {
//                    // too large even if we have no dots
//                    return false;
//                }
//            }
//        }
////std::cerr << "value[" << pos << "] = " << value << "\n";
//        addr[pos] = value;
//        if(*ip != '.')
//        {
//            if(*ip != '\0')
//            {
//                return false;
//            }
//            ++pos;
//            break;
//        }
//    }
//
////std::cerr << "pos = " << pos << "\n";
//    switch(pos)
//    {
//    case 1:
//        // one large value is considered valid for IPv4
//        // max. was already checked
//        return true;
//
//    case 2:
//        return addr[0] < 256 && addr[1] < 0x1000000;
//
//    case 3:
//        return addr[0] < 256 && addr[1] < 256 && addr[2] < 0x10000;
//
//    case 4:
//        return addr[0] < 256 && addr[1] < 256 && addr[2] < 256 && addr[3] < 256;
//
//    //case 0: (can happen on empty string)
//    default:
//        // no values, that is incorrect!?
//        return false;
//
//    }
//
//    snap::NOTREACHED();
//}
//
//
///** \brief Check wether a string represents an IPv6 address.
// *
// * This function quickly checks whether the specified string defines a
// * valid IPv6 address. It supports the IPv4 notation at times used
// * inside an IPv6 notation.
// *
// * \note
// * The function can be called with a null pointer in which case it
// * immediate returns false.
// *
// * \param[in] ip  A pointer to a string holding an address.
// *
// * \return true if the \p ip string represents an IPv6 address.
// */
//bool is_ipv6(char const * ip)
//{
//    if(ip == nullptr)
//    {
//        return false;
//    }
//
//    // an IPv6 is a set of 16 bit numbers separated by colon
//    // the last two numbers can represented in dot notation (ipv4 class a)
//    //
//    bool found_colon_colon(false);
//    int count(0);
//    if(*ip == ':'
//    && ip[1] == ':')
//    {
//        found_colon_colon = true;
//        ip += 2;
//    }
//    for(; *ip != '\0'; ++ip)
//    {
//        if(count >= 8)
//        {
//            return false;
//        }
//
//        // all numbers are in hexadecimal
//        int value(0);
//        bool first(true);
//        for(;; ++ip, first = false)
//        {
//            if(*ip >= '0' && *ip <= '9')
//            {
//                value = value * 16 + *ip - '0';
//            }
//            else if(*ip >= 'a' && *ip <= 'f')
//            {
//                value = value * 16 + *ip - 'a' + 10;
//            }
//            else if(*ip >= 'A' && *ip <= 'F')
//            {
//                value = value * 16 + *ip - 'A' + 10;
//            }
//            else
//            {
//                if(first)
//                {
//                    // not even one digit, not good
//                    return false;
//                }
//                // not valid hexadecimal, may be ':' or '\0' (tested below)
//                break;
//            }
//            if(value >= 0x10000)
//            {
//                // too large, must be 16 bit numbers
//                return false;
//            }
//        }
//        ++count;
////std::cerr << count << ". value=" << value << " -> " << static_cast<int>(*ip) << "\n";
//        if(*ip == '\0')
//        {
//            break;
//        }
//
//        // note: if we just found a '::' then here *ip == ':' still
//        if(*ip == '.')
//        {
//            // if we have a '.' we must end with an IPv4 and we either
//            // need found_colon_colon to be true or the count must be
//            // exactly 6 (1 "missing" colon)
//            //
//            if(!found_colon_colon
//            && count != 7)  // we test with 7 because the first IPv4 number was already read
//            {
//                return false;
//            }
//            // also the value is 0 to 255 or it's an error too, but the
//            // problem here is that we need a decimal number and we just
//            // checked it as an hexadecimal...
//            //
//            if((value & 0x00f) >= 0x00a
//            || (value & 0x0f0) >= 0x0a0
//            || (value & 0xf00) >= 0xa00)
//            {
//                return false;
//            }
//            // transform back to a decimal number to verify the max.
//            //
//            value = (value & 0x00f) + (value & 0x0f0) / 16 * 10 + (value & 0xf00) / 256 * 100;
//            if(value > 255)
//            {
//                return false;
//            }
//            // now check the other numbers
//            int pos(1); // start at 1 since we already have 1 number checked
//            for(++ip; *ip != '\0'; ++ip, ++pos)
//            {
//                if(*ip < '0' || *ip > '9' || pos >= 4)
//                {
//                    // not a valid number
//                    return false;
//                }
//
//                // only expect decimal in this case in class d (a.b.c.d)
//                value = 0;
//                for(; *ip >= '0' && *ip <= '9'; ++ip)
//                {
//                    value = value * 10 + *ip - '0';
//                    if(value > 255)
//                    {
//                        // too large
//                        return false;
//                    }
//                }
//
//                if(*ip != '.')
//                {
//                    if(*ip != '\0')
//                    {
//                        return false;
//                    }
//                    break;
//                }
//            }
//
//            // we got a valid IPv4 at the end of IPv6 and we
//            // found the '\0' so we are all good...
//            //
//            return true;
//        }
//
//        if(*ip != ':')
//        {
//            return false;
//        }
//
//        // double colon?
//        if(ip[1] == ':')
//        {
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wstrict-overflow"
//            if(!found_colon_colon && count < 6)
//#pragma GCC diagnostic pop
//            {
//                // we can accept one '::'
//                ++ip;
//                found_colon_colon = true;
//            }
//            else
//            {
//                // a second :: is not valid for an IPv6
//                return false;
//            }
//        }
//    }
//
//    return count == 8 || (count >= 1 && found_colon_colon);
//}
//
//
///** \brief Retrieve an address and a port from a string.
// *
// * This function breaks up an address and a port number from a string.
// *
// * The address can either be an IPv4 address followed by a colon and
// * the port number, or an IPv6 address written between square brackets
// * ([::1]) followed by a colon and the port number. We also support
// * just a port specification as in ":4040".
// *
// * Port numbers are limited to a number between 1 and 65535 inclusive.
// * They can only be specified in base 10.
// *
// * The port is optional only if a \p default_port is provided (by
// * default the \p default_port parameter is set to zero meaning that
// * it is not specified.)
// *
// * If the addr_port string is empty, then the addr and port parameters
// * are not modified, which means you want to define them with defaults
// * before calling this function.
// *
// * \exception snapwebsites_exception_invalid_parameters
// * If any parameter is considered invalid (albeit the validity of the
// * address is not checked since it could be a fully qualified domain
// * name) then this exception is raised.
// *
// * \param[in] addr_port  The address and port pair.
// * \param[in,out] addr  The address part, without the square brackets for
// *                IPv6 addresses.
// * \param[in,out] port  The port number (1 to 65535 inclusive.)
// * \param[in] protocol  The protocol for the port (i.e. "tcp" or "udp")
// */
//void get_addr_port(QString const & addr_port, QString & addr, int & port, char const * protocol)
//{
//    // if there is a colon, we may have a port or IPv6
//    //
//    int const p(addr_port.lastIndexOf(":"));
//    if(p != -1)
//    {
//        QString port_str;
//
//        // if there is a ']' then we have an IPv6
//        //
//        int const bracket(addr_port.lastIndexOf("]"));
//        if(bracket != -1)
//        {
//            // we must have a starting '[' otherwise it is wrong
//            //
//            if(addr_port[0] != '[')
//            {
//                SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\" (missing '[' at the start.)");
//                throw tcp_client_server_parameter_error("get_addr_port(): invalid [IPv6]:port specification, '[' missing.");
//            }
//
//            // extract the address
//            //
//            addr = addr_port.mid(1, bracket - 1); // exclude the '[' and ']'
//
//            // is there a port?
//            //
//            if(p == bracket + 1)
//            {
//                // IPv6 port specification is just after the ']'
//                //
//                port_str = addr_port.mid(p + 1); // ignore the ':'
//            }
//            else if(bracket != addr_port.length() - 1)
//            {
//                // the ']' is not at the very end when no port specified
//                //
//                SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\" (']' is not at the end)");
//                throw tcp_client_server_parameter_error("get_addr_port(): invalid [IPv6]:port specification, ']' not at the end.");
//            }
//        }
//        else
//        {
//            // IPv4 port specification
//            //
//            if(p > 0)
//            {
//                // if p is zero, then we just had a port (:4040)
//                //
//                addr = addr_port.mid(0, p); // ignore the ':'
//            }
//            port_str = addr_port.mid(p + 1); // ignore the ':'
//        }
//
//        // if port_str is still empty, we had an IPv6 without port
//        //
//        if(!port_str.isEmpty())
//        {
//            // first check whether the port is a number
//            //
//            bool ok(false);
//            port = port_str.toInt(&ok, 10); // force base 10
//            if(!ok)
//            {
//                // not a valid number, try to get it from /etc/services
//                //
//                struct servent const * s(getservbyname(port_str.toUtf8().data(), protocol));
//                if(s == nullptr)
//                {
//                    SNAP_LOG_FATAL("invalid port specification in \"")(addr_port)("\", port not a decimal number nor a known service name.");
//                    throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, port number or name is not valid.");
//                }
//                port = s->s_port;
//            }
//        }
//    }
//    else if(!addr_port.isEmpty())
//    {
//        // just an IPv4 address specified, no port
//        //
//        addr = addr_port;
//    }
//
//    // the address could end up being the empty string here
//    if(addr.isEmpty())
//    {
//        SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\", address is empty.");
//        throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, address is empty (this generally happens when a request is done with no default address).");
//    }
//
//    // finally verify that the port is in range
//    if(port <= 0
//    || port > 65535)
//    {
//        SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\", port out of bounds.");
//        throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, port number is out of bounds (1 .. 65535).");
//    }
//}


} // namespace ed
// vim: ts=4 sw=4 et

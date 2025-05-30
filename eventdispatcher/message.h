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

/** \file
 * \brief Messages sent between services.
 *
 * Class used to manage messages.
 */

// self
//
#include    <eventdispatcher/message_definition.h>
#include    <eventdispatcher/utils.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/timespec_ex.h>


// libaddr
//
#include    <libaddr/addr.h>
#include    <libaddr/addr_unix.h>



namespace ed
{



typedef int                         message_version_t;

// this version defines the protocol version, it should really rarely
// change if ever
//
constexpr message_version_t         MESSAGE_VERSION = 1;

constexpr char const                MESSAGE_VERSION_NAME[]  = "version";



class message
{
public:
    typedef std::shared_ptr<message>    pointer_t;
    typedef std::vector<message>        vector_t;
    typedef std::list<message>          list_t;
    typedef string_map_t                parameters_t;

    enum class format_t
    {
        MESSAGE_FORMAT_STRING,
        MESSAGE_FORMAT_JSON,
    };

    bool                    from_message(std::string const & msg);
    bool                    from_string(std::string const & msg);
    bool                    from_json(std::string const & msg);
    std::string             to_message(format_t format = format_t::MESSAGE_FORMAT_STRING) const;
    std::string             to_string() const;
    std::string             to_json() const;

    std::string const &     get_sent_from_server() const;
    void                    set_sent_from_server(std::string const & server);
    std::string const &     get_sent_from_service() const;
    void                    set_sent_from_service(std::string const & service);
    std::string const &     get_server() const;
    void                    set_server(std::string const & server);
    std::string const &     get_service() const;
    void                    set_service(std::string const & service);
    void                    reply_to(message const & msg);
    std::string const &     get_command() const;
    void                    set_command(std::string const & command);
    message_version_t       get_message_version() const;
    bool                    check_version_parameter() const;
    void                    add_version_parameter();
    void                    add_parameter(std::string const & name, std::string const & value);
    void                    add_parameter(std::string const & name, std::int16_t value);
    void                    add_parameter(std::string const & name, std::uint16_t value);
    void                    add_parameter(std::string const & name, std::int32_t value);
    void                    add_parameter(std::string const & name, std::uint32_t value);
    void                    add_parameter(std::string const & name, long long value);
    void                    add_parameter(std::string const & name, unsigned long long value);
    void                    add_parameter(std::string const & name, std::int64_t value);
    void                    add_parameter(std::string const & name, std::uint64_t value);
    void                    add_parameter(std::string const & name, addr::addr const & value, bool mask = false);
    void                    add_parameter(std::string const & name, addr::addr_unix const & value);
    void                    add_parameter(std::string const & name, snapdev::timespec_ex const & value);
    bool                    has_parameter(std::string const & name) const;
    bool                    check_parameters(message_parameter::vector_t const & parameter_definitions) const;
    std::string             get_parameter(std::string const & name) const;
    std::int64_t            get_integer_parameter(std::string const & name) const;
    snapdev::timespec_ex    get_timespec_parameter(std::string const & name) const;
    parameters_t const &    get_all_parameters() const;

    template<typename T>
    void                    user_data(std::shared_ptr<T> data) { f_user_data = data; }
    template<typename T>
    std::shared_ptr<T>      user_data() { return std::static_pointer_cast<T>(f_user_data); }

    void                    mark_processed();
    bool                    was_processed() const;

private:
    std::string             f_sent_from_server = std::string();
    std::string             f_sent_from_service = std::string();
    std::string             f_server = std::string();
    std::string             f_service = std::string();
    std::string             f_command = std::string();
    parameters_t            f_parameters = parameters_t();
    mutable std::string     f_cached_message = std::string();
    mutable std::string     f_cached_json = std::string();
    std::shared_ptr<void>   f_user_data = std::shared_ptr<void>();
    bool                    f_processed = false;
};


void        verify_message_name(
                  std::string const & name
                , bool can_be_empty = false
                , bool can_be_lowercase = true);



} // namespace ed


// IMPORTANT NOTE: placed in global scope to avoid overloading
//                 and preventing other global scope << from
//                 being ignored
//
template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator << (std::basic_ostream<CharT, Traits> & os, ed::message const & msg)
{
    std::string const str(msg.to_string());
    snaplogger::message * m(dynamic_cast<snaplogger::message *>(&os));
    if(m != nullptr)
    {
        m->add_field("eventdispatcher_message", str);
    }
    return os << str;
}



// vim: ts=4 sw=4 et

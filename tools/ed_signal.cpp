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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the Connection with Send Message class.
 *
 * This is a base class which ease the implementation of a connection
 * which is able to send and receive messages.
 */


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/signal_handler.h>
#include    <eventdispatcher/tcp_client_message_connection.h>
#include    <eventdispatcher/udp_server_message_connection.h>
#include    <eventdispatcher/version.h>


// snaplogger
//
#include    <snaplogger/logger.h>
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// advgetopt
//
#include    <advgetopt/exception.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/stringize.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("encrypt")
        , advgetopt::ShortName('e')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("use a secure connection if set to true, 1 or yes (TCP only)")
    ),
    advgetopt::define_option(
          advgetopt::Name("message")
        , advgetopt::ShortName('m')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("command to send to the specified server")
    ),
    advgetopt::define_option(
          advgetopt::Name("param")
        , advgetopt::ShortName('p')
        , advgetopt::Flags(advgetopt::any_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("a parameter to send along the command, can be repeated")
    ),
    advgetopt::define_option(
          advgetopt::Name("reply")
        , advgetopt::ShortName('r')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("use this option to see the reply, otherwise it waits for the reply but doesn't do anything with it (TCP only)")
    ),
    advgetopt::define_option(
          advgetopt::Name("secret-code")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("a simple password so we can make UDP packets very slightly more secure (uses parameter \"udp_secret\")")
    ),
    advgetopt::define_option(
          advgetopt::Name("server")
        , advgetopt::ShortName('s')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("the IP address and port to connect to (port defaults to 4041)")
    ),
    advgetopt::define_option(
          advgetopt::Name("type")
        , advgetopt::ShortName('t')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("defines the type of connection: \"tcp\" or \"udp\" (default) -- WARNING: the default can be changed in the configuration file")
    ),
    advgetopt::end_options()
};

advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};

constexpr char const * const g_configuration_files[] =
{
    "/etc/eventdispatcher/ed-signal.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "ed-signal",
    .f_group_name = "eventdispatcher",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "ED_SIGNAL",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = EVENTDISPATCHER_VERSION_STRING,
    .f_license = "GNU GPL v2 or newer",
    .f_copyright = "Copyright (c) 2012-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop

}
// noname namespace



class ed_signal;

class tcp_signal
    : public ed::tcp_client_message_connection
{
public:
    typedef std::shared_ptr<tcp_signal>     pointer_t;

                                    tcp_signal(
                                          ed_signal * parent
                                        , addr::addr const & address
                                        , ed::mode_t mode);
    virtual                         ~tcp_signal() override;
                                    tcp_signal(tcp_signal const &) = delete;
    tcp_signal &                    operator = (tcp_signal const &) = delete;

    void                            show_reply();

    // tcp_client_buffer_connection implementation
    virtual void                    process_line(std::string const & line) override;

private:
    ed_signal *                     f_parent = nullptr;
    bool                            f_show_reply = false;
};




class ed_signal
{
public:
    static constexpr int const      DEFAULT_PORT = 4041;

                                    ed_signal(int argc, char * argv[]);

    int                             run();
    void                            done();

private:
    advgetopt::getopt               f_opts;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    tcp_signal::pointer_t           f_tcp_connection = tcp_signal::pointer_t();
};



tcp_signal::tcp_signal(
              ed_signal * parent
            , addr::addr const & address
            , ed::mode_t mode)
    : tcp_client_message_connection(address, mode)
    , f_parent(parent)
{
}


tcp_signal::~tcp_signal()
{
}


void tcp_signal::show_reply()
{
    f_show_reply = true;
}


void tcp_signal::process_line(std::string const & line)
{
    if(f_show_reply)
    {
        std::cout << line << std::endl;
    }

    f_parent->done();
}






ed_signal::ed_signal(int argc, char * argv[])
    : f_opts(g_options_environment)
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/eventdispatcher/logger"))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }
}


int ed_signal::run()
{
    ed::message msg;

    {
        if(!f_opts.is_defined("message"))
        {
            throw std::runtime_error("the --message parameter is required");
        }
        std::string const cmd(f_opts.get_string("message"));
        if(cmd.empty())
        {
            throw std::runtime_error("the --message parameter cannot be an empty string");
        }
        msg.set_command(cmd);
    }

    {
        std::size_t const max(f_opts.size("param"));
        for(std::size_t idx(0); idx < max; ++idx)
        {
            std::string const param(f_opts.get_string("param", idx));
            std::string::size_type const pos(param.find('='));
            if(pos == std::string::npos)
            {
                msg.add_parameter(param, "");
            }
            else
            {
                msg.add_parameter(param.substr(0, pos), param.substr(pos + 1));
            }
        }
    }

    bool use_tcp(false);
    if(f_opts.is_defined("type"))
    {
        std::string const type(f_opts.get_string("type"));
        if(type == "tcp")
        {
            use_tcp = true;
        }
        else if(type != "udp")
        {
            throw std::runtime_error("unrecognized connection type: \"" + type + "\", we support \"tcp\" and \"udp\"");
        }
    }

    addr::addr const server(addr::string_to_addr(
          f_opts.get_string("server")
        , "127.0.0.1"
        , DEFAULT_PORT
        , use_tcp ? "tcp" : "udp"));

    if(use_tcp)
    {
        bool encrypt(false);
        if(f_opts.is_defined("encrypt"))
        {
            std::string const e(f_opts.get_string("encrypt"));
            encrypt = advgetopt::is_true(e);
            if(!encrypt && !advgetopt::is_false(e))
            {
                throw std::runtime_error("encrypt parameter is not true or false (\"" + e + "\" is not valid)");
            }
        }

        f_tcp_connection = std::make_shared<tcp_signal>(
                  this
                , server
                , encrypt
                        ? ed::mode_t::MODE_SECURE
                        : ed::mode_t::MODE_PLAIN
            );

        if(f_opts.is_defined("reply"))
        {
            f_tcp_connection->show_reply();
        }

        f_tcp_connection->send_message(msg);

        f_communicator = ed::communicator::instance();
        if(!f_communicator->add_connection(f_tcp_connection))
        {
            throw std::runtime_error("could not add TCP connection to communicator");
        }

        f_communicator->run();
    }
    else
    {
        std::string secret_code;
        if(f_opts.is_defined("secret_code"))
        {
            secret_code = f_opts.get_string("secret-code");
        }

        // very simple in this case we can just send the message and
        // we're done (no need for the communicator itself)
        //
        ed::udp_server_message_connection::send_message(
                  server
                , msg
                , secret_code);
    }

    return 0;
}


void ed_signal::done()
{
    f_communicator->remove_connection(f_tcp_connection);
}



int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();

    try
    {
        ed_signal s(argc, argv);
        return s.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(e.code());
    }
    catch(std::exception const & e)
    {
        std::cerr << "error: an exception occurred (1): " << e.what() << std::endl;
        SNAP_LOG_FATAL
            << "an exception occurred (1): "
            << e.what()
            << SNAP_LOG_SEND;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred (2)." << std::endl;
        SNAP_LOG_FATAL
            << "an unknown exception occurred (2)."
            << SNAP_LOG_SEND;
        exit(2);
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et

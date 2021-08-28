

//
// Here is an example creating a daemon using the event dispatcher
//
// It shows you how to initialize the command line options (f_opt)
// along the snaplogger and then query various values.
//


// snaplogger lib
//
#include    <snaplogger/options.h>


// libaddr lib
//
#include    <libaddr/addr.h>
#include    <libaddr/addr_parser.h>


// advgetopt lib
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/options.h>
#include    <advgetopt/exception.h>


// eventdispatcher lib
//
#include    <eventdispatcher/logrotate_udp_messenger.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>
#include    <eventdispatcher/version.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>


// "local header declarations"
//

class my_client;

class client
    : public ed::tcp_client_permanent_message_connection
{
public:
    typedef std::shared_ptr<client>
                                pointer_t;

                                client(
                                      my_client * c
                                    , addr::addr const & a);
                                client(client const &) = delete;

    client &                    operator = (client const &) = delete;

    void                        msg_hi(ed::message & msg);
    void                        msg_climb(ed::message & msg);
    void                        msg_who(ed::message & msg);
    void                        msg_bye(ed::message & msg);

    // tcp_client_permanent_message_connection implementation
    //

private:
    my_client *                 f_my_client = nullptr;
    ed::dispatcher<client>::pointer_t
                                f_dispatcher = ed::dispatcher<client>::pointer_t();
};



class my_client
{
public:
    static constexpr int const  DEFAULT_PORT            = 3001;
    static constexpr int const  DEFAULT_LOG_ROTATE_PORT = 3003;

                                my_client(int argc, char * argv []);

    int                         run();
    void                        quit();

private:
    void                        setup_logrotate_listener();
    void                        setup_connection();

    advgetopt::getopt           f_opt;
    ed::communicator::pointer_t f_communicator = ed::communicator::pointer_t();
    ed::logrotate_udp_messenger::pointer_t
                                f_log_rotate_messenger = ed::logrotate_udp_messenger::pointer_t();
    client::pointer_t           f_client = client::pointer_t();
};










namespace
{

constexpr advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("server")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:3001")
        , advgetopt::Help("the host to connect to for IPC messages")
    ),
    advgetopt::define_option(
          advgetopt::Name("log-rotate-listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:3003")
        , advgetopt::Help("the host to listen on for the LOG message")
    ),
    advgetopt::define_option(
          advgetopt::Name("log-rotate-secret")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("")
        , advgetopt::Help("a secret code to be used along the log-rotate-listen option; use empty to not have to use a secret code")
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
    "/etc/eventdispatcher/client.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "client",
    .f_group_name = "ed",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "CLIENT",
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = EVENTDISPATCHER_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = "Copyright (c) 2021-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) "  Virtual Entertainment",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop


ed::dispatcher<client>::dispatcher_match::vector_t const g_messages =
{
    {
          "HI"
        , &client::msg_hi
    },
    {
          "CLIMB"
        , &client::msg_climb
    },
    {
          "WHO"
        , &client::msg_who
    },
    {
          "BYE"
        , &client::msg_bye
    },

    // ALWAYS LAST
    {
          nullptr
        , &client::msg_reply_with_unknown
        , &ed::dispatcher<client>::dispatcher_match::always_match
    }
};




}
// no name namespace









client::client(
          my_client * c
        , addr::addr const & a)
    : tcp_client_permanent_message_connection(
              a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
            , a.get_port())
    , f_my_client(c)
    , f_dispatcher(new ed::dispatcher<client>(
              this
            , g_messages))
{
    set_name("client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);
}


void client::msg_hi(ed::message & msg)
{
    ed::message reply;
    reply.set_command("DAD");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_climb(ed::message & msg)
{
    ed::message reply;
    reply.set_command("TOP");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_who(ed::message & msg)
{
    ed::message reply;
    reply.set_command("MOM");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_bye(ed::message & msg)
{
    snap::NOT_USED(msg);

    ed::communicator::pointer_t communicator(ed::communicator::instance());
    communicator->remove_connection(shared_from_this());

    f_my_client->quit();
}

















my_client::my_client(int argc, char * argv [])
    : f_opt(g_options_environment)
    , f_communicator(ed::communicator::instance())	// see the event dispatcher project
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opt, "/etc/ve/logger");

    setup_logrotate_listener();
    setup_connection();
}


void my_client::setup_logrotate_listener()
{
    std::string const log_rotate_listen(f_opt.get_string("log-rotate-listen"));
    if(log_rotate_listen.empty())
    {
        // this should never happen since we have a default
        //
        SNAP_LOG_FATAL
            << "the \"log_rotate_listen=...\" must be defined."
            << SNAP_LOG_SEND;
        throw std::runtime_error("the \"log_rotate_listen=...\" must be defined.");
    }

    addr::addr const log_rotate_addr(addr::string_to_addr(
          log_rotate_listen
        , "127.0.0.1"
        , DEFAULT_LOG_ROTATE_PORT
        , "udp"));

    f_log_rotate_messenger = std::make_shared<ed::logrotate_udp_messenger>(
          log_rotate_addr
        , f_opt.get_string("log-rotate-secret"));

    f_communicator->add_connection(f_log_rotate_messenger);
}


void my_client::setup_connection()
{
    std::string const server(f_opt.get_string("server"));
    if(server.empty())
    {
        // this should never happen since we have a default
        //
        SNAP_LOG_FATAL
            << "the \"server=...\" must be defined."
            << SNAP_LOG_SEND;
        throw std::runtime_error("the \"server=...\" must be defined.");
    }

    addr::addr const addr(addr::string_to_addr(
          server
        , "127.0.0.1"
        , DEFAULT_PORT
        , "tcp"));

    f_client = std::make_shared<client>(this, addr);

    f_communicator->add_connection(f_client);
}


int my_client::run()
{
    f_communicator->run();
    return 0;
}


void my_client::quit()
{
    f_communicator->remove_connection(f_log_rotate_messenger);
}





int main(int argc, char * argv[])
{
    try
    {
        my_client c(argc, argv);
        return c.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(std::exception const & e)
    {
        std::cerr                                                               
            << "error: an exception occurred: "                                 
            << e.what()                                                         
            << std::endl;                                                       
        SNAP_LOG_ERROR                                                          
            << "error: an exception occurred: "                                 
            << e.what()                                                         
            << SNAP_LOG_SEND;                                                   
    }
    catch(...)
    {
        std::cerr                                                               
            << "error: an unknown exception occurred."                          
            << std::endl;                                                       
        SNAP_LOG_ERROR                                                          
            << "error: an unknown exception occurred."                          
            << SNAP_LOG_SEND;                                                   
    }

    return 1;
}


// vim: ts=4 sw=4 et

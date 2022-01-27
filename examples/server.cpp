

//
// Here is an example creating a daemon using the event dispatcher
//
// It shows you how to initialize the command line options (f_opt)
// along the snaplogger and then query various values and setup
// a server.
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
#include    <eventdispatcher/dispatcher.h>
#include    <eventdispatcher/logrotate_udp_messenger.h>
#include    <eventdispatcher/tcp_server_client_message_connection.h>
#include    <eventdispatcher/tcp_server_connection.h>
#include    <eventdispatcher/version.h>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// last include
//
#include    <snapdev/poison.h>




// "local header declarations"
//

class my_daemon;

class client
    : public ed::tcp_server_client_message_connection
{
public:
    typedef std::shared_ptr<client>
                                pointer_t;

                                client(
                                      my_daemon * d
                                    , ed::tcp_bio_client::pointer_t client);
                                client(client const &) = delete;

    client &                    operator = (client const &) = delete;

    void                        msg_dad(ed::message & msg);
    void                        msg_top(ed::message & msg);
    void                        msg_mom(ed::message & msg);
    void                        msg_quit(ed::message & msg);

    // tcp_server_client_message_connection implementation
    //

private:
    my_daemon *                 f_my_daemon = nullptr;
    ed::dispatcher<client>::pointer_t
                                f_dispatcher = ed::dispatcher<client>::pointer_t();
};


class listener
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<listener>
                                pointer_t;

    static constexpr int const  DEFAULT_TCP_PORT = 4978;

                                listener(my_daemon * d, addr::addr const & a);
                                listener(listener const &) = delete;
    virtual                     ~listener() override;

    listener &                  operator = (listener const &) = delete;

    // tcp_server_connection implementation
    //
    virtual void                process_accept() override;

private:
    my_daemon *                 f_my_daemon = nullptr;
    ed::communicator::pointer_t f_communicator = ed::communicator::pointer_t();
    //client::weak_pointer_t      f_connection = client::weak_pointer_t();
};


class my_daemon
{
public:
    static constexpr int const  DEFAULT_PORT            = 3001;
    static constexpr int const  DEFAULT_LOG_ROTATE_PORT = 3002;

                                my_daemon(int argc, char * argv []);

    int                         run();
    void                        quit();

private:
    void                        setup_logrotate_listener();
    void                        setup_listener();

    advgetopt::getopt           f_opt;
    ed::communicator::pointer_t f_communicator = ed::communicator::pointer_t();
    ed::logrotate_udp_messenger::pointer_t
                                f_log_rotate_messenger = ed::logrotate_udp_messenger::pointer_t();
    listener::pointer_t         f_listener = listener::pointer_t();
};





namespace
{

constexpr advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:3001")
        , advgetopt::Help("the host to listen on for IPC messages")
    ),
    advgetopt::define_option(
          advgetopt::Name("log-rotate-listen")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::DefaultValue("127.0.0.1:3002")
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
    "/etc/eventdispatcher/server.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "server",
    .f_group_name = "ed",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SERVER",
    .f_section_variables_name = nullptr,
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
          "DAD"
        , &client::msg_dad
    },
    {
          "TOP"
        , &client::msg_top
    },
    {
          "MOM"
        , &client::msg_mom
    },
    {
          "QUIT"
        , &client::msg_quit
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









client::client(my_daemon * d, ed::tcp_bio_client::pointer_t c)
    : tcp_server_client_message_connection(c)
    , f_my_daemon(d)
    , f_dispatcher(new ed::dispatcher<client>(
              this
            , g_messages))
{
    set_name("client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    ed::message msg;
    msg.set_command("HI");
    send_message(msg);
}


void client::msg_dad(ed::message & msg)
{
    ed::message reply;
    reply.set_command("CLIMB");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_top(ed::message & msg)
{
    ed::message reply;
    reply.set_command("WHO");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_mom(ed::message & msg)
{
    ed::message reply;
    reply.set_command("BYE");
    reply.reply_to(msg);
    send_message(reply);
}


void client::msg_quit(ed::message & msg)
{
    snap::NOT_USED(msg);

    ed::communicator::instance()->remove_connection(shared_from_this());
    f_my_daemon->quit();
}














listener::listener(my_daemon * d, addr::addr const & a)
    : tcp_server_connection(
            a.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY)
          , a.get_port()
          , std::string()       // no SSL
          , std::string())
    , f_my_daemon(d)
    , f_communicator(ed::communicator::instance())	// see the event dispatcher project
{
}


listener::~listener()
{
}


void listener::process_accept()
{
    // a new client just connected, create a new service_connection
    // object and add it to the snap_communicator object.
    //
    ed::tcp_bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred, report in the logs
        int const e(errno);
        SNAP_LOG_ERROR
            << "somehow accept() failed with errno: "
            << e
            << " -- "
            << strerror(e)
            << SNAP_LOG_SEND;
        return;
    }

    client::pointer_t new_connection(std::make_shared<client>(f_my_daemon, new_client));

    if(!f_communicator->add_connection(new_connection))
    {
        // this should never happen here since each "new" creates a
        // new pointer
        //
        SNAP_LOG_ERROR
            << "new client connection could not be added to the communicator list of connections"
            << SNAP_LOG_SEND;
    }
    else
    {
        //f_connections.push_back(new_connection);
    }
}









my_daemon::my_daemon(int argc, char * argv [])
    : f_opt(g_options_environment)
    , f_communicator(ed::communicator::instance())	// see the event dispatcher project
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opt, "/etc/ve/logger");

    setup_logrotate_listener();
    setup_listener();
}


void my_daemon::setup_logrotate_listener()
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


void my_daemon::setup_listener()
{
    std::string const listen(f_opt.get_string("listen"));
    if(listen.empty())
    {
        // this should never happen since we have a default
        //
        SNAP_LOG_FATAL
            << "the \"listen=...\" must be defined."
            << SNAP_LOG_SEND;
        throw std::runtime_error("the \"listen=...\" must be defined.");
    }

    addr::addr const addr(addr::string_to_addr(
          listen
        , "127.0.0.1"
        , DEFAULT_PORT
        , "tcp"));

    f_listener = std::make_shared<listener>(this, addr);

    f_communicator->add_connection(f_listener);
}


int my_daemon::run()
{
    f_communicator->run();
    return 0;
}


void my_daemon::quit()
{
    f_communicator->remove_connection(f_log_rotate_messenger);
    f_communicator->remove_connection(f_listener);
}







int main(int argc, char * argv[])
{
    try
    {
        my_daemon daemon(argc, argv);
        return daemon.run();
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

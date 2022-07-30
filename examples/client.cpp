

//
// Here is an example creating a daemon using the event dispatcher
//
// It shows you how to initialize the command line options (f_opt)
// along the snaplogger and then query various values.
//


// snaplogger 
//
#include    <snaplogger/options.h>


// libaddr
//
#include    <libaddr/addr.h>
#include    <libaddr/addr_parser.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/options.h>
#include    <advgetopt/exception.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/logrotate_udp_messenger.h>
#include    <eventdispatcher/tcp_client_permanent_message_connection.h>
#include    <eventdispatcher/version.h>


// snaplogger
//
#include    <snaplogger/message.h>


// boost
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
    ed::dispatcher::pointer_t   f_dispatcher = ed::dispatcher::pointer_t();
};



class my_client
    : private ed::logrotate_extension
{
public:
    static constexpr int const  DEFAULT_PORT            = 3001;
    static constexpr int const  DEFAULT_LOGROTATE_PORT  = 3003;

                                my_client(int argc, char * argv []);

    int                         run();
    void                        quit();

private:
    void                        setup_connection();

    advgetopt::getopt           f_opts;
    ed::communicator::pointer_t f_communicator = ed::communicator::pointer_t();
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
    .f_license = nullptr,
    .f_copyright = "Copyright (c) 2021-" BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) "  Virtual Entertainment",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};
#pragma GCC diagnostic pop






}
// no name namespace









client::client(
          my_client * c
        , addr::addr const & a)
    : tcp_client_permanent_message_connection(a)
    , f_my_client(c)
    , f_dispatcher(std::make_shared<ed::dispatcher>(this))
{
    set_name("client");
#ifdef _DEBUG
    f_dispatcher->set_trace();
#endif
    set_dispatcher(f_dispatcher);

    f_dispatcher->add_matches({
        DISPATCHER_MATCH("HI",    &client::msg_hi),
        DISPATCHER_MATCH("CLIMB", &client::msg_climb),
        DISPATCHER_MATCH("WHO",   &client::msg_who),
        DISPATCHER_MATCH("BYE",   &client::msg_bye),

        // ALWAYS LAST
        DISPATCHER_CATCH_ALL()
    });
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
    snapdev::NOT_USED(msg);

    ed::communicator::pointer_t communicator(ed::communicator::instance());
    communicator->remove_connection(shared_from_this());

    f_my_client->quit();
}

















my_client::my_client(int argc, char * argv [])
    : logrotate_extension(
          f_opts
        , "127.0.0.1"
        , DEFAULT_LOGROTATE_PORT)
    , f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())	// see the event dispatcher project
{
    snaplogger::add_logger_options(f_opts);
    add_logrotate_options();
    f_opts.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opts, "/etc/ve/logger");
    process_logrotate_options();

    setup_connection();
}




void my_client::setup_connection()
{
    std::string const server(f_opts.get_string("server"));
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
    disconnect_logrotate_messenger();
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

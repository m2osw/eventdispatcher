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

/** \file
 * \brief Tool used to check a certificate end date.
 *
 * This tool reads the certificate attached to a domain and retrieves the
 * not-after field. It then checks that against the limit specified as a
 * number of days. If under the limit, then the tool returns with exit
 * code 1. Otherwise it returns with exit code 0.
 *
 * Basic usage example:
 *
 * \code
 *     check-certificate [--info] [--limit <days>] <domain-name>
 * \endcode
 *
 * The default number of days is 14 which gives you two weeks to update
 * the certificate if not automatically working already.
 *
 * \todo
 * Add support for multiple domain names although we cannot return 0 or 1
 * in that case.
 */


// eventdispatcher
//
#include    <eventdispatcher/certificate.h>
#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/signal_handler.h>
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
#include    <advgetopt/conf_file.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/validator_double.h>


// snapdev
//
#include    <snapdev/gethostname.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/stringize.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



const advgetopt::option g_options[] =
{
    // COMMANDS
    //
    advgetopt::define_option(
          advgetopt::Name("info")
        , advgetopt::ShortName('i')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("show the certificate info.")
    ),

    // OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("limit")
        , advgetopt::ShortName('l')
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::DefaultValue("14")
        , advgetopt::Help("number of days under which the certificate is considered in need of renewal.")
    ),

    // DEFAULT
    //
    advgetopt::define_option(
          advgetopt::Name("domain")
        , advgetopt::Flags(advgetopt::command_flags<
              advgetopt::GETOPT_FLAG_GROUP_NONE
            //, advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_DEFAULT_OPTION>())
    ),

    // END
    //
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
    "/etc/eventdispatcher/check-certificate.conf",
    nullptr
};

advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "check-certificate",
    .f_group_name = "eventdispatcher",
    .f_options = g_options,
    .f_environment_variable_name = "CHECK_CERTIFICATE",
    .f_configuration_files = g_configuration_files,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] <domain>\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = EVENTDISPATCHER_VERSION_STRING,
    .f_license = "GNU GPL v2 or newer",
    .f_copyright = "Copyright (c) 2012-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_groups = g_group_descriptions
};



}
// no name namespace



class check_certificate
{
public:
    static constexpr int const      DEFAULT_PORT = 4041;

                                    check_certificate(int argc, char * argv[]);

    int                             run();

private:
    void                            print(char const * field_name, std::string const & value);

    advgetopt::getopt               f_opts;
    bool                            f_info = false;
    double                          f_limit = 14.0;
    std::string                     f_domain = std::string();
};






check_certificate::check_certificate(int argc, char * argv[])
    : f_opts(g_options_environment)
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(
          f_opts
        , "/etc/eventdispatcher/logger"
        , std::cout
        , !isatty(fileno(stdin))))
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 1);
    }
}


int check_certificate::run()
{
    // check whether --info is defined
    //
    f_info = f_opts.is_defined("info");

    // get define the used defined limit
    //
    if(f_opts.is_defined("limit"))
    {
        std::string const limit(f_opts.get_string("limit"));
        if(!advgetopt::validator_double::convert_string(limit, f_limit))
        {
            throw ed::invalid_parameter("limit must be a valid decimal number, it can include a decimal point (i.e. 3.5).");
        }
    }

    // make sure domain was defined
    //
    if(!f_opts.is_defined("domain"))
    {
        throw ed::invalid_parameter("a domain name is required.");
    }
    f_domain = f_opts.get_string("domain");

    // get the certificate
    //
    ed::certificate cert;
    if(!cert.load_from_domain(f_domain))
    {
        std::cerr
            << "error: failed to load the certificate from \""
            << f_domain
            << "\".\n";
        return 1;
    }

    // check whether it's under the limit
    //
    snapdev::timespec_ex const not_after(cert.get_not_after());
    snapdev::timespec_ex const now(snapdev::now());
    snapdev::timespec_ex const time_left(not_after - now);
    double const secs(time_left.to_sec());
    double const days(secs / (24.0 * 60.0 * 60.0));

    // if --info was specified show the certificate info
    //
    if(f_info)
    {
        print("domain", f_domain);
        print("not-before", cert.get_not_before().to_string("%Y/%m/%d %H:%M:%S", true));
        print("not-after", cert.get_not_after().to_string("%Y/%m/%d %H:%M:%S", true));

        print("issuer-common-name", cert.get_issuer_common_name());
        print("issuer-country-name", cert.get_issuer_country_name());
        print("issuer-locality-name", cert.get_issuer_locality_name());
        print("issuer-state-or-province-name", cert.get_issuer_state_or_province_name());
        print("issuer-organization-name", cert.get_issuer_organization_name());
        print("issuer-organizational-unit", cert.get_issuer_organizational_unit());
        print("issuer-email-address", cert.get_issuer_email_address());

        print("subject-common-name", cert.get_subject_common_name());
        print("subject-country-name", cert.get_subject_country_name());
        print("subject-locality-name", cert.get_subject_locality_name());
        print("subject-state-or-province-name", cert.get_subject_state_or_province_name());
        print("subject-organization-name", cert.get_subject_organization_name());
        print("subject-organizational-unit", cert.get_subject_organizational_unit());
        print("subject-email-address", cert.get_subject_email_address());
    }

    return days >= f_limit ? 0 : 1;
}


void check_certificate::print(char const * field_name, std::string const & value)
{
    if(!value.empty())
    {
        std::cout
            << field_name
            << ": "
            << value
            << '\n';
    }
}





int main(int argc, char * argv[])
{
    ed::signal_handler::create_instance();

    try
    {
        check_certificate cc(argc, argv);
        return cc.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(e.code());
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL
            << "an exception occurred (1): "
            << e.what()
            << SNAP_LOG_SEND;
        exit(1);
    }
    catch(...)
    {
        SNAP_LOG_FATAL
            << "an unknown exception occurred (2)."
            << SNAP_LOG_SEND;
        exit(2);
    }
    snapdev::NOT_REACHED();
}


// vim: ts=4 sw=4 et

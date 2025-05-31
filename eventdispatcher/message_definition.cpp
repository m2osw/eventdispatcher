// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Handle message definitions.
 *
 * The library supports loading message definitions from files. Those
 * definitions are useful to make sure a message includes all the
 * required parameters, does not include certain forbidden parameters,
 * and that parameters have a value that matches its type.
 *
 * This implements the get_message_definition() and with time probably
 * many other functions. For now, the check is done in the message.cpp
 * which is probably not the right place...
 */


// self
//
#include    "eventdispatcher/message_definition.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/message.h"


// advgetopt
//
#include    <advgetopt/conf_file.h>


// cppthread
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>


// snapdev
//
#include    <snapdev/pathinfo.h>
#include    <snapdev/tokenize_string.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



namespace
{



/** \brief The extension used to check for message definitions.
 *
 * This prefix is appened to the filename before we try to read
 * message definitions for a given message.
 */
char const * const g_message_definition_suffix = ".conf";


/** \brief The paths to the list of message definitions.
 *
 * This variable is dynamically set when you call the
 * process_message_definition_options() function.
 *
 * The variable supports any number of paths separated by
 * a colon (:). In most cases, you don't need more than one
 * path, but tests usually require at least two: the folder
 * where the message definitions get installed
 * (`BUILD/Debug/dist/eventdispatcher/messages`) and the location
 * of the messages this very project defines.
 */
std::string g_message_definition_paths = std::string();


/** \brief The list of loaded message definitions.
 *
 * We load message definitions the first time we receive a message.
 * The definitions are then stored in this map until the application
 * is done.
 */
message_definition::map_t   g_message_definitions;


/** \brief Options to handle the message definition.
 *
 * At the moment, this gives the user the ability to define the
 * path to the definitions. This is really useful for the programmers
 * since the definitions are under the BUILD folder rather than
 * `/usr/share/...`
 */
advgetopt::option const g_options[] =
{
    // MESSAGE DEFINITION OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("path-to-message-definitions")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("the path to the message definitions used to verify message validity before dispatching them.")
        , advgetopt::DefaultValue("/usr/share/eventdispatcher/messages")
    ),

    // END
    //
    advgetopt::end_options()
};



} // no name namespace



/** \brief Add message definition specific command line options.
 *
 * This function adds the message definition command line options
 * to your \p opts variable.
 *
 * This needs to be called before you parse the argv array of command
 * line options.
 *
 * \param[in,out] opts  The option array to dynamically update with the
 * message definition command line options.
 */
void add_message_definition_options(advgetopt::getopt & opts)
{
    opts.parse_options_info(g_options, true);
}


/** \brief Retrieve the command line parameters.
 *
 * This function needs to be called before the get_message_definition()
 * function gets called in order to properly setup the path to
 * the list of message definitions.
 *
 * \param[in] opts  The advgetopt options.
 */
void process_message_definition_options(advgetopt::getopt const & opts)
{
    g_message_definition_paths = opts.get_string("path-to-message-definitions");
}


/** \brief Set the list of paths to message definitions.
 *
 * In most cases, you want to use the add_message_definition_options()
 * and the process_message_definition_options() functions. Those handle
 * command line options so the user can use the --path-to-message-definitions
 * option on their command line.
 *
 * When working on tests, however, it happens that this is not a good
 * option and having direct access to the variable is much easier.
 * To make the test more robuts, it is better to call this function
 * again once done in order to clear the paths:
 *
 * \code
 *     ed::set_message_definition_paths("my:paths:here");
 *     ... run test ...
 *     ed::set_message_definition_paths(std::string());
 * \endcode
 *
 * Or use the RAII clas proposed:
 *
 * \code
 *     ed::manage_message_definition_paths mdp("my:paths:here");
 *     ... run test ...
 * \endcode
 */
void set_message_definition_paths(std::string const & paths)
{
    g_message_definition_paths = paths;
}


/** \brief Read definitions of a message.
 *
 * This function searches for a file named \p command plus the .conf
 * suffix in the message definition directory
 * (`/usr/share/eventdispatcher/messages`).
 *
 * \note
 * If a package gets installed later with a command definition which we
 * missed loading here, then that command will continue to not be tested
 * (as in the newly instaleld definition will be ignored). These definitions
 * will be taken in account on a restart of any services making use of them.
 *
 * \warning
 * This function is expected to be called with the real message command
 * and not the f_match.f_expr which could be a regular expression.
 *
 * \todo
 * Add support for includes.
 *
 * \param[in] command  Name of the message definition to load.
 *
 * \return A pointer to the loaded message definition.
 */
message_definition::pointer_t get_message_definition(std::string const & command)
{
    if(g_message_definition_paths.empty())
    {
        throw invalid_parameter(
              "message definition:"
            + command
            + ": no paths defined to message definitions. (i.e. did you call the add_message_definition_options() and process_message_definition_options() functions?)");
    }

    verify_message_name(command);

    cppthread::guard lock(*cppthread::g_system_mutex);

    auto it(g_message_definitions.find(command));
    if(it != g_message_definitions.end())
    {
        return it->second;
    }

    message_definition::pointer_t def(std::make_shared<message_definition>());
    def->f_command = command;
    g_message_definitions[command] = def;

    std::list<std::string> paths;
    snapdev::tokenize_string(
          paths
        , g_message_definition_paths
        , ":"
        , true);

    bool found(false);
    for(auto const & p : paths)
    {
        std::string filename(p);
        filename += '/';
        filename += command;
        filename += g_message_definition_suffix;

        if(!snapdev::pathinfo::file_exists(filename))
        {
            // no such file, just return the default message definition
            //
            continue;
        }
        found = true;

        advgetopt::conf_file_setup setup(filename);
        advgetopt::conf_file::pointer_t config(advgetopt::conf_file::get_conf_file(setup));
        advgetopt::conf_file::sections_t sections(config->get_sections());
        for(auto const & s : sections)
        {
            // section names use dashes between words
            // our messages use underscores
            //
            message_parameter param = {
                .f_name = advgetopt::option_with_underscores(s),
            };

            std::string param_name(s + "::type");
            if(config->has_parameter(param_name))
            {
                std::string const & type(config->get_parameter(param_name));
                if(type == "default"
                || type == "string")
                {
                    param.f_type = parameter_type_t::PARAMETER_TYPE_STRING;
                }
                else if(type == "integer")
                {
                    param.f_type = parameter_type_t::PARAMETER_TYPE_INTEGER;
                }
                else if(type == "address")
                {
                    param.f_type = parameter_type_t::PARAMETER_TYPE_ADDRESS;
                }
                else if(type == "timespec")
                {
                    param.f_type = parameter_type_t::PARAMETER_TYPE_TIMESPEC;
                }
                else
                {
                    throw invalid_parameter(
                          "message definition:"
                        + command
                        + ": parameter type \""
                        + type
                        + "\" is not a supported type.");
                }
            }

            param_name = s + "::flags";
            if(config->has_parameter(param_name))
            {
                // start from a clean slate
                //
                param.f_flags = 0;

                std::string const & flags(config->get_parameter(param_name));
                std::list<std::string> flag_names;
                snapdev::tokenize_string(flag_names, flags, ",", true);
                for(auto const & f : flag_names)
                {
                    if(f == "required")
                    {
                        param.f_flags |= PARAMETER_FLAG_REQUIRED;
                    }
                    else if(f == "empty")
                    {
                        param.f_flags |= PARAMETER_FLAG_EMPTY;
                    }
                    else if(f == "forbidden")
                    {
                        param.f_flags |= PARAMETER_FLAG_FORBIDDEN;
                    }
                    else if(f != "optional"
                         && f != "defined"
                         && f != "allowed")
                    {
                        throw invalid_parameter(
                              "message definition:"
                            + command
                            + ": parameter flag \""
                            + f
                            + "\" not supported.");
                    }
                }
            }

            def->f_parameters.push_back(param);
        }

        // we only read one config file; other copies in the
        // other folders are ignored
        //
        break;
    }

#ifdef _DEBUG
    if(!found)
    {
        throw invalid_parameter(
              "message definition for \""
            + command
            + "\" not found.");
    }
#endif

    return def;
}



} // namespace ed
// vim: ts=4 sw=4 et

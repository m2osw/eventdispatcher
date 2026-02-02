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

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/message_definition.h>
#include    <eventdispatcher/tcp_bio_options.h>
#include    <eventdispatcher/version.h>


// libexcept
//
#include    <libexcept/exception.h>


// snaplogger
//
#include    <snaplogger/logger.h>


// C++
//
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{


char **         g_argv = nullptr;


} // SNAP_CATCH2_NAMESPACE namespace


int setup_test(Catch::Session & session)
{
    snapdev::NOT_USED(session);

    // at this point, I setup all the paths in one place
    //
    // note that it is possible to change this list at any time
    //
    // WARNING: the order matters, we want to test with our source
    //          (i.e. original) files first
    //
    ed::set_message_definition_paths(
        SNAP_CATCH2_NAMESPACE::g_source_dir() + "/tests/message-definitions:"
            + SNAP_CATCH2_NAMESPACE::g_source_dir() + "/eventdispatcher/message-definitions:"
            + SNAP_CATCH2_NAMESPACE::g_dist_dir() + "/share/eventdispatcher/messages");

    return 0;
}


int main(int argc, char * argv[])
{
    SNAP_CATCH2_NAMESPACE::g_argv = argv;

    snaplogger::logger::pointer_t l(snaplogger::logger::get_instance());
    l->add_console_appender();
    l->set_severity(snaplogger::severity_t::SEVERITY_ALL);
    snaplogger::mark_ready(); // we do not process options, so we have to explicitly call mark_ready()

    ed::bio_auto_cleanup auto_bio_cleanup;

    return SNAP_CATCH2_NAMESPACE::snap_catch2_main(
              "eventdispatcher"
            , EVENTDISPATCHER_VERSION_STRING
            , argc
            , argv
            , []() { libexcept::set_collect_stack(libexcept::collect_stack_t::COLLECT_STACK_NO); }
            , nullptr
            , &setup_test
        );
}


// vim: ts=4 sw=4 et

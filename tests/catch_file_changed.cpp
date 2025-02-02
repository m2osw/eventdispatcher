// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/file_changed.h>

//#include    <eventdispatcher/local_stream_server_client_message_connection.h>
//#include    <eventdispatcher/local_stream_server_connection.h>
//#include    <eventdispatcher/dispatcher.h>


// cppthread
//
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



bool                            g_event_processed = false;


typedef std::function<void()>   tweak_callback_t;


class tweak_files
    : public cppthread::runner
{
public:
    typedef std::shared_ptr<tweak_files>        pointer_t;

                                tweak_files(
                                      std::string const & name
                                    , tweak_callback_t callback);

    virtual void                run() override;

private:
    tweak_callback_t            f_callback = tweak_callback_t();
};


tweak_files::tweak_files(
          std::string const & name
        , tweak_callback_t callback)
    : runner(name)
    , f_callback(callback)
{
}


void tweak_files::run()
{
    f_callback();
}






class file_listener
    : public ed::file_changed
{
public:
    typedef std::shared_ptr<file_listener>        pointer_t;

                                file_listener();
    virtual                     ~file_listener();

    void                        add_expected(
                                      std::string const & watched_path
                                    , ed::file_event_mask_t events
                                    , std::string const & filename);
    void                        run_test(
                                      std::string const & name
                                    , tweak_callback_t callback);

    // implementation of file_changed
    //
    virtual void                process_event(ed::file_event const & watch_event) override;
    virtual void                process_timeout() override;

private:
    std::list<ed::file_event>   f_expected = std::list<ed::file_event>();
    tweak_files::pointer_t      f_tweak_files = tweak_files::pointer_t();
    cppthread::thread::pointer_t
                                f_thread = cppthread::thread::pointer_t();
};









file_listener::file_listener()
{
    set_name("file-listener");

    g_event_processed = false;
}


file_listener::~file_listener()
{
    g_event_processed = f_expected.empty();
}


void file_listener::process_event(ed::file_event const & watch_event)
{
    // if the vector is empty, then we received more events than expected
    // and there is a bug somewhere (test or implementation)
    //
    // Note: this can happen because once the vector is empty we wait
    //       another second to know whether we're done or not
    //
    CATCH_REQUIRE_FALSE(f_expected.empty());

    CATCH_REQUIRE(f_expected.front().get_watched_path() == watch_event.get_watched_path());
    CATCH_REQUIRE(f_expected.front().get_events()       == watch_event.get_events());
    CATCH_REQUIRE(f_expected.front().get_filename()     == watch_event.get_filename());

    f_expected.pop_front();

    if(f_expected.empty())
    {
        // wait another 3 seconds to make sure that no more events occur
        // after the last one
        //
        set_timeout_delay(3'000'000);
    }
}


void file_listener::process_timeout()
{
    remove_from_communicator();
}


void file_listener::add_expected(
      std::string const & watched_path
    , ed::file_event_mask_t events
    , std::string const & filename)
{
    f_expected.push_back(ed::file_event(watched_path, events, filename));
}


void file_listener::run_test(
      std::string const & name
    , tweak_callback_t callback)
{
    f_tweak_files = std::make_shared<tweak_files>(name, callback);
    f_thread = std::make_shared<cppthread::thread>(name, f_tweak_files);
    f_thread->start();
}



} // no name namespace



CATCH_TEST_CASE("file_changed_events", "[file_changed]")
{
    CATCH_START_SECTION("file_changed_events: attributes")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        std::string const dir(SNAP_CATCH2_NAMESPACE::get_tmp_dir("attributes"));

        {
            file_listener::pointer_t listener(std::make_shared<file_listener>());
            listener->watch_files(dir, ed::SNAP_FILE_CHANGED_EVENT_ATTRIBUTES);

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_ATTRIBUTES
                | ed::SNAP_FILE_CHANGED_EVENT_DIRECTORY
                , std::string());

            communicator->add_connection(listener);

            int r(0);
            listener->run_test("attributes", [dir, &r]() {
                    sleep(rand() % 3);
                    r = chmod(dir.c_str(), 0770);
                });

            communicator->run();

            CATCH_REQUIRE(r == 0);
        }

        CATCH_REQUIRE(g_event_processed);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("file_changed_events: create, write, close file, then open, read, close, finally delete")
    {
        ed::communicator::pointer_t communicator(ed::communicator::instance());

        std::string const dir(SNAP_CATCH2_NAMESPACE::get_tmp_dir("file-changed"));
        std::string const filename(dir + "/test.txt");

        {
            file_listener::pointer_t listener(std::make_shared<file_listener>());
            listener->watch_files(dir, ed::SNAP_FILE_CHANGED_EVENT_ALL);

            // create/write/close events
            //
            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_CREATED
                , "test.txt");

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_ACCESS
                , "test.txt");

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_WRITE
                , "test.txt");

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_ACCESS
                | ed::SNAP_FILE_CHANGED_EVENT_UPDATED
                , "test.txt");

            // open/read/close events
            //
            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_ACCESS
                , "test.txt");

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_READ
                , "test.txt");

            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_ACCESS
                , "test.txt");

            // delete events
            //
            listener->add_expected(
                  dir
                , ed::SNAP_FILE_CHANGED_EVENT_DELETED
                , "test.txt");

            communicator->add_connection(listener);

            int r(0);
            listener->run_test("file", [filename, &r]() {
                    std::string const message("this is a test file");
                    sleep(rand() % 3);

                    // create/write/close
                    {
                        std::ofstream out(filename);
                        out << message << std::endl;
                        if(r == 0)
                        {
                            r = out.fail() ? 1 : 0;
                        }
                    }

                    sleep(rand() % 3);

                    // open/read/close
                    {
                        std::ifstream in(filename);
                        std::string line;
                        std::getline(in, line, '\n');
                        if(r == 0)
                        {
                            r = in.fail() || line != message ? 1 : 0;
                        }
                    }

                    sleep(rand() % 3);

                    // delete
                    {
                        if(unlink(filename.c_str()) != 0)
                        {
                            r = 1;
                        }
                    }
                });

            communicator->run();

            CATCH_REQUIRE(r == 0);
        }

        CATCH_REQUIRE(g_event_processed);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et

// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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

// eventdispatcher
//
#include    <eventdispatcher/process_changed.h>
#include    <eventdispatcher/communicator.h>


// last include
//
#include    <snapdev/poison.h>



// the file listener uses inotify which is not (well? at all?) supported by /proc
//class file_listener
//    : public ed::file_changed
//{
//public:
//                        file_listener();
//                        file_listener(file_listener const &) = delete;
//    file_listener const &
//                        operator = (file_listener const &) = delete;
//
//    // file_changed implementation
//    //
//    virtual void        process_event(ed::file_event const & watch_event) override;
//
//private:
//};
//
//
//file_listener::file_listener()
//{
//    //std::string const path("/proc"); -- this does not work (because /proc is a special mounted file system)
//    std::string const path("/home/alexis");
//
//    watch_file(
//              path
//            , ed::SNAP_FILE_CHANGED_EVENT_UPDATED
//            | ed::SNAP_FILE_CHANGED_EVENT_WRITE);
//}
//
//
//void file_listener::process_event(ed::file_event const & watch_event)
//{
//std::cerr << "--- received event: "
//<< watch_event.get_watched_path()
//<< " -- "
//<< std::hex << watch_event.get_events() << std::dec
//<< " -- "
//<< watch_event.get_filename()
//<< "\n";
//}


class process_listener
    : public ed::process_changed
{
public:
                        process_listener();
                        process_listener(process_listener const &) = delete;
    process_listener const &
                        operator = (process_listener const &) = delete;

    // process_changed implementation
    //
    virtual void        process_event(ed::process_changed_event const & event) override;

private:
};


process_listener::process_listener()
{
}


void process_listener::process_event(ed::process_changed_event const & event)
{
    std::cout << "--- process event: "
        << event.get_event()
        << " cpu: "
        << event.get_cpu()
        << " timestamp: "
        << event.get_timestamp()
        << " ("
        << event.get_realtime().to_string("%D %T.%N") // note: this is always in local time
        << ") pid: "
        << event.get_pid() << "/" << event.get_tgid()
        << " ppid: "
        << event.get_parent_pid() << "/" << event.get_parent_tgid()
        << " uid: "
        << static_cast<int32_t>(event.get_ruid()) << "/" << static_cast<int32_t>(event.get_euid())
        << " gid: "
        << static_cast<int32_t>(event.get_rgid()) << "/" << static_cast<int32_t>(event.get_egid())
        << " command: "
        << event.get_command()
        << " exit: "
        << event.get_exit_code() << "/" << event.get_exit_signal()
        << "\n";
}


int main(int argc, char * argv[])
{
    if(argc > 1)
    {
        if(strcmp(argv[1], "--help") == 0
        || strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage: process-listener\n";
            return 1;
        }

        std::cerr << "error: no command line parameters are supported.\n";
        return 1;
    }

    process_listener::pointer_t listen(std::make_shared<process_listener>());

    ed::communicator::pointer_t communicator(ed::communicator::instance());
    communicator->add_connection(listen);

    communicator->run();

    return 0;
}

// vim: ts=4 sw=4 et

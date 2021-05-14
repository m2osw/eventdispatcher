// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */


// self
//
#include    "eventdispatcher/file_changed.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <algorithm>
#include    <cstring>


// C lib
//
#include    <sys/inotify.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{


file_changed::event_t::event_t(std::string const & watched_path
                                  , event_mask_t events
                                  , std::string const & filename)
    : f_watched_path(watched_path)
    , f_events(events)
    , f_filename(filename)
{
    if(f_watched_path.empty())
    {
        throw event_dispatcher_initialization_error("a file_changed watch path cannot be the empty string.");
    }

    if(f_events == SNAP_FILE_CHANGED_EVENT_NO_EVENTS)
    {
        throw event_dispatcher_initialization_error("a file_changed events parameter cannot be 0.");
    }
}


std::string const & file_changed::event_t::get_watched_path() const
{
    return f_watched_path;
}


file_changed::event_mask_t file_changed::event_t::get_events() const
{
    return f_events;
}


std::string const & file_changed::event_t::get_filename() const
{
    return f_filename;
}


bool file_changed::event_t::operator < (event_t const & rhs) const
{
    return f_watched_path < rhs.f_watched_path;
}


file_changed::watch_t::watch_t()
{
}


file_changed::watch_t::watch_t(std::string const & watched_path, event_mask_t events, uint32_t add_flags)
    : f_watched_path(watched_path)
    , f_events(events)
    , f_mask(events_to_mask(events) | add_flags | IN_EXCL_UNLINK)
{
}


void file_changed::watch_t::add_watch(int inotify)
{
    f_watch = inotify_add_watch(inotify, f_watched_path.c_str(), f_mask);
    if(f_watch == -1)
    {
        int const e(errno);
        SNAP_LOG_WARNING
            << "inotify_add_watch() returned an error (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;

        // it did not work
        //
        throw event_dispatcher_initialization_error("inotify_add_watch() failed");
    }
}


void file_changed::watch_t::merge_watch(int inotify, event_mask_t const events)
{
    f_mask |= events_to_mask(events);

    // The documentation is not 100% clear about an update so for now
    // I remove the existing watch and create a new one... it should
    // not happen very often anyway
    //
    if(f_watch != -1)
    {
        remove_watch(inotify);
    }

    f_watch = inotify_add_watch(inotify, f_watched_path.c_str(), f_mask);
    if(f_watch == -1)
    {
        int const e(errno);
        SNAP_LOG_WARNING
            << "inotify_raddwatch() returned an error (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;

        // it did not work
        //
        throw event_dispatcher_initialization_error("inotify_add_watch() failed");
    }
}


void file_changed::watch_t::remove_watch(int inotify)
{
    if(f_watch != -1)
    {
        int const r(inotify_rm_watch(inotify, f_watch));
        if(r != 0)
        {
            // we output the error if one occurs, but go on as if nothing
            // happened
            //
            int const e(errno);
            SNAP_LOG_WARNING
                << "inotify_rm_watch() returned an error (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
        }

        // we can remove it just once
        //
        f_watch = -1;
    }
}


file_changed::file_changed()
    : f_inotify(inotify_init1(IN_NONBLOCK | IN_CLOEXEC))
{
    if(f_inotify == -1)
    {
        throw event_dispatcher_initialization_error("file_changed: inotify_init1() failed.");
    }
}


file_changed::~file_changed()
{
    // watch_t are not RAII because we copy them in maps...
    // so we have to "manually" clean up here, but making them RAII would
    // mean creating an impl and thus hiding the problem at a different
    // level which is less effective...
    //
    for(auto & w : f_watches)
    {
        w.second.remove_watch(f_inotify);
    }

    close(f_inotify);
}


/** \brief Try to merge a new watch.
 *
 * If you attempt to watch the same path again, instead of adding a new watch,
 * we instead want to merge it. This is important because the system
 * does not generate a new watch when you do that.
 *
 * In this case, the \p events parameter is viewed as parameters being
 * added to the watched. If you want to replace the previous watch instead,
 * make sure to first remove it, then re-add it with new flags as required.
 *
 * \param[in] watched_path  The path the user wants to watch.
 * \param[in] events  The events being added to the watch.
 */
bool file_changed::merge_watch(std::string const & watched_path, event_mask_t const events)
{
    auto const & wevent(std::find_if(
              f_watches.begin()
            , f_watches.end()
            , [&watched_path](auto const & w)
            {
                return w.second.f_watched_path == watched_path;
            }));
    if(wevent == f_watches.end())
    {
        // not found
        //
        return false;
    }

    wevent->second.merge_watch(f_inotify, events);

    return true;
}


void file_changed::watch_file(std::string const & watched_path, event_mask_t const events)
{
    if(!merge_watch(watched_path, events))
    {
        watch_t watch(watched_path, events, 0);
        watch.add_watch(f_inotify);
        f_watches[watch.f_watch] = watch;
    }
}


void file_changed::watch_symlink(std::string const & watched_path, event_mask_t const events)
{
    if(!merge_watch(watched_path, events))
    {
        watch_t watch(watched_path, events, IN_DONT_FOLLOW);
        watch.add_watch(f_inotify);
        f_watches[watch.f_watch] = watch;
    }
}


void file_changed::watch_directory(std::string const & watched_path, event_mask_t const events)
{
    if(!merge_watch(watched_path, events))
    {
        watch_t watch(watched_path, events, IN_ONLYDIR);
        watch.add_watch(f_inotify);
        f_watches[watch.f_watch] = watch;
    }
}


void file_changed::stop_watch(std::string const & watched_path)
{
    // because of the merge, even though the watched_path is not the
    // index of our map, it will be unique so we really only need to
    // find one such entry
    //
    auto wevent(std::find_if(
                     f_watches.begin()
                   , f_watches.end()
                   , [&](auto & w)
                   {
                       return w.second.f_watched_path == watched_path;
                   }));

    if(wevent != f_watches.end())
    {
        wevent->second.remove_watch(f_inotify);
        f_watches.erase(wevent);
    }
}


bool file_changed::is_reader() const
{
    return true;
}


int file_changed::get_socket() const
{
    // if we did not add any watches, avoid adding another fd to the poll()
    //
    if(f_watches.empty())
    {
        return -1;
    }

    return f_inotify;
}


void file_changed::set_enable(bool enabled)
{
    connection::set_enable(enabled);

    // TODO: inotify will continue to send us messages when disabled
    //       and that's a total of 16K of messages! That's a lot of
    //       memory wasted if the connection gets disabled for a long
    //       amount of time; what we want to do instead is disconnect
    //       completely on a disable and reconnect on a re-enable
}


void file_changed::process_read()
{
    // were notifications closed in between?
    //
    if(f_inotify == -1)
    {
        return;
    }

    // WARNING: this is about 4Kb of buffer on the stack
    //          it is NOT 256 structures because all events with a name
    //          have the name included in themselves and that "eats"
    //          space in the next structure
    //
    struct inotify_event buffer[256];

    for(;;)
    {
        // read a few messages in one call
        //
        ssize_t const len(read(f_inotify, buffer, sizeof(buffer)));
        if(len <= 0)
        {
            if(len == 0
            || errno == EAGAIN)
            {
                // reached the end of the current queue
                //
                return;
            }

            // TODO: close the inotify on errors?
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while reading from inotify (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
        // convert the buffer to a character pointer to make it easier to
        // move the pointer to the next structure
        //
        char const * start(reinterpret_cast<char const *>(buffer));
        char const * end(start + len);
        while(start < end)
        {
            // get the pointer to the current inotify event
            //
            struct inotify_event const & ievent(*reinterpret_cast<struct inotify_event const *>(start));
            if(start + sizeof(struct inotify_event) + ievent.len > end)
            {
                // unless there is a huge bug in the inotify implementation
                // this exception should never happen
                //
                throw event_dispatcher_unexpected_data("somehow the size of this ievent does not match what we just read.");
            }

            // convert the inotify even in one of our events
            //
            auto const & wevent(f_watches.find(ievent.wd));
            if(wevent != f_watches.end())
            {
                // XXX: we need to know whether this flag can appear with
                //      others (i.e. could we at the same time have a message
                //      saying there was a read and a queue overflow?); if
                //      so, then we need to run the else part even on
                //      overflows
                //
                if((ievent.mask & IN_Q_OVERFLOW) != 0)
                {
                    SNAP_LOG_ERROR
                        << "Received an event queue overflow error."
                        << SNAP_LOG_SEND;
                }
                else
                {
                    event_t const watch_event(wevent->second.f_watched_path
                                            , mask_to_events(ievent.mask)
                                            , std::string(ievent.name, ievent.len));

                    process_event(watch_event);

                    // if the event received included IN_IGNORED then we need
                    // to remove that watch
                    //
                    if((ievent.mask & IN_IGNORED) != 0)
                    {
                        // before losing the wevent, make sure we disconnect
                        // from the OS version
                        //
                        const_cast<watch_t &>(wevent->second).remove_watch(f_inotify);
                        f_watches.erase(ievent.wd);
                        f_watches.erase(wevent);
                    }
                }
            }
            else
            {
                // we do not know about this notifier, close it
                // (this should never happen... unless we read the queue
                // for a watch that had more events and we had not read it
                // yet, in that case the watch was certainly already
                // removed... it should not hurt to re-remove it.)
                //
                inotify_rm_watch(f_inotify, ievent.wd);
            }

            // move the pointer to the next stucture until we reach 'end'
            //
            start += sizeof(struct inotify_event) + ievent.len;
        }
    }
}


uint32_t file_changed::events_to_mask(event_mask_t const events)
{
    uint32_t mask(0);

    if((events & SNAP_FILE_CHANGED_EVENT_ATTRIBUTES) != 0)
    {
        mask |= IN_ATTRIB;
    }

    if((events & SNAP_FILE_CHANGED_EVENT_READ) != 0)
    {
        mask |= IN_ACCESS;
    }

    if((events & SNAP_FILE_CHANGED_EVENT_WRITE) != 0)
    {
        mask |= IN_MODIFY;
    }

    if((events & SNAP_FILE_CHANGED_EVENT_CREATED) != 0)
    {
        mask |= IN_CREATE | IN_MOVED_FROM | IN_MOVE_SELF;
    }

    if((events & SNAP_FILE_CHANGED_EVENT_DELETED) != 0)
    {
        mask |= IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO | IN_MOVE_SELF;
    }

    if((events & SNAP_FILE_CHANGED_EVENT_ACCESS) != 0)
    {
        mask |= IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE;
    }

    if(mask == 0)
    {
        throw event_dispatcher_initialization_error("invalid file_changed events parameter, it was not changed to any IN_... flags.");
    }

    return mask;
}


file_changed::event_mask_t file_changed::mask_to_events(uint32_t const mask)
{
    event_mask_t events(0);

    if((mask & IN_ATTRIB) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_ATTRIBUTES;
    }

    if((mask & IN_ACCESS) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_READ;
    }

    if((mask & IN_MODIFY) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_WRITE;
    }

    if((mask & (IN_CREATE | IN_MOVED_FROM)) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_CREATED;
    }

    if((mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_TO)) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_DELETED;
    }

    if((mask & (IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_ACCESS;
    }

    // return flags only
    //
    if((mask & IN_ISDIR) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_DIRECTORY;
    }

    if((mask & IN_IGNORED) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_GONE;
    }

    if((mask & IN_UNMOUNT) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_UNMOUNTED;
    }

    return events;
}



} // namespace ed
// vim: ts=4 sw=4 et

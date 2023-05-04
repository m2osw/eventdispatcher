// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the file_changed class.
 *
 * This class is a wrapper around the inotify library. It allows you to
 * listen to various changes to files.
 */


// self
//
#include    "eventdispatcher/file_changed.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <algorithm>
#include    <cstring>


// C
//
#include    <sys/inotify.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{


namespace
{


constexpr char const * const    g_no_path = "/";


} // no name namepsace


file_event::file_event(
          std::string const & watched_path
        , file_event_mask_t events
        , std::string const & filename)
    : f_watched_path(watched_path)
    , f_events(events)
    , f_filename(filename)
{
    if(f_watched_path.empty())
    {
        throw initialization_error("a file_changed watch path cannot be the empty string.");
    }

    if(f_events == SNAP_FILE_CHANGED_EVENT_NO_EVENTS)
    {
        throw initialization_error("a file_changed events parameter cannot be 0.");
    }
}


std::string const & file_event::get_watched_path() const
{
    return f_watched_path;
}


file_event_mask_t file_event::get_events() const
{
    return f_events;
}


std::string const & file_event::get_filename() const
{
    return f_filename;
}


bool file_event::operator < (file_event const & rhs) const
{
    return f_watched_path < rhs.f_watched_path;
}








file_changed::watch_t::watch_t()
{
}


file_changed::watch_t::watch_t(
          std::string const & watched_path
        , file_event_mask_t events
        , uint32_t add_flags)
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
        SNAP_LOG_FATAL
            << "inotify_add_watch() returned an error (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")."
            << SNAP_LOG_SEND;

        // it did not work
        //
        throw initialization_error("inotify_add_watch() failed");
    }
}


void file_changed::watch_t::merge_watch(int inotify, file_event_mask_t const events)
{
    f_mask |= events_to_mask(events);

    // The documentation is not 100% clear about an update so for now
    // I remove the existing watch and create a new one... it should
    // not happen very often anyway
    //
    // TODO: actually the docs clearly say that if it already exists, further
    //       calls to the inotify_add_watch() update the watch -- also
    //       deleting & recreating is not going to be atomic; anything that
    //       happens in between will be lost
    //
    if(f_watch != -1)
    {
        remove_watch(inotify);
    }

    f_watch = inotify_add_watch(inotify, f_watched_path.c_str(), f_mask);
    if(f_watch == -1)
    {
        int const e(errno);
        std::stringstream ss;
        ss << "inotify_add_watch() returned an error (errno: "
           << e
           << " -- "
           << strerror(e)
           << ").";
        initialization_error err(ss.str());
        SNAP_LOG_FATAL
            << err
            << SNAP_LOG_SEND;

        // it did not work
        //
        throw err;
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
        throw initialization_error("file_changed: inotify_init1() failed.");
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
 * the new events get added to the existing instance. This is important
 * because the operating system does not generate a new watch when you do that.
 *
 * In this case, the \p events parameter is viewed as parameters being
 * added to the existing watched. If you instead want to replace the
 * previous watch, make sure to first remove it, then re-add it with
 * new flags as required.
 *
 * \warning
 * The current implementation is not atomic. If you change an existing
 * watch in whichever way, there is a small period of time when the
 * watch gets turned off. This means you may lose events.
 *
 * \warning
 * The project is not thread safe. You have to make sure that watches
 * get added by one thread at a time.
 *
 * \param[in] watched_path  The path the user wants to watch.
 * \param[in] events  The events being added to the watch.
 *
 * \return true if the merge happened.
 */
bool file_changed::merge_watch(
      std::string const & watched_path
    , file_event_mask_t const events)
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


void file_changed::watch_file(std::string const & watched_path, file_event_mask_t const events)
{
    if(!merge_watch(watched_path, events))
    {
        watch_t watch(watched_path, events, 0);
        watch.add_watch(f_inotify);
        f_watches[watch.f_watch] = watch;
    }
}


void file_changed::watch_symlink(std::string const & watched_path, file_event_mask_t const events)
{
    if(!merge_watch(watched_path, events))
    {
        watch_t watch(watched_path, events, IN_DONT_FOLLOW);
        watch.add_watch(f_inotify);
        f_watches[watch.f_watch] = watch;
    }
}


void file_changed::watch_directory(std::string const & watched_path, file_event_mask_t const events)
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
    // Note: at the moment the only close happens in the destructor so this
    //       just can't happen; but I think that at some point it will on
    //       some errors and then we have to test the fd like this
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
                throw unexpected_data("somehow the size of this ievent does not match what we just read.");
            }

            // WARNING: the ievent.len is the total length including
            //          the '\0' characters; so we cannot use it
            //          to create the string in C++; however, if len
            //          is 0, then we must pass an empty string which
            //          the following implements
            //
            std::string filename;
            if(ievent.len > 0)
            {
                filename = ievent.name;
            }

            if(ievent.wd == -1)
            {
                // an error occured, we need special handling
                //
                if((ievent.mask & IN_Q_OVERFLOW) != 0)
                {
                    // the number of events in the queue is defined in:
                    //
                    //     /proc/sys/fs/inotify/max_queued_events
                    //
                    // (however, it may be the maximum size of the queue in
                    // bytes rather than the number of events...)
                    //
                    SNAP_LOG_RECOVERABLE_ERROR
                        << "Received an event queue overflow error."
                        << SNAP_LOG_SEND;

                    // on an overflow we need to verify all the files
                    // "manually" for changes (like one would do on
                    // startup) so the user needs to receive an event
                    // about that
                    //
                    file_event const watch_event(
                              g_no_path
                            , SNAP_FILE_CHANGED_EVENT_LOST_SYNC
                            , filename);
                    process_event(watch_event);
                }
                else
                {
                    SNAP_LOG_ERROR
                        << "Received an unknown error on the queue."
                        << SNAP_LOG_SEND;

                    // we do not know about this error, emit a generic error
                    // event to the user
                    //
                    file_event const watch_event(
                              g_no_path
                            , SNAP_FILE_CHANGED_EVENT_ERROR
                            , filename);
                    process_event(watch_event);
                }
            }
            else
            {
                // convert the inotify event in one of our events
                //
                auto const & wevent(f_watches.find(ievent.wd));
                if(wevent != f_watches.end())
                {
                    file_event const watch_event(wevent->second.f_watched_path
                                            , mask_to_events(ievent.mask)
                                            , filename);
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
                else
                {
                    // we do not know about this notifier, close it
                    // (this should never happen... unless we read the queue
                    // for a watch that had more events and we had not read it
                    // yet, in that case the watch was certainly already
                    // removed by the user... it should not hurt to re-remove it.)
                    //
                    inotify_rm_watch(f_inotify, ievent.wd);
                }
            }
            // move the pointer to the next structure until we reach 'end'
            //
            start += sizeof(struct inotify_event) + ievent.len;
        }
    }
}


uint32_t file_changed::events_to_mask(file_event_mask_t const events)
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

    if((events & SNAP_FILE_CHANGED_EVENT_UPDATED) != 0)
    {
        mask |= IN_CLOSE_WRITE;
    }

    if(mask == 0)
    {
        throw initialization_error("invalid file_changed events parameter, it was not changed to any IN_... flags.");
    }

    return mask;
}


file_event_mask_t file_changed::mask_to_events(uint32_t const mask)
{
    file_event_mask_t events(0);

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

    if((mask & IN_CLOSE_WRITE) != 0)
    {
        events |= SNAP_FILE_CHANGED_EVENT_UPDATED;
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

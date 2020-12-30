// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// self
//
#include    "eventdispatcher/connection.h"


// C++ lib
//
#include    <map>



namespace ed
{



class file_changed
    : public connection
{
public:
    typedef std::shared_ptr<file_changed>           pointer_t;
    typedef std::uint32_t                           event_mask_t;

    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_NO_EVENTS  = 0x0000;

    // bits added to watch_...() functions
    //
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ATTRIBUTES = 0x0001;   // chmod, chown (timestamp, link count, user/group, etc.)
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_READ       = 0x0002;   // read, execve
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_WRITE      = 0x0004;   // write, truncate
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_CREATED    = 0x0008;   // open & O_CREAT, rename, mkdir, link, symlink, bind
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_DELETED    = 0x0010;   // unlink, rename
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ACCESS     = 0x0020;   // open, close

    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_IO         = SNAP_FILE_CHANGED_EVENT_READ
                                                                   | SNAP_FILE_CHANGED_EVENT_WRITE;

    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ALL        = SNAP_FILE_CHANGED_EVENT_ATTRIBUTES
                                                                   | SNAP_FILE_CHANGED_EVENT_IO
                                                                   | SNAP_FILE_CHANGED_EVENT_CREATED
                                                                   | SNAP_FILE_CHANGED_EVENT_DELETED
                                                                   | SNAP_FILE_CHANGED_EVENT_ACCESS;

    // flags added in event_t objects
    //
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_DIRECTORY  = 0x1000;   // object is a directory
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_GONE       = 0x2000;   // removed
    static event_mask_t const   SNAP_FILE_CHANGED_EVENT_UNMOUNTED  = 0x4000;   // unmounted

    class event_t
    {
    public:
                                event_t(std::string const & watched_path
                                      , event_mask_t events
                                      , std::string const & filename);

        std::string const &     get_watched_path() const;
        event_mask_t            get_events() const;
        std::string const &     get_filename() const;

        bool                    operator < (event_t const & rhs) const;

    private:
        std::string             f_watched_path = std::string();
        event_mask_t            f_events       = 0;
        std::string             f_filename     = std::string();
    };

                                file_changed();
    virtual                     ~file_changed() override;

    void                        watch_file(std::string const & watched_path, event_mask_t const events);
    void                        watch_symlink(std::string const & watched_path, event_mask_t const events);
    void                        watch_directory(std::string const & watched_path, event_mask_t const events);

    void                        stop_watch(std::string const & watched_path);

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;
    virtual void                set_enable(bool enabled);
    virtual void                process_read() override;

    // new callback
    virtual void                process_event(event_t const & watch_event) = 0;

private:
    // TODO: RAII would be great with an impl and a counter...
    //       (i.e. we make copies at the moment.)
    struct watch_t
    {
        typedef std::map<int, watch_t>          map_t;

                                watch_t();
                                watch_t(std::string const & watched_path, event_mask_t events, uint32_t add_flags);

        void                    add_watch(int inotify);
        void                    merge_watch(int inotify, event_mask_t const events);
        void                    remove_watch(int inotify);

        std::string             f_watched_path = std::string();
        event_mask_t            f_events       = SNAP_FILE_CHANGED_EVENT_NO_EVENTS;
        uint32_t                f_mask         = 0;
        int                     f_watch        = -1;
    };

    bool                        merge_watch(std::string const & watched_path, event_mask_t const events);
    static uint32_t             events_to_mask(event_mask_t const events);
    static event_mask_t         mask_to_events(uint32_t const mask);

    int                         f_inotify = -1;
    watch_t::map_t              f_watches = watch_t::map_t();
};



} // namespace ed
// vim: ts=4 sw=4 et

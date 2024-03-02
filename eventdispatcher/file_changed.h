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
#pragma once

/** \file
 * \brief File changed class.
 *
 * Class used to handle file system events. This connection is useful to
 * quickly react to changes made to your file system. It also tracks
 * read access of a file.
 *
 * It is currently used in the snaprfs project to know when a file changed
 * and send it over to other computers in your cluster.
 *
 * \todo
 * Implement a "file_state" class which gives us a way to check the current
 * state of a file at any time.
 */

// self
//
#include    <eventdispatcher/connection.h>


// C++
//
#include    <map>
#include    <set>



namespace ed
{


typedef std::uint32_t                      file_event_mask_t;

static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_NO_EVENTS  = 0x0000;

// bits added to watch_...() functions
//
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_ATTRIBUTES = 0x0001;   // chmod, chown (timestamp, link count, user/group, etc.)
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_READ       = 0x0002;   // read, execve
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_WRITE      = 0x0004;   // write, truncate
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_CREATED    = 0x0008;   // open & O_CREAT, rename, mkdir, link, symlink, bind
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_DELETED    = 0x0010;   // unlink, rename
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_ACCESS     = 0x0020;   // open, close
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_UPDATED    = 0x0040;   // open + write/truncate + close (i.e. IN_CLOSE_WRITE)
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_EXISTS     = 0x0080;   // file exists (when adding a watch)
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_RECURSIVE  = 0x0100;   // auto-listen to sub-directories

// errors can always occur, whether you add them to your watch mask or not
//
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_LOST_SYNC  = 0x0400;   // queue overflow (some messages did not make it)
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_ERROR      = 0x0800;   // unknown error detected

static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_IO         = SNAP_FILE_CHANGED_EVENT_READ
                                                                              | SNAP_FILE_CHANGED_EVENT_WRITE;

static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_ALL        = SNAP_FILE_CHANGED_EVENT_ATTRIBUTES
                                                                              | SNAP_FILE_CHANGED_EVENT_IO
                                                                              | SNAP_FILE_CHANGED_EVENT_CREATED
                                                                              | SNAP_FILE_CHANGED_EVENT_DELETED
                                                                              | SNAP_FILE_CHANGED_EVENT_ACCESS;

// flags added in file_event objects
//
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_DIRECTORY  = 0x1000;   // object is a directory
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_GONE       = 0x2000;   // removed
static constexpr file_event_mask_t const   SNAP_FILE_CHANGED_EVENT_UNMOUNTED  = 0x4000;   // unmounted


class file_event
{
public:
                            file_event(
                                      std::string const & watched_path
                                    , file_event_mask_t events
                                    , std::string const & filename);

    std::string const &     get_watched_path() const;
    file_event_mask_t       get_events() const;
    std::string const &     get_filename() const;

    bool                    operator < (file_event const &) const;

private:
    std::string             f_watched_path = std::string();
    file_event_mask_t       f_events       = 0;
    std::string             f_filename     = std::string();
};



class file_changed
    : public connection
{
public:
    typedef std::shared_ptr<file_changed>           pointer_t;

                                file_changed();
    virtual                     ~file_changed() override;

    void                        watch_files(std::string const & watch_path, file_event_mask_t const events);
    void                        watch_symlinks(std::string const & watch_path, file_event_mask_t const events);
    void                        watch_directories(std::string const & watch_path, file_event_mask_t const events);

    void                        stop_watch(std::string watched_path);

    // connection implementation
    virtual bool                is_reader() const override;
    virtual int                 get_socket() const override;
    virtual void                set_enable(bool enabled);
    virtual void                process_read() override;

    // new callback
    virtual void                process_event(file_event const & watch_event) = 0;

private:
    // TODO: RAII would be great with an impl and a counter...
    //       (i.e. we make copies at the moment.)
    struct watch_t
    {
        typedef std::shared_ptr<watch_t>        pointer_t;
        typedef std::map<int, pointer_t>        map_t;

                                watch_t();
                                watch_t(
                                      std::string const & watched_path
                                    , std::string const & pattern
                                    , file_event_mask_t events
                                    , std::uint32_t add_flags);

        void                    add_watch(int inotify);
        void                    merge_watch(
                                      int inotify
                                    , std::string const & pattern
                                    , file_event_mask_t const events
                                    , std::uint32_t add_flags);
        void                    remove_watch(int inotify);
        bool                    match_patterns(std::string const & filename);

        std::string             f_watched_path = std::string();
        std::set<std::string>   f_patterns     = std::set<std::string>();
        file_event_mask_t       f_events       = SNAP_FILE_CHANGED_EVENT_NO_EVENTS;
        std::uint32_t           f_mask         = 0;
        int                     f_watch        = -1;
    };

    void                        merge_watch(
                                      std::string watched_path
                                    , file_event_mask_t const events
                                    , int flags);
    static uint32_t             events_to_mask(file_event_mask_t const events);
    static file_event_mask_t    mask_to_events(uint32_t const mask);
    bool                        path_and_pattern(std::string & path, std::string & pattern);

    int                         f_inotify = -1;
    watch_t::map_t              f_watches = watch_t::map_t();
};



} // namespace ed
// vim: ts=4 sw=4 et

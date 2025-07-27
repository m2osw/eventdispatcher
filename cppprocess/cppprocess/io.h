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
#pragma once


// snapdev
//
#include    <snapdev/callback_manager.h>


// C++
//
#include    <functional>
#include    <list>
#include    <memory>



namespace cppprocess
{



typedef std::uint32_t       io_flags_t;

constexpr io_flags_t        IO_FLAG_NONE    = 0;

constexpr io_flags_t        IO_FLAG_INPUT   = 0x0001;
constexpr io_flags_t        IO_FLAG_OUTPUT  = 0x0002;

enum class done_reason_t
{
    DONE_REASON_EOF,
    DONE_REASON_ERROR,
    DONE_REASON_INVALID,
    DONE_REASON_HUP,
};



// any input or output pipe or stream
class io
{
public:
    typedef std::shared_ptr<io>     pointer_t;
    typedef std::function<bool (io *, done_reason_t)>
                                    process_io_done_t;
    typedef snapdev::callback_manager<process_io_done_t>
                                    done_callbacks_t;

                            io(io_flags_t flags);
    virtual                 ~io();
                            io(io const &) = delete;
    io &                    operator = (io const &) = delete;

    io_flags_t              get_flags() const;

    done_callbacks_t::callback_id_t
                            add_process_done_callback(process_io_done_t done);
    bool                    remove_process_done_callback(done_callbacks_t::callback_id_t id);

    // new callbacks
    //
    virtual int             get_fd();
    virtual int             get_other_fd();
    virtual void            close_both();
    virtual void            close_other();
    virtual void            process_starting();
    virtual bool            process_done(done_reason_t reason);

private:
    io_flags_t              f_flags = IO_FLAG_NONE;
    done_callbacks_t        f_process_done = done_callbacks_t();
};



} // namespace cppprocess
// vim: ts=4 sw=4 et

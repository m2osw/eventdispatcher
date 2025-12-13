// Copyright (c) 2018-2025  Made to Order Software Corp.  All Rights Reserved
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

// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/fd_connection.h>



namespace ed
{


// see .cpp for implementation
namespace detail
{
class ncurses_impl;
}


class cui_connection
    : public ed::fd_connection
{
public:
    typedef std::shared_ptr<cui_connection>    pointer_t;

    enum class color_t
    {
        NORMAL,

        BLACK,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE
    };

                    cui_connection(std::string const & history_filename = std::string());

    virtual         ~cui_connection() override;

    void            output(std::string const & line);
    void            output(std::string const & line, color_t f, color_t b);
    void            clear_output();
    void            refresh();
    void            set_prompt(std::string const & prompt);

    // implementation of snap_communicator::snap_fd_connection
    virtual void    process_read() override;

    // new callbacks
    //
    // the "command" (whatever text was typed in the command
    // area gets sent to you through this function)
    //
    // the "quit" is called whenever Ctrl-D was clicked on an
    // empty line; you must get the console closed or it will
    // be blocked; further typing will go to the normal console
    // instead of the "command", so it is important to take
    // that callback in account
    //
    virtual void    ready();
    virtual void    process_command(std::string const & command) = 0;
    virtual void    process_quit();
    virtual void    process_help();

private:
    friend detail::ncurses_impl;

    std::shared_ptr<detail::ncurses_impl>     f_impl = std::shared_ptr<detail::ncurses_impl>();
};


} // namespace ed
// vim: ts=4 sw=4 et

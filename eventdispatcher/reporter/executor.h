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
#pragma once

// self
//
#include    <eventdispatcher/reporter/state.h>


// eventdispatcher
//
#include    <eventdispatcher/thread_done_signal.h>


// cppthread
//
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



class executor
{
public:
    typedef std::shared_ptr<executor>   pointer_t;
    typedef std::function<void()>       thread_done_callback_t;

                                    executor(state::pointer_t s);
                                    ~executor();

    void                            start();
    bool                            run();
    void                            stop();

    void                            set_thread_done_callback(thread_done_callback_t callback);

private:
    ed::thread_done_signal::pointer_t
                                    f_done_signal = ed::thread_done_signal::pointer_t();
    cppthread::runner::pointer_t    f_runner = cppthread::runner::pointer_t();
    cppthread::thread::pointer_t    f_thread = cppthread::thread::pointer_t();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

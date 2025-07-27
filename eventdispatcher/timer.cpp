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

/** \file
 * \brief Implementation of the Timer class.
 *
 * This class allows you to create a "connection" which is just a timer.
 *
 * All connections have a timer feature, but at times you have to either
 * disable a connection or you already use the timer for some other
 * reasons so we offer a separate timer class for you additional needs.
 */


// self
//
#include    "eventdispatcher/timer.h"

#include    "eventdispatcher/utils.h"


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initializes the timer object.
 *
 * This function initializes the timer object with the specified \p timeout
 * defined in microseconds.
 *
 * Note that by default all connection objects are persistent
 * since in most cases that is the type of connections you are interested
 * in. Therefore timers are also persistent. This means if you
 * want a one time callback, you want to call the remove_connection()
 * function with your timer from your callback.
 *
 * Pass 0 as \p timeout to have a one time process_timeout() call once
 * the run() function is ready. Pass -1 to start with a disabled timer.
 * Pass a positive number to get ticks every time that amount of
 * microseconds have passed. You can change these values using the
 * set_timeout_date() and set_timeout_delay() functions later.
 *
 * \note
 * POSIX offers timers (in Linux since kernel version 2.6), only
 * (a) these generate signals, which is generally considered slow
 * in comparison to a timeout assigned to the poll() function, and
 * (b) the kernel posts at most one timer signal at a time across
 * one process, in other words, if 5 timers time out before you are
 * given a chance to process the timer, you only get one single
 * signal.
 *
 * \param[in] timeout_us  The timeout in microseconds.
 */
timer::timer(std::int64_t timeout_us)
{
    if(timeout_us == 0)
    {
        // if zero, we assume that the timeout is a one time trigger
        // and that it will be set to other dates at other later times
        //
        set_timeout_date(get_current_date());
    }
    else
    {
        set_timeout_delay(timeout_us);
    }
}


/** \brief Retrieve a reference to the timer callback manager.
 *
 * This function returns a reference to the timer callback manager.
 * It can be used to manage the functions being called by the timer.
 *
 * For additional information about the callback manager, see the snapdev
 * project. See also the timer::process_timer() reimplementation as it
 * gives an example on how to use this functionality.
 *
 * \note
 * It is expected to be used only if you create a standalone timer
 * opposed to creating a class that derives from a timer and re-implements
 * the process_timer() function.
 *
 * \return A reference to the timer callback manager.
 *
 * \sa process_timer()
 */
timer::callback_manager_t & timer::get_callback_manager()
{
    return f_callback_manager;
}


/** \brief Retrieve the socket of the timer object.
 *
 * Timer objects are never attached to a socket so this function always
 * returns -1.
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return Always -1.
 */
int timer::get_socket() const
{
    return -1;
}


/** \brief Tell that the socket is always valid.
 *
 * This function always returns true since the timer never uses a socket.
 *
 * \return Always true.
 */
bool timer::valid_socket() const
{
    return true;
}


/** \brief Implementation of the process_timeout() function.
 *
 * By default, the process_timeout() is expected to be implemented by your
 * own derived version of the timer class. However, many times more than one
 * timer is required and having to create a new class each time is a lot
 * of work. By default, a timer object will call one or more functions you
 * setup using the get_callback_manager().add_callback() function.
 *
 * Here is a simple example also showing how to remove a callback once you
 * are done with the timer.
 *
 * \code
 * in the .cpp:
 *
 *     void my_class::init()
 *     {
 *         f_timer = std::shared_ptr<ed::timer>(1000);
 *         f_id = f_timer->get_callback_manager().add_callback(sdtd::bind(&my_class::tick, std::placeholders::_1, "caller one"));
 *     }
 *
 *     // the function receives a pointer to the timer which can be useful
 *     // if the function is not part of the object that owns the timer
 *     //
 *     bool my_class::tick(ed::timer::pointer_t t, std::string const & caller)
 *     {
 *         if(<some condition>)
 *         {
 *             // this means we are done
 *             //
 *             f_timer->get_callback_manager().remove_callback(f_id);
 *             f_id = ed::timer::callback_manager_t::NULL_CALLBACK_ID;
 *             f_timer.clear();
 *             return;
 *         }
 *
 *         // do some other work with the tick event
 *     }
 *
 * in the .h:
 *
 * private:
 *     ed::timer::pointer_t                            f_timer = ed::timer::pointer_t();
 *     ed::timer::callback_manager_t::callback_id_t    f_timer_id = ed::timer::callback_manager_t::NULL_CALLBACK_ID;
 * \endcode
 *
 * \note
 * If no functions are added to the callback manager, then this function
 * does nothing.
 *
 * \sa get_callback_manager()
 */
void timer::process_timeout()
{
    snapdev::NOT_USED(f_callback_manager.call(std::dynamic_pointer_cast<timer>(shared_from_this())));
}



} // namespace ed
// vim: ts=4 sw=4 et

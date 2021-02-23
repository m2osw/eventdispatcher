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
 * \brief Implementation of the dispatcher_support class.
 *
 * The various connection classes that know how to handle a message derive
 * from this class. This means the message can then automatically get
 * dispatched.
 */


// self
//
#include    "eventdispatcher/dispatcher_support.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief The destuctor.
 *
 * The destructor cleans up the dispatcher support object.
 */
dispatcher_support::~dispatcher_support()
{
}


/** \brief Define a dispatcher to execute your functions.
 *
 * The dispatcher to use to dispatch messages when received. The dispatch
 * happens by matching the command name with the dispatcher_match and
 * calling the corresponding function.
 *
 * If no match is found, then nothing gets executed by the dispatcher and
 * your default process_message() function gets called instead. If you
 * use a "match all" type of entry in your dispatcher, then your
 * process_message() function never gets called.
 *
 * \param[in] d  The pointer to your dispatcher object.
 */
void dispatcher_support::set_dispatcher(dispatcher_base::pointer_t d)
{
    f_dispatcher = d;
}


/** \brief Get the dispatcher used to execute your message functions.
 *
 * This function returns the dispatcher one set with the set_dispatcher()
 * function. It may be a nullptr.
 *
 * \warning
 * Note that it may return nullptr because the weak pointer was just
 * set to nullptr as the owner of the dispatcher was deleted.
 *
 * \return The pointer to the dispatcher used to execite messages or nullptr.
 */
dispatcher_base::pointer_t dispatcher_support::get_dispatcher() const
{
    return f_dispatcher.lock();
}


/** \brief Dispatcher the specified message.
 *
 * This dispatcher function searches for a function that matches the
 * command of the specified \p message.
 *
 * The dispatcher handles a vector of dispatcher_match structures each
 * of which defines a message that this daemon understands. The dispatch
 * is done on a match as determined the the f_match() static function.
 *
 * The function executes the f_execute() function on a match. If none of
 * the dispatcher_match entries match the input message, then the default
 * process resumes, which is to call the process_message() function. This
 * is done as a fallback and it should only be used if you want to be
 * able to handle very complex cases as in the snapcommunicator. In most
 * cases, having a function that handles your command(s) will be more
 * than enough.
 *
 * If you called the add_communicator_commands() function on your
 * dispatcher, it won't be necessary to implement the process_message()
 * since it adds a last entry which is a "catch all" entry. This entry
 * uses the function that replies to the user with the UNKNOWN message.
 * Assuming you do not do anything extraordinary, you just need to
 * implement the ready() and stop() functions. If you have dynamic
 * commands that the default msg_help() wont' understand, then you
 * need to also implement the help() function.
 *
 * \param[in,out] message  The message being dispatched.
 *
 * \return true if the dispatcher handled the message, false if the
 *         process_message() function was called instead.
 */
bool dispatcher_support::dispatch_message(message & msg)
{
    auto d(f_dispatcher.lock());
    if(d != nullptr)
    {
        // we have a dispatcher installed, try to dispatch that message
        //
        if(d->dispatch(msg))
        {
            return true;
        }
    }

    // either there was no dispatcher installed or the message is
    // not in the list of messages handled by this dispatcher
    //
    process_message(msg);

    return false;
}


/** \brief A default implementation of the process_message() function.
 *
 * This function is adefault fallback for the process_message()
 * functionality. If you define a dispatcher, then you probably
 * won't need to define a process_message() which in most cases
 * would do the exact same thing but it would be called.
 *
 * This is especially true if you finish your list of matches
 * with the always_match() function and msg_reply_with_unknown()
 * as the function to run when that entry is hit.
 *
 * \code
 *      {
 *          nullptr
 *        , &dispatcher<my_connection>::dispatcher_match::msg_reply_with_unknown
 *        , &dispatcher<my_connection>::dispatcher_match::always_match
 *      },
 * \endcode
 *
 * \todo
 * Look into fixing this function so it can send the UNKNOWN message itself.
 * That way we'd avoid the last entry in the match array, which would allow
 * us to have binary search (much faster).
 *
 * \param[in] message  The message to be processed.
 */
void dispatcher_support::process_message(message const & msg)
{
    // We don't currently have access to the send_message() function from
    // here--the inter_thread_message_connection class causes a problem
    // because it has two process_message() functions: process_message_a()
    // and process_message_b().
    //
    //message unknown;
    //unknown.reply_to(message);
    //unknown.set_command("UNKNOWN");
    //unknown.add_parameter("command", message.get_command());
    //if(!send_message(unknown, false))
    //{
    //    SNAP_LOG_WARNING
    //        << "could not reply with UNKNOWN message to \""
    //        << message.get_command()
    //        << "\""
    //        << SNAP_LOG_SEND;
    //}

    SNAP_LOG_FATAL
        << "process_message() with message \""
        << msg.to_message()
        << "\" was not reimplemented in your class and"
        << " the always_match() was not used in your dispatcher matches."
        << SNAP_LOG_SEND;

    throw event_dispatcher_implementation_error(
              "your class is not reimplementing the process_message()"
              " virtual function and your dispatcher did not catch mnessage \""
            + msg.to_message()
            + "\".");
}



} // namespace ed
// vim: ts=4 sw=4 et

// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the dispatcher class.
 *
 * Class used to handle messages received from clients and services.
 */


// self
//
#include    "eventdispatcher/dispatcher.h"

#include    "eventdispatcher/names.h"


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \class dispatcher
 * \brief A template to create a list of messages to dispatch on receival.
 *
 * Whenever you receive messages, they can automatically get dispatched to
 * various functions using the dispatcher.
 *
 * You define a dispatcher_match array and then add a dispatcher to
 * your connection object.
 *
 * \code
 *      #include    <eventdispatcher/names.h>
 *      ...
 *      ed::dispatcher<my_connection>::dispatcher_match const my_messages[] =
 *      {
 *          ed::dispatcher<my_connection>::define_match(
 *              ed::dispatcher<my_connection>::Expression(g_name_ed_cmd_help)
 *            , ed::dispatcher<my_connection>::Execute(&my_connection::msg_help)
 *            //, ed::dispatcher<my_connection>::MatchFunc(&ed::dispatcher<my_connection>::dispatcher_match::one_to_one_match) -- use default
 *          ),
 *          ed::dispatcher<my_connection>::define_match(
 *              ed::dispatcher<my_connection>::Expression(g_name_ed_cmd_status)
 *            , ed::dispatcher<my_connection>::Execute(&my_connection::msg_status)
 *            //, ed::dispatcher<my_connection>::MatchFunc(&dispatcher<my_connection>::dispatcher_match::one_to_one_match) -- use default
 *          ),
 *          ... // other messages
 *
 *          // with the following, you can end your list with a catch all,
 *          // which generate the UNKNOWN message (not required)
 *          //
 *          // if you have that entry, your own process_message() function
 *          // will not get called
 *          //
 *          // if you call the add_communicator_commands() then this rule
 *          // is automatically added for you
 *          //
 *          f_dispatcher->define_catch_all()
 *      };
 * \endcode
 *
 * In most cases you do not need to specify the matching function. It will
 * use the default which is a one to one match. So in the example above,
 * for "HELP", only a message with the command set to "HELP" will match.
 * When a match is found, the corresponding function (msg_help() here)
 * gets called.
 *
 * Note that "functions" are actually offsets. You will get `this` defined
 * as expected when your function gets called. The one drawback is that
 * only a function of the connection you attach the dispatcher to can
 * be called from the dispatcher. This is because we want to have a
 * static table instead of a dynamic one created each time we start
 * a process (in a website server, many processes get started again and
 * again.)
 *
 * \note
 * The T parameter of this template is often referenced as a "connection"
 * because it is expected to be a connection. There is actually no such
 * constraint on that object. It just needs to understand the dispatcher
 * usage which is to call the dispatch() function whenever a message is
 * received. Also it needs to implement any f_execute() function as
 * defined in its dispatcher_match vector.
 *
 * \note
 * This is documented here because it is a template and we cannot do
 * that in the .cpp (at least older versions of doxygen could not.)
 *
 * \todo
 * Transform the dispatcher_match with classes so we can build the
 * array safely.
 *
 * \tparam T  The connection class to be used with this dispathcer.
 */



/** \typedef dispatcher::pointer_t
 * \brief A smart pointer of the dispatcher.
 *
 * Although we expect the array of `dispatcher_match` to be
 * statically defined, the `dispatcher`, on the other hand,
 * is quite dynamic and needs to be allocated in a smart
 * pointer then added to your connection.
 */



/** \brief Initialize the dispatcher with your connection and messages.
 *
 * This function takes a pointer to your connection and an array
 * of matches.
 *
 * Whenever a message is received by one of your connections, the
 * dispatch() function gets called which checks the message against
 * each entry in this array of \p matches.
 *
 * To finish up the initialization of the dispatcher, you want to call
 * the add_matches() function with a vector of matches.
 *
 * You may also call the add_communicater_commands() if you want to handle
 * communicator daemon messages automatically.
 *
 * \param[in] connection  The connection for which this dispatcher is
 *                        created.
 */
dispatcher::dispatcher(ed::connection_with_send_message * c)
    : f_connection(c)
{
}


/** \brief Add a default array of possible matches.
 *
 * In Snap! a certain number of messages are always exactly the same
 * and these can be implemented internally so each daemon doesn't have
 * to duplicate that work over and over again. These are there in part
 * because the communicator daemon expects those messages there.
 *
 * IMPORTANT NOTE: If you add your own version in your dispatcher_match
 * vector, then these will be ignored since your version will match first
 * and the dispatcher uses the first function only.
 *
 * This array currently includes:
 *
 * \li ALIVE -- msg_alive() -- auto-reply with ABSOLUTELY
 * \li INVALID -- msg_log_unknown() -- in case we receive a message we
 *                understand but with missing/invalid parameters
 * \li HELP -- msg_help() -- returns the list of all the messages
 * \li LEAK -- msg_leak() -- log memory usage
 * \li LOG_ROTATE -- msg_log_rotate() -- reopen() the logger
 * \li QUITTING -- msg_quitting() -- calls stop(true);
 * \li READY -- msg_ready() -- calls ready() -- communicatord always
 *              sends that message so it has to be supported
 * \li RESTART -- msg_restart() -- calls restart() -- it is triggered
 *                when a restart is required (i.e. the library was
 *                upgraded, a configuration file was updated, etc.)
 * \li STOP -- msg_stop() -- calls stop(false);
 * \li UNKNOWN -- msg_log_unknown() -- in case we receive a message we
 *                don't understand
 * \li * -- msg_reply_with_unknown() -- the last entry will be a grab
 *          all pattern which returns the UNKNOWN message automatically
 *          for you
 *
 * The msg_...() functions must be declared in your class T. If you
 * use the system connection_with_send_message class then they're
 * already defined there.
 *
 * The `HELP` response is automatically built from the f_matches.f_expr
 * strings. However, if any one function used to match the message
 * command is not one_to_one_match(), then that string doesn't get used.
 *
 * If the message makes use of a regular expression (i.e. the function is
 * not the one_to_one_match()) then the user help() function gets called
 * and we expect that function to add any dynamic message the daemon
 * understands.
 *
 * The LOG_ROTATE message reconfigures the logger by reopening it if the
 * is_configured() function says it was configured. In any other
 * circumstances, nothing happens.
 *
 * Note that the UNKNOWN message is understood and just logs the message
 * received. This allows us to see that \b we sent a message that the
 * receiver (not us) does not understand and adjust our code accordingly
 * (i.e. add support for that message in that receiver or maybe fix the
 * spelling).
 *
 * The \p auto_catch_all flag is true by default, meaning that the
 * service does not support any other messages and wants to reply with
 * the `UNKNOWN` message. Setting this to false means that you will
 * either add even more messages manually or that you want your
 * process_message() called (which is the last resort when the dispatcher
 * fails to process a message).
 *
 * \todo
 * Look at whether it would be possible to move the add_communicater_commands()
 * to the communicatord library. At the moment, this comes with a set of
 * messages defined in the connection_with_send_message so it would be strange
 * to break that in half. If we can move both there, then we would be ready
 * for the switch. It may mean deriving from yet another class when you
 * derive from a connection with messages which is not ideal.
 *
 * \param[in] auto_catch_all  If true, automatically catch all other
 * messages and reply with `UNKNOWN`.
 */
void dispatcher::add_communicator_commands(bool auto_catch_all)
{
    // avoid more than one realloc()
    //
    f_matches.reserve(f_matches.size() + 11);

    add_matches({
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_alive)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_alive, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_help)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_help, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_invalid)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_log_unknown, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_leak)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_leak, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_log_rotate)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_log_rotate, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_quitting)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_quitting, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_ready)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_ready, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_restart)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_restart, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_service_unavailable)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_service_unavailable, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_stop)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_stop, f_connection, std::placeholders::_1))
        ),
        ::ed::define_match(
              ::ed::Expression(g_name_ed_cmd_unknown)
            , ::ed::Callback(std::bind(&connection_with_send_message::msg_log_unknown, f_connection, std::placeholders::_1))
        ),
    });

    // always last
    //
    if(auto_catch_all)
    {
        add_match(define_catch_all());
    }
}


/** \brief Retrieve a reference to the list of matches.
 *
 * This function returns a reference to the list of matches.
 *
 * \warning
 * If the dispatcher is given an "always match" case, then it is
 * handled separately from the f_matches vector. It is done that way to
 * allow for much more dynamic lists of matches. So this function
 * returns all the matches except that "always match" (if defined).
 *
 * \return A const reference to the internal vector of matches.
 */
dispatcher_match::vector_t const & dispatcher::get_matches() const
{
    return f_matches;
}


/** \brief Append a match at the end of the list of matches.
 *
 * This function adds yet another match to the list of matches, allowing
 * you to add more matches in your sub-classes. For example, the fluid
 * settings client wants to capture the FLUID_... specific messages and
 * uses this function to add its own message matches.
 */
void dispatcher::add_match(dispatcher_match const & m)
{
    if(f_show_matches)
    {
        SNAP_LOG_CONFIG
            << "add_match() with command expression \""
            << (m.f_expr == nullptr ? "<match all>" : m.f_expr)
            << "\"."
            << SNAP_LOG_SEND;
    }

    // special handling of the "always match" case
    //
    if(m.match_is_always_match())
    {
        if(f_end.f_callback != nullptr)
        {
            throw implementation_error(
                  std::string("add_match() called with a second \"always_match()\" rule (expression \"")
                + (m.f_expr == nullptr ? "<undefined>" : m.f_expr)
                + "\").");
        }
        f_end = m;
    }
    else
    {
        // insert taking the priority in account
        // (very important for callbacks)
        //
        auto it(std::find_if(
              f_matches.begin()
            , f_matches.end()
            , [&m](auto const & item){
                return m.f_priority < item.f_priority;
            }));
        f_matches.insert(it, m);
    }
}


/** \brief Append all the matches found in a vector of matches.
 *
 * This function appends all the matches found in the specified
 * \p matches parameter to this dispatcher.
 *
 * This function calls the add_match() once for each matches.
 *
 * \param[in] matches  An array of matches.
 */
void dispatcher::add_matches(dispatcher_match::vector_t const & matches)
{
    for(auto const & m : matches)
    {
        add_match(m);
    }
}


/** \brief Remove all the matches with specified tag.
 *
 * Whenever you dynamically add matches to a dispatcher, you may need to
 * remove them at the time you destroy your connection. This function
 * allows you to do so by removing all the matches with the specified
 * \p tag.
 *
 * \note
 * The dispatcher_match::DISPATCHER_MATCH_NO_TAG tag is the default (0)
 * and considered to not be a valid value for this function. If called
 * with this value, then the function does nothing.
 *
 * \param[in] tag  The matches with that tag have to be removed.
 */
void dispatcher::remove_matches(dispatcher_match::tag_t tag)
{
    if(tag == dispatcher_match::DISPATCHER_MATCH_NO_TAG)
    {
        return;
    }

    for(auto it(f_matches.begin()); it != f_matches.end(); )
    {
        if(it->f_tag == tag)
        {
            it = f_matches.erase(it);
        }
        else
        {
            ++it;
        }
    }
}


/** \brief The dispatch function.
 *
 * This is the function your message system will call whenever
 * the system receives a message.
 *
 * The function returns true if the message was dispatched.
 * When that happen, the process_message() function of the
 * connection should not be called.
 *
 * You may not include a message in the array of `dispatcher_match`
 * if it is too complicated to match or too many variables are
 * necessary then you will probably want to use your
 * process_message().
 *
 * By adding a catch-all at the end of your list of matches, you
 * can easily have one function called for any message. By default
 * the dispatcher environment offers such a match function and
 * it also includes a function that sends the UNKNOWN message as
 * an immediate reply to a received message.
 *
 * \param[in] msg  The message to be dispatched.
 *
 * \return true if the message was dispatched, false otherwise.
 */
bool dispatcher::dispatch(message & msg)
{
    if(f_trace)
    {
        SNAP_LOG_TRACE
            << "dispatch message \""
            << msg.to_message()
            << "\"."
            << SNAP_LOG_SEND;
    }

    // go in order to execute matches
    //
    // remember that a dispatcher with just a set of well defined command
    // names is a special case (albeit frequent) and we can't process
    // using a map (a.k.a. fast binary search) as a consequence
    //
    for(auto const & m : f_matches)
    {
        if(m.execute(msg))
        {
            return true;
        }
    }

    // if at least one callback was hit, we consider that the message was
    // processed and return true here
    //
    if(msg.was_processed())
    {
        return true;
    }

    // the always match is not in the main vector, test it separately
    //
    if(f_end.f_callback != nullptr)
    {
        if(f_end.execute(msg))
        {
            return true;
        }
    }

    return false;
}


/** \brief Set whether the dispatcher should trace your messages or not.
 *
 * By default, the f_trace flag is set to false. You can change it to
 * true while debugging. You should remember to turn it back off once
 * you make an official version of your service to avoid the possibly
 * huge overhead of sending all those log messages. One way to do so
 * is to place the code within \#ifdef/\#endif as in:
 *
 * \code
 *     #ifdef _DEBUG
 *         my_dispatcher->set_trace();
 *     #endif
 * \endcode
 *
 * \param[in] trace  Set to true to get SNAP_LOG_TRACE() of each message.
 */
void dispatcher::set_trace(bool trace)
{
    f_trace = trace;
}


/** \brief Set whether to show the matches as they get added.
 *
 * If you first set this flag and then call the add_match() or add_matches()
 * then the name of the commands that get added are sent to the logs. In
 * most cases, at this point, we use this in our debug version of the
 * services we develop. Later we want to add a command line flag so any
 * time we can turn this on for any service and see what's accepted.
 */
void dispatcher::set_show_matches(bool show_matches)
{
    f_show_matches = show_matches;
}


/** \brief Retrieve the list of commands.
 *
 * This function transforms the vector of f_matches in a list of
 * commands in a string_list_t.
 *
 * \note
 * The \p commands parameter is not reset. This means you may add commands
 * ahead of this call and they will still be there on return.
 *
 * \note
 * The \p commands is a set so we avoid getting duplicates. This is because
 * a match which accepts a callback does not stop processing the value.
 * As a result, we can end up processing multiple functions each of which
 * would understand the exact same command. You may want to make sure that
 * you use the callback priority on those match definitions.
 * \node
 * \code
 *     ed::define_match(
 *           ed::Expression(communicatord::g_name_communicatord_cmd_status)
 *         , ed::Callback(std::bind(&cluckd::msg_status, c, std::placeholders::_1))
 *         , ed::MatchFunc(&ed::one_to_one_callback_match)
 *         , ed::Priority(ed::dispatcher_match::DISPATCHER_MATCH_CALLBACK_PRIORITY)
 *     ),
 * \endcode
 *
 * \param[in,out] commands  The place where the list of commands is saved.
 *
 * \return false if the commands were all determined, true if some need
 *         help from the user of this dispatcher.
 */
bool dispatcher::get_commands(advgetopt::string_set_t & commands)
{
    bool need_user_help(f_end.f_callback != nullptr);
    for(auto const & m : f_matches)
    {
        if(m.f_expr == nullptr)
        {
            if(!m.match_is_always_match() // this one should not happend here (such ends up in f_end instead)
            && !m.match_is_callback_match())
            {
                // this is a "special case" where the user has
                // a magical function which does not require an
                // expression at all (i.e. "hard coded" in a
                // function)
                //
                need_user_help = true;
            }
            //else -- always match is the last entry and that just
            //        means we can return UNKNOWN on an unknown message
        }
        else if(m.match_is_one_to_one_match()
             || m.match_is_one_to_one_callback_match())
        {
            // add the f_expr as is since it represents a command
            // as is
            //
            auto inserted(commands.insert(m.f_expr));
            if(!inserted.second)
            {
                // tell the user that his configuration includes duplicates
                // which is fine if those are CALLBACKs as in the match()
                // functions return match_t::MATCH_CALLBACK
                //
                // you can also set the message handler priority if it is
                // a callback and not for the other (i.e. the callback
                // priority will insert the match() function earlier)
                //
                // i.e. ed::Priority(ed::dispatcher_match::DISPATCHER_MATCH_CALLBACK_PRIORITY)
                //
                SNAP_LOG_CONFIGURATION_WARNING
                    << "command \""
                    << m.f_expr
                    << "\" was already inserted. Is it a \"match_t::MATCH_CALLBACK\"?"
                       " If so then it is fine."
                       " If not, some of your callback functions may not get called."
                    << SNAP_LOG_SEND;
            }
        }
        else
        {
            // this is not a one to one match, so possibly a
            // full regex or similar
            //
            need_user_help = true;
        }
    }

    return need_user_help;
}


/** \brief Return a match which defines a "catch all" matcher.
 *
 * This function creates a dispatcher_match object which will catch all
 * the messages no matter what. This can be used to \em close the list
 * of matchers and not get your process_message() function called. The
 * default matcher calls the
 * connection_with_send_message::msg_reply_with_unknown()
 * function which simply returns an "UNKNOWN" message to the sender.
 *
 * \return A dispatcher_match object to add to your dispatcher.
 */
dispatcher_match dispatcher::define_catch_all() const
{
    return define_match(
          Callback(std::bind(&connection_with_send_message::msg_reply_with_unknown, f_connection, std::placeholders::_1))
        , MatchFunc(&always_match));
}








/** \var dispatcher::f_connection
 * \brief The connection pointer.
 *
 * This parameter is set by the constructor. It represents the
 * connection this dispatcher was added to (a form of parent of
 * this dispatcher object.)
 *
 * \warning
 * This is a bare pointer because the dispatcher is allocated in
 * the constructor of the connection using the dispatcher and in
 * a constructor, you still do not have access to a shared pointer.
 */


/** \var dispatcher::f_matches
 * \brief The array of possible matches.
 *
 * This is the vector of your messages with the corresponding
 * match and execute functions. This is used to go through
 * the matches and execute (dispatch) as required.
 */


/** \var dispatcher::f_ended
 * \brief Whether the "always match" was added to the list.
 *
 * The list of matches can be ended with a match which uses the
 * "always match" rule (a function which always returns true). When
 * that special match is used at the end of your list, this flag is
 * set to true.
 *
 * If you try to add another rule after the match all, then an
 * exception is raised.
 *
 * \sa add_match()
 * \sa add_matches()
 * \sa add_communicator_commands()
 */


/** \var dispatcher::f_trace
 * \brief Tell whether messages should be traced or not.
 *
 * Because your service may accept and send many messages a full
 * trace on all of them can really be resource intensive. By default
 * the system will not trace anything. By setting this parameter to
 * true (call set_trace() for that) you request the SNAP_LOG_TRACE
 * to run on each message received by this dispatcher. This is done
 * on entry so whether the message is processed by the dispatcher
 * or your own send_message() function, it will trace that message.
 */



} // namespace ed
// vim: ts=4 sw=4 et

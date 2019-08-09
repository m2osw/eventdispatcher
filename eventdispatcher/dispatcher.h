// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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


// snapwebsites lib
//
#include    "eventdispatcher/snap_communicator.h"
#include    "eventdispatcher/utils.h"


// snaplogger lib
//
#include    <snaplogger/message.h>



namespace ed
{



/** \brief A template to create a list of messages to dispatch on receival.
 *
 * Whenever you receive messages, they can automatically get dispatched to
 * various functions using the dispatcher.
 *
 * You define a dispatcher_match array and then add a dispatcher to
 * your connection object.
 *
 * \code
 *      ed::dispatcher<my_connection>::dispatcher_match const my_messages[] =
 *      {
 *          {
 *              "HELP"
 *            , &my_connection::msg_help
 *            //, &dispatcher<my_connection>::dispatcher_match::one_to_one_match -- use default
 *          },
 *          {
 *              "STATUS"
 *            , &my_connection::msg_status
 *            //, &dispatcher<my_connection>::dispatcher_match::one_to_one_match -- use default
 *          },
 *          ... // other messages
 *
 *          // if you'd like you can end your list with a catch all which
 *          // generate the UNKNOWN message with the following (not required)
 *          // if you have that entry, your own process_message() function
 *          // will not get called
 *          {
 *              nullptr
 *            , &dispatcher<my_connection>::dispatcher_match::msg_reply_with_unknown
 *            , &dispatcher<my_connection>::dispatcher_match::always_match
 *          },
 *      };
 * \endcode
 *
 * In most cases you do not need to specify the matching function. It will
 * use the default which is a one to one match. So in the example above,
 * for "HELP", only a message with the command set to "HELP" will match.
 * When a match is found, the correcsponding function (msg_help() here)
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
 * defined in its dispatch_match vector.
 *
 * \note
 * This is documented here because it is a template and we cannot do
 * that in the .cpp (at least older versions of doxygen could not.)
 */
template<typename T>
class dispatcher
    : public dispatcher_base
{
public:
    /** \brief A smart pointer of the dispatcher.
     *
     * Although we expect the array of `dispatcher_match` to be
     * statically defined, the `dispatcher`, on the other hand,
     * is quite dynamic and needs to be allocated in a smart
     * pointer then added to your connection.
     */
    typedef std::shared_ptr<dispatcher> pointer_t;

    /** \brief This structure is used to define the list of supported messages.
     *
     * Whenever you create an array of messages, you use this structure.
     *
     * The structure takes a few parameters as follow:
     *
     * \li f_expr -- the "expression" to be matched to the command name
     *               for example "HELP".
     * \li f_execute -- the function offset to execute on a match.
     * \li f_match -- the function to check whether the expression is a match;
     *                it has a default of one_to_one_match() which means the
     *                f_expr string is viewed as a plain string defining the
     *                message name as is.
     *
     * The command name is called "f_expr" but some matching functions may
     * make use of the "f_expr" parameter as an expression such as a
     * regular expression. Such functions will be added here with time, you
     * may also have your own, of course. The match function is expected to
     * be a static or standalone function.
     */
    struct dispatcher_match
    {
        /** \brief Define a vector of dispatcher_match objects.
         *
         * This function includes an array of dispatcher_match objects.
         * Whenever you define dispatcher_match objects, you want to
         * use the C++11 syntax to create a vector.
         *
         * \attention
         * We are NOT using a match because the matching may make use
         * of complex functions that support things such as complex as
         * regular expressions. In other words, the name of the message
         * may not just be a simple string.
         */
        typedef std::vector<dispatcher_match>       vector_t;

        /** \brief The execution function.
         *
         * This type defines the execution function. We give it the message
         * on a match. If the command name is not a match, it is ignored.
         */
        typedef void (T::*execute_func_t)(message & msg);

        /** \brief The match function return types.
         *
         * Whenever a match function is called, it may return one of:
         *
         * \li MATCH_FALSE
         *
         * The function did not match anything. Ignore the corresponding
         * function.
         *
         * \li MATCH_TRUE
         *
         * This is a match, execute the function. We are done with this list
         * of matches.
         *
         * \li MATCH_CALLBACK
         *
         * The function is a callback, it gets called and the process
         * continues. Since the message parameter is read/write, it is
         * a way to tweak the message before other functions get it.
         */
        enum class match_t
        {
            MATCH_FALSE,
            MATCH_TRUE,
            MATCH_CALLBACK
        };

        /** \brief The match function.
         *
         * This type defines the match function. We give it the message
         * which has the command name, although specialized matching
         * function could test other parameters from the message such
         * as the origination of the message.
         */
        typedef match_t (*match_func_t)(std::string const & expr, message & msg);

        /** \brief The default matching function.
         *
         * This function checks the command one to one to the expression.
         * The word in the expression is compared as is to the command
         * name:
         *
         * \code
         *          return expr == msg.get_command();
         * \endcode
         *
         * We will add other matching functions with time
         * (start_with_match(), regex_match(), etc.)
         *
         * \note
         * It is permissible to use a match function to modify the
         * message in some way, however, it is not recommended.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return MATCH_TRUE if it is a match, MATCH_FALSE otherwise.
         */
        static match_t one_to_one_match(std::string const & expr, message & msg)
        {
            return expr == msg.get_command()
                            ? match_t::MATCH_TRUE
                            : match_t::MATCH_FALSE;
        }

        /** \brief Always returns true.
         *
         * This function always returns true. This is practical to close
         * your list of messages and return a specific message. In most
         * cases this is used to reply with the UNKNOWN message.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return Always returns MATCH_TRUE.
         */
        static match_t always_match(std::string const & expr, message & msg)
        {
            snap::NOTUSED(expr);
            snap::NOTUSED(msg);
            return match_t::MATCH_TRUE;
        }

        /** \brief Always returns true.
         *
         * This function always returns true. This is practical to close
         * your list of messages and return a specific message. In most
         * cases this is used to reply with the UNKNOWN message.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return Always returns MATCH_CALLBACK.
         */
        static match_t callback_match(std::string const & expr, message & msg)
        {
            snap::NOTUSED(expr);
            snap::NOTUSED(msg);
            return match_t::MATCH_CALLBACK;
        }

        /** \brief The expression to compare against.
         *
         * The expression is most often going to be the exact command name
         * which will be matched with the one_to_one_match() function.
         *
         * For other match functions, this would be whatever type of
         * expression supported by those other functions.
         *
         * \note
         * Effective C++ doesn't like bare pointers, but there is no real
         * reason for us to waste time and memory by having an std::string
         * here. It's going to always be a constant pointer anyway.
         */
        char const *        f_expr = nullptr;

        /** \brief The execute function.
         *
         * This is an offset in your connection class. We do not allow
         * std::bind() because we do not want the array of messages to be
         * dynamic (that way it is created at compile time and loaded as
         * ready/prepared data on load.)
         *
         * The functions called have `this` defined so you can access
         * your connection data and other functions. It requires the
         * `&` and the class name to define the pointer, like this:
         *
         * \code
         *      &MyClass::my_message_function
         * \endcode
         *
         * The execution is started by calling the run() function.
         */
        execute_func_t      f_execute = nullptr;

        /** \brief The match function.
         *
         * The match function is used to know whether that command
         * dispatch function was found.
         *
         * By default this parameter is set to one_to_one_match().
         * This means the command has to be one to one equal to
         * the f_expr string.
         *
         * The matching is done in the match() function.
         */
        match_func_t        f_match = &::ed::dispatcher<T>::dispatcher_match::one_to_one_match;

        /** \brief Run the execution function if this is a match.
         *
         * First this function checks whether the command of the message
         * in \p msg matches this `dispatcher_match` expression. In
         * most cases the match function is going to be
         * one_on_one_match() which means it has to be exactly equal.
         *
         * If it is a match, this function runs your \p connection execution
         * function (i.e. the message gets dispatched) and then it returns
         * true.
         *
         * If the message is not a match, then the function returns false
         * and only the matching function was called. In this case the
         * \p connection does not get used.
         *
         * When this function returns true, you should not call the
         * process_message() function since that was already taken care
         * of. The process_message() function should only be called
         * if the message was not yet dispatched. When the list of
         * matches includes a catch all at the end, the process_message()
         * will never be called.
         *
         * \note
         * Note that we do not offer two functions, one to run the match
         * function and one to execute the match because you could otherwise
         * end up calling the execute function (dispatch) on any
         * `dispatcher_match` entry and we did not want that to happen.
         *
         * \param[in] connection  The connection attached to that
         *                        `dispatcher_match`.
         * \param[in] msg  The message that matched.
         *
         * \return true if the connection execute function was called.
         */
        bool execute(T * connection, ::ed::message & msg) const
        {
            match_t m(f_match(f_expr, msg));
            if(m == match_t::MATCH_TRUE
            || m == match_t::MATCH_CALLBACK)
            {
                (connection->*f_execute)(msg);
                if(m == match_t::MATCH_TRUE)
                {
                    return true;
                }
            }

            return false;
        }

        /** \brief Check whether f_match is one_to_one_match().
         *
         * This function checks whether the f_match function was defined
         * to one_to_one_match()--the default--and if so returns true.
         *
         * \return true if f_match is the one_to_one_match() function.
         */
        bool match_is_one_to_one_match() const
        {
            return f_match == &snap::dispatcher<T>::dispatcher_match::one_to_one_match;
        }

        /** \brief Check whether f_match is always_match().
         *
         * This function checks whether the f_match function was defined
         * to always_match() and if so returns true.
         *
         * \return true if f_match is the always_match() function.
         */
        bool match_is_always_match() const
        {
            return f_match == &snap::dispatcher<T>::dispatcher_match::always_match;
        }

        /** \brief Check whether f_match is always_match().
         *
         * This function checks whether the f_match function was defined
         * to always_match() and if so returns true.
         *
         * \return true if f_match is the always_match() function.
         */
        bool match_is_callback_match() const
        {
            return f_match == &snap::dispatcher<T>::dispatcher_match::callback_match;
        }
    };

private:
    /** \brief The connection pointer.
     *
     * This parameter is set by the constructor. It represents the
     * connection this dispatcher was added to (a form of parent of
     * this dispatcher object.)
     */
    T *                         f_connection = nullptr;

    /** \brief The array of possible matches.
     *
     * This is the vector of your messages with the corresponding
     * match and execute functions. This is used to go through
     * the matches and execute (dispatch) as required.
     */
    typename snap::dispatcher<T>::dispatcher_match::vector_t  f_matches = {};

    /** \brief Tell whether messages should be traced or not.
     *
     * Because your service may accept and send many messages a full
     * trace on all of them can really be resource intensive. By default
     * the system will not trace anything. By setting this parameter to
     * true (call set_trace() for that) you request the SNAP_LOG_TRACE()
     * to run on each message received by this dispatcher. This is done
     * on entry so whether the message is processed by the dispatcher
     * or your own send_message() function, it will trace that message.
     */
    bool                        f_trace = false;

public:

    /** \brief Initialize the dispatcher with your connection and messages.
     *
     * This function takes a pointer to your connection and an array
     * of matches.
     *
     * Whenever a message is received by one of your connections, the
     * dispatch() function gets called which checks the message against
     * each entry in this array of \p matches.
     *
     * \param[in] connection  The connection for which this dispatcher is
     *                        created.
     * \param[in] matches  The array of dispatch keywords and functions.
     * \param[in] matches_size  The sizeof() of the matches array.
     */
    dispatcher<T>(T * connection, typename snap::dispatcher<T>::dispatcher_match::vector_t matches)
        : f_connection(connection)
        , f_matches(matches)
    {
    }

    // prevent copies
    dispatcher<T>(dispatcher<T> const & rhs) = delete;
    dispatcher<T> & operator = (dispatcher<T> const & rhs) = delete;

    /** \brief Add a default array of possible matches.
     *
     * In Snap! a certain number of messages are always exactly the same
     * and these can be implemented internally so each daemon doesn't have
     * to duplicate that work over and over again. These are there in part
     * because the snapcommunicator expects those messages there.
     *
     * IMPORTANT NOTE: If you add your own version in your dispatcher_match
     * vector, then these will be ignored since your version will match first
     * and the dispatcher uses the first function only.
     *
     * This array currently includes:
     *
     * \li HELP -- msg_help() -- returns the list of all the messages
     * \li LOG -- msg_log() -- reconfigure() the logger
     * \li QUITTING -- msg_quitting() -- calls stop(true);
     * \li READY -- msg_ready() -- calls ready() -- snapcommunicator always
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
     * The HELP response is automatically built from the f_matches.f_expr
     * strings. However, if the function used to match the expression is
     * not one_to_one_match(), then that string doesn't get used.
     *
     * If any message can't be determine (i.e. the function is not the
     * one_to_one_match()) then the user help() function gets called and
     * we expect that function to add any dynamic message the daemon
     * understands.
     *
     * The LOG message reconfigures the logger if the is_configure() function
     * says it is configured. In any other circumstances, nothing happens.
     *
     * Note that the UNKNOWN message is understood and just logs the message
     * received. This allows us to see that WE sent a message that the receiver
     * (not us) does not understand and adjust our code accordingly (i.e. add
     * support for that message in that receiver or maybe fixed the spelling.)
     */
    void add_snap_communicator_commands()
    {
        // avoid more than one realloc()
        //
        f_matches.reserve(f_matches.size() + 7);

        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "HELP";
            m.f_execute = &T::msg_help;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "ALIVE";
            m.f_execute = &T::msg_alive;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "LOG";
            m.f_execute = &T::msg_log;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "QUITTING";
            m.f_execute = &T::msg_quitting;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "READY";
            m.f_execute = &T::msg_ready;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "RESTART";
            m.f_execute = &T::msg_restart;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "STOP";
            m.f_execute = &T::msg_stop;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = "UNKNOWN";
            m.f_execute = &T::msg_log_unknown;
            //m.f_match = <default>;
            f_matches.push_back(m);
        }
        {
            typename snap::dispatcher<T>::dispatcher_match m;
            m.f_expr = nullptr; // any other message
            m.f_execute = &T::msg_reply_with_unknown;
            m.f_match = &snap::dispatcher<T>::dispatcher_match::always_match;
            f_matches.push_back(m);
        }
    }

    typename snap::dispatcher<T>::dispatcher_match::vector_t const & get_matches() const
    {
        return f_matches;
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
     * You may not include a message in the array of `dispatch_match`
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
    virtual bool dispatch(::ed::message & msg) override
    {
        if(f_trace)
        {
            SNAP_LOG_TRACE
                << "dispatch message \""
                << msg.to_message()
                << "\".";
        }

        // go in order to execute matches
        //
        // remember that a dispatcher with just a set of well defined command
        // names is a special case (albeit frequent) and we can't process
        // using a map (a.k.a. fast binary search) as a consequence
        //
        for(auto const & m : f_matches)
        {
            if(m.execute(f_connection, msg))
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
     * is to place the code within #ifdef/#endif as in:
     *
     * \code
     *     #ifdef _DEBUG
     *         my_dispatcher->set_trace();
     *     #endif
     * \endcode
     *
     * \param[in] trace  Set to true to get SNAP_LOG_TRACE() of each message.
     */
    void set_trace(bool trace = true)
    {
        f_trace = trace;
    }

    /** \brief Retrieve the list of commands.
     *
     * This function transforms the vector of f_matches in a list of
     * commands in a QStringList.
     *
     * \param[in,out] commands  The place where the list of commands is saved.
     *
     * \return true if the commands were all determined, false if some need
     *         help from the user of this dispatcher.
     */
    virtual bool get_commands(string_list_t & commands) override
    {
        bool need_user_help(false);
        for(auto const & m : f_matches)
        {
            if(m.f_expr == nullptr)
            {
                if(!m.match_is_always_match()
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
            else if(m.match_is_one_to_one_match())
            {
                // add the f_expr as is since it represents a command
                // as is
                //
                // note: the fromUtf8() is not extremely important
                //       since commands have to be uppercase letters
                //       and digits and the underscore it would work
                //       with fromLatin1() too
                //
                commands << m.f_expr;
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
};



} // namespace ed
// vim: ts=4 sw=4 et

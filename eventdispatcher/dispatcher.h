// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/dispatcher_base.h"
#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/utils.h"


// snaplogger
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
 *          ed::dispatcher<my_connection>::define_match(
 *              ed::dispatcher<my_connection>::Expression("HELP")
 *            , ed::dispatcher<my_connection>::Execute(&my_connection::msg_help)
 *            //, ed::dispatcher<my_connection>::MatchFunc(&ed::dispatcher<my_connection>::dispatcher_match::one_to_one_match) -- use default
 *          ),
 *          ed::dispatcher<my_connection>::define_match(
 *              ed::dispatcher<my_connection>::Expression("STATUS")
 *            , ed::dispatcher<my_connection>::Execute(&my_connection::msg_status)
 *            //, ed::dispatcher<my_connection>::MatchFunc(&dispatcher<my_connection>::dispatcher_match::one_to_one_match) -- use default
 *          },
 *          ... // other messages
 *
 *          // if you'd like you can end your list with a catch all which
 *          // generate the UNKNOWN message with the following (not required)
 *          // if you have that entry, your own process_message() function
 *          // will not get called
 *          ed::dispatcher<my_connection>::define_catch_all()
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

        /** \brief Always returns MATCH_TRUE.
         *
         * This function always returns MATCH_TRUE. This is practical to
         * close your list of messages and return a specific message. In
         * most cases this is used to reply with the UNKNOWN message.
         *
         * \param[in] expr  The expression to compare the command against.
         * \param[in] msg  The message to match against this expression.
         *
         * \return Always returns MATCH_TRUE.
         */
        static match_t always_match(std::string const & expr, message & msg)
        {
            snapdev::NOT_USED(expr, msg);
            return match_t::MATCH_TRUE;
        }

        /** \brief Always returns MATCH_CALLBACK.
         *
         * This function always returns MATCH_CALLBACK. It is used
         * to call the f_execute function as a callback. The processing
         * continues after calling a callback function (i.e. the
         * execute() function returns false, meaning that the message
         * was not yet processed). This is useful if you want to execute
         * some code against many or all messages before actually
         * processing the messages individually.
         *
         * \param[in] expr  The expression is ignored.
         * \param[in] msg  The message is ignored.
         *
         * \return Always returns MATCH_CALLBACK.
         */
        static match_t callback_match(std::string const & expr, message & msg)
        {
            snapdev::NOT_USED(expr, msg);
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
         * ready/prepared data on load).
         *
         * The functions called have `this` defined so you can access
         * your connection data and other functions. It requires the
         * `&` and the class name to define the pointer, like this:
         *
         * \code
         *      &MyClass::my_message_function
         * \endcode
         *
         * The execution is started by calling the execute() function.
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
         * Note that the dispatch_match has two functions: one to match
         * the message against the dispatch_match and one to execute when
         * f_match() returns MATCH_TRUE or MATCH_CALLBACK. That way only
         * the matching functions get called. Note that on MATCH_CALLBACK
         * the function returns `false` (i.e. continue to loop through
         * the supported messages). The MATCH_CALLBACK feature is rarely
         * used.
         *
         * \param[in] connection  The connection attached to that
         *                        `dispatcher_match`.
         * \param[in] msg  The message that matched.
         *
         * \return true if the connection execute function was called.
         */
        bool execute(T * connection, ::ed::message & msg) const
        {
            match_t m(f_match(f_expr == nullptr ? std::string() : f_expr, msg));
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
            return f_match == &ed::dispatcher<T>::dispatcher_match::one_to_one_match;
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
            return f_match == &ed::dispatcher<T>::dispatcher_match::always_match;
        }

        /** \brief Check whether f_match is callback_match().
         *
         * This function checks whether the f_match function was defined
         * to callback_match() and if so returns true.
         *
         * \return true if f_match is the callback_match() function.
         */
        bool match_is_callback_match() const
        {
            return f_match == &ed::dispatcher<T>::dispatcher_match::callback_match;
        }
    };

    template<typename V>
    class MatchValue
    {
    public:
        typedef V   value_t;

        MatchValue<V>(V const v)
            : f_value(v)
        {
        }

        value_t get() const
        {
            return f_value;
        }

    private:
        value_t     f_value;
    };

    class Expression
        : public MatchValue<char const *>
    {
    public:
        Expression()
            : MatchValue<char const *>(nullptr)
        {
        }

        Expression(char const * expr)
            : MatchValue<char const *>(expr == nullptr || *expr == '\0' ? nullptr : expr)
        {
        }
    };

    class Execute
        : public MatchValue<typename ed::dispatcher<T>::dispatcher_match::execute_func_t>
    {
    public:
        Execute()
            : MatchValue<typename ed::dispatcher<T>::dispatcher_match::execute_func_t>(nullptr)
        {
        }

        Execute(typename ed::dispatcher<T>::dispatcher_match::execute_func_t expr)
            : MatchValue<typename ed::dispatcher<T>::dispatcher_match::execute_func_t>(expr)
        {
        }
    };

    class MatchFunc
        : public MatchValue<typename ed::dispatcher<T>::dispatcher_match::match_func_t>
    {
    public:
        MatchFunc()
            : MatchValue<typename ed::dispatcher<T>::dispatcher_match::match_func_t>(&::ed::dispatcher<T>::dispatcher_match::one_to_one_match)
        {
        }

        MatchFunc(typename ed::dispatcher<T>::dispatcher_match::match_func_t match)
            : MatchValue<typename ed::dispatcher<T>::dispatcher_match::match_func_t>(match == nullptr ? &::ed::dispatcher<T>::dispatcher_match::one_to_one_match : match)
        {
        }
    };

    template<typename V, typename F, class ...ARGS>
    static
    typename std::enable_if<std::is_same<V, F>::value, typename V::value_t>::type
    find_match_value(F first, ARGS ...args)
    {
        snapdev::NOT_USED(args...);
        return first.get();
    }

    template<typename V, typename F, class ...ARGS>
    static
    typename std::enable_if<!std::is_same<V, F>::value, typename V::value_t>::type
    find_match_value(F first, ARGS ...args)
    {
        snapdev::NOT_USED(first);
        return find_match_value<V>(args...);
    }

    template<class ...ARGS>
    static
    ed::dispatcher<T>::dispatcher_match define_match(ARGS ...args)
    {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
        ed::dispatcher<T>::dispatcher_match match =
        {
            .f_expr =    find_match_value<Expression  >(args..., Expression()),
            .f_execute = find_match_value<Execute     >(args..., Execute()),
            .f_match =   find_match_value<MatchFunc   >(args..., MatchFunc()),
        };
    #pragma GCC diagnostic pop

        if(match.f_execute == nullptr)
        {
            throw std::logic_error("an execute function is required, it cannot be set to nullptr.");
        }

        if(match.f_match == &ed::dispatcher<T>::dispatcher_match::one_to_one_match
        && match.f_expr == nullptr)
        {
            // although it works (won't crash) a message command cannot be
            // the empty string so we forbid that in our tables
            //
            throw std::logic_error("an expression is required for the one_to_one_match().");
        }

        return match;
    }

    static
    ed::dispatcher<T>::dispatcher_match define_catch_all()
    {
        return define_match(
              Execute(&T::msg_reply_with_unknown)
            , MatchFunc(&ed::dispatcher<T>::dispatcher_match::always_match));
    }

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
    typename ed::dispatcher<T>::dispatcher_match::vector_t  f_matches = {};

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
     */
    dispatcher<T>(T * connection, typename ed::dispatcher<T>::dispatcher_match::vector_t matches)
        : f_connection(connection)
        , f_matches(matches)
    {
    }

    // prevent copies
    dispatcher<T>(dispatcher<T> const &) = delete;
    dispatcher<T> & operator = (dispatcher<T> const &) = delete;

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
     * \li ALIVE -- msg_alive() -- auto-reply with ABSOLUTELY
     * \li HELP -- msg_help() -- returns the list of all the messages
     * \li LEAK -- msg_leak() -- log memory usage
     * \li LOG_ROTATE -- msg_log_rotate() -- reopen() the logger
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
     * \param[in] auto_catch_all  If true, automatically catch all other
     * messages and reply with `UNKNOWN`.
     */
    void add_communicator_commands(bool auto_catch_all = true)
    {
        // avoid more than one realloc()
        //
        f_matches.reserve(f_matches.size() + 11);

        f_matches.push_back(define_match(Expression("ALIVE"),               Execute(&T::msg_alive)));
        f_matches.push_back(define_match(Expression("HELP"),                Execute(&T::msg_help)));
        f_matches.push_back(define_match(Expression("LEAK"),                Execute(&T::msg_leak)));
        f_matches.push_back(define_match(Expression("LOG_ROTATE"),          Execute(&T::msg_log_rotate)));
        f_matches.push_back(define_match(Expression("QUITTING"),            Execute(&T::msg_quitting)));
        f_matches.push_back(define_match(Expression("READY"),               Execute(&T::msg_ready)));
        f_matches.push_back(define_match(Expression("RESTART"),             Execute(&T::msg_restart)));
        f_matches.push_back(define_match(Expression("SERVICE_UNAVAILABLE"), Execute(&T::msg_service_unavailable)));
        f_matches.push_back(define_match(Expression("STOP"),                Execute(&T::msg_stop)));
        f_matches.push_back(define_match(Expression("UNKNOWN"),             Execute(&T::msg_log_unknown)));

        // always last
        //
        if(auto_catch_all)
        {
            f_matches.push_back(define_catch_all());
        }
    }

    typename ed::dispatcher<T>::dispatcher_match::vector_t const & get_matches() const
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
    virtual bool dispatch(::ed::message & msg) override
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
    void set_trace(bool trace = true)
    {
        f_trace = trace;
    }

    /** \brief Retrieve the list of commands.
     *
     * This function transforms the vector of f_matches in a list of
     * commands in a string_list_t.
     *
     * \param[in,out] commands  The place where the list of commands is saved.
     *
     * \return false if the commands were all determined, true if some need
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
                commands.push_back(m.f_expr);
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

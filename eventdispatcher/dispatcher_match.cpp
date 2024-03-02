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
#include    "eventdispatcher/dispatcher_match.h"

#include    "eventdispatcher/connection_with_send_message.h"


// last include
//
#include    <snapdev/poison.h>



namespace ed
{




/** \struct dispatcher_match
 * \brief This structure is used to define the list of supported messages.
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


/** \typedef dispatcher_match::vector_t
 * \brief Define a vector of dispatcher_match objects.
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


/** \typedef dispatcher_match::execute_callback_t
 * \brief The execution callback.
 *
 * This type defines the execution callback. A standard function that
 * can be added dynamically, especially useful to add dynamic function
 * for intermediaries added to a dispatcher. Note that you must either
 * add an execution function or an execution callback.
 */


/** \enum match_t
 * \brief The match function return types.
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
 * a way to tweak the message before other functions receive it.
 */


/** \typedef match_func_t
 * \brief The match function type.
 *
 * This type defines the match function. We give it the message
 * which has the command name, although specialized matching
 * function could test other parameters from the message such
 * as the origination of the message.
 */


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
match_t one_to_one_match(std::string const & expr, message & msg)
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
match_t always_match(std::string const & expr, message & msg)
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
match_t callback_match(std::string const & expr, message & msg)
{
    snapdev::NOT_USED(expr, msg);
    return match_t::MATCH_CALLBACK;
}


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
 * \param[in] msg  The message that matched.
 *
 * \return true if the connection execute function was called.
 */
bool dispatcher_match::execute(message & msg) const
{
    match_t const m(f_match(f_expr == nullptr ? std::string() : f_expr, msg));
    if(m == match_t::MATCH_TRUE
    || m == match_t::MATCH_CALLBACK)
    {
        if(f_callback == nullptr)
        {
            throw invalid_callback("neither f_execute nor f_callback was defined.");
        }
        f_callback(msg);
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
bool dispatcher_match::match_is_one_to_one_match() const
{
    return f_match == &one_to_one_match;
}


/** \brief Check whether f_match is always_match().
 *
 * This function checks whether the f_match function was defined
 * to always_match() and if so returns true.
 *
 * \return true if f_match is the always_match() function.
 */
bool dispatcher_match::match_is_always_match() const
{
    return f_match == &always_match;
}


/** \brief Check whether f_match is callback_match().
 *
 * This function checks whether the f_match function was defined
 * to callback_match() and if so returns true.
 *
 * \return true if f_match is the callback_match() function.
 */
bool dispatcher_match::match_is_callback_match() const
{
    return f_match == &callback_match;
}








/** \var dispatcher_match::f_expr
 * \brief The expression to compare against.
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


/** \var dispatcher_match::f_callback
 * \brief The callback function.
 *
 * This is an std::function which can be bound at run time. Note
 * that if f_execute is not nullptr, then this callback function
 * is ignored.
 */


/** \var dispatcher_match::f_match
 * \brief The match function.
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



} // namespace ed
// vim: ts=4 sw=4 et

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
#pragma once

/** \file
 * \brief Declaration of the dispatcher_match class.
 *
 * Class used to define events handled by the dispatcher.
 */


// self
//
#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/message.h>


// C++
//
#include    <functional>



namespace ed
{


enum class match_t
{
    MATCH_FALSE,
    MATCH_TRUE,
    MATCH_CALLBACK
};

typedef match_t (*match_func_t)(std::string const & expr, message & msg);

match_t      one_to_one_match(std::string const & expr, message & msg);
match_t      always_match(std::string const & expr, message & msg);
match_t      callback_match(std::string const & expr, message & msg);

struct dispatcher_match
{
    typedef std::vector<dispatcher_match>       vector_t;
    typedef std::function<void(message & msg)>  execute_callback_t;
    typedef int                                 tag_t;

    constexpr static tag_t const                DISPATCHER_MATCH_NO_TAG = 0;

    bool                execute(message & msg) const;
    bool                match_is_one_to_one_match() const;
    bool                match_is_always_match() const;
    bool                match_is_callback_match() const;

    char const *        f_expr = nullptr;
    execute_callback_t  f_callback = execute_callback_t();
    match_func_t        f_match = &one_to_one_match;
    tag_t               f_tag = DISPATCHER_MATCH_NO_TAG;
};



template<typename V>
class MatchValue
{
public:
    typedef V   value_t;

    MatchValue(V const v)
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



class Callback
    : public MatchValue<typename dispatcher_match::execute_callback_t>
{
public:
    Callback()
        : MatchValue<typename dispatcher_match::execute_callback_t>(nullptr)
    {
    }

    Callback(typename dispatcher_match::execute_callback_t callback)
        : MatchValue<typename dispatcher_match::execute_callback_t>(callback)
    {
    }
};



class MatchFunc
    : public MatchValue<match_func_t>
{
public:
    MatchFunc()
        : MatchValue<match_func_t>(&one_to_one_match)
    {
    }

    MatchFunc(match_func_t match)
        : MatchValue<match_func_t>(match == nullptr ? &one_to_one_match : match)
    {
    }
};



class Tag
    : public MatchValue<dispatcher_match::tag_t>
{
public:
    Tag()
        : MatchValue<dispatcher_match::tag_t>(0)
    {
    }

    Tag(dispatcher_match::tag_t tag)
        : MatchValue<dispatcher_match::tag_t>(tag)
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
dispatcher_match define_match(ARGS ...args)
{
    dispatcher_match match =
    {
        .f_expr =     find_match_value<Expression  >(args..., Expression()),
        .f_callback = find_match_value<Callback    >(args...),
        .f_match =    find_match_value<MatchFunc   >(args..., MatchFunc()),
        .f_tag =      find_match_value<Tag         >(args..., Tag()),
    };

    if(match.f_callback == nullptr)
    {
        throw parameter_error("a callback function is required in dispatcher_match, it cannot be set to nullptr.");
    }

    if(match.f_match == &one_to_one_match
    && match.f_expr == nullptr)
    {
        // although it works (won't crash) a message command cannot be
        // the empty string so we forbid that in our tables
        //
        throw parameter_error("an expression is required for the one_to_one_match().");
    }

    return match;
}








#define DISPATCHER_MATCH(command, function) \
        ::ed::define_match( \
              ::ed::Expression(command) \
            , ::ed::Callback(std::bind(function, this, std::placeholders::_1)) \
        )

#define DISPATCHER_CATCH_ALL() \
        ::ed::define_match( \
              ::ed::Callback(std::bind(&::ed::connection_with_send_message::msg_reply_with_unknown, this, std::placeholders::_1)) \
            , ::ed::MatchFunc(&::ed::always_match) \
        )



} // namespace ed
// vim: ts=4 sw=4 et

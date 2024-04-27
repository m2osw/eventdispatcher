// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "executor.h"

#include    "variable_address.h"
#include    "variable_floating_point.h"
#include    "variable_integer.h"
#include    "variable_list.h"
#include    "variable_regex.h"
#include    "variable_string.h"
#include    "variable_timestamp.h"
#include    "variable_void.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{


namespace
{



enum class step_t
{
    STEP_START,     // start the thread
    STEP_DONE,      // we're done here
    STEP_CONTINUE,  // just continue as is
};


class background_executor
    : public cppthread::runner
{
public:
    typedef std::shared_ptr<background_executor>    pointer_t;

                            background_executor(
                                  state::pointer_t s
                                , ed::thread_done_signal::pointer_t done_signal);

    // cppthread::runner implementation
    //
    virtual void            enter() override;
    virtual void            run() override;
    virtual void            leave(cppthread::leave_status_t status) override;

    step_t                  execute_instruction();
    variable::pointer_t     primary_to_variable(expression::pointer_t value, std::string const & name);
    expression::pointer_t   compute(expression::pointer_t expr);
    state::pointer_t        get_state() const;

private:
    state::pointer_t        f_state = state::pointer_t();
    ed::thread_done_signal::pointer_t
                            f_done_signal = ed::thread_done_signal::pointer_t();
};


background_executor::background_executor(
          state::pointer_t s
        , ed::thread_done_signal::pointer_t done_signal)
    : runner("background_executor")
    , f_state(s)
    , f_done_signal(done_signal)
{
}


void background_executor::enter()
{
    f_state->set_in_thread(true);
}


void background_executor::run()
{
    while(continue_running())
    {
        if(execute_instruction() == step_t::STEP_DONE)
        {
            break;
        }
    }
}


void background_executor::leave(cppthread::leave_status_t status)
{
    f_done_signal->thread_done();
    f_state->set_in_thread(false);

    // if the status is INSTRUMENTATION, then we already received an exception
    //
    switch(status)
    {
    case cppthread::leave_status_t::LEAVE_STATUS_NORMAL:
    case cppthread::leave_status_t::LEAVE_STATUS_INSTRUMENTATION:
        break;

    default:
        throw std::runtime_error("thread failed with leave status: " + std::to_string(static_cast<int>(status)));

    }
}


step_t background_executor::execute_instruction()
{
    state::trace_callback_t trace(f_state->get_trace_callback());

    ip_t const ip(f_state->get_ip());
    if(ip >= f_state->get_statement_size())
    {
        return step_t::STEP_DONE;
    }
    statement::pointer_t stmt(f_state->get_statement(ip));
    f_state->set_running_statement(stmt);
    f_state->set_ip(ip + 1);

    f_state->clear_parameters();

    instruction::pointer_t inst(stmt->get_instruction());
    if(inst->get_name() == "run")
    {
        if(f_state->get_in_thread())
        {
            throw std::runtime_error("run() instruction found when already running in the background.");
        }
        return step_t::STEP_START;
    }

    // transform expressions from the statement in parameters in
    // the state before calling the function
    //
    for(parameter_declaration const * decls(inst->parameter_declarations());
        decls->f_name != nullptr;
        ++decls)
    {
        expression::pointer_t expr(stmt->get_parameter(decls->f_name));
        if(expr == nullptr)
        {
            if(decls->f_required)
            {
                // LCOV_EXCL_START
                throw std::runtime_error(
                      "parameter \""
                    + std::string(decls->f_name)
                    + "\" was expected for instruction \""
                    + stmt->get_instruction()->get_name()
                    + "\".");
                // LCOV_EXCL_STOP
            }
            continue;
        }
        expression::pointer_t value(compute(expr));
        variable::pointer_t param;
        switch(value->get_operator())
        {
        case operator_t::OPERATOR_PRIMARY:
            param = primary_to_variable(value, decls->f_name);
            break;

        case operator_t::OPERATOR_LIST:
            // go through each element and convert them
            //
            param = std::make_shared<variable_list>(decls->f_name);
            for(std::size_t idx(0); idx < value->get_expression_size(); ++idx)
            {
                expression::pointer_t item(value->get_expression(idx));
                if(item->get_operator() != operator_t::OPERATOR_NAMED)
                {
                    throw std::logic_error("list item was not an OPERATOR_NAMED expression."); // LCOV_EXCL_LINE
                }
                std::size_t const count(item->get_expression_size());
                switch(count)
                {
                case 1:
                case 2:
                    break;

                // LCOV_EXCL_START
                default:
                    throw std::logic_error("OPERATOR_NAMED list item expression does not have one or two items.");
                // LCOV_EXCL_STOP

                }
                expression::pointer_t name_expr(item->get_expression(0));
                token t(name_expr->get_token());
                if(t.get_token() != token_t::TOKEN_IDENTIFIER)
                {
                    throw std::logic_error("OPERATOR_NAMED first item is not an identifier."); // LCOV_EXCL_LINE
                }
                std::string const & name(t.get_string());
                variable::pointer_t p;
                if(count == 1)
                {
                    p = std::make_shared<variable_void>(name);
                }
                else
                {
                    p = primary_to_variable(item->get_expression(1), name);
                }
                std::static_pointer_cast<variable_list>(param)->add_item(p);
            }
            break;

        // LCOV_EXCL_START
        default:
            throw std::runtime_error("operator type not supported to convert expression to variable.");
        // LCOV_EXCL_STOP

        }
        if(decls->f_type != param->get_type())
        {
            if(strcmp(decls->f_type, "any") != 0
            && (strcmp(decls->f_type, "number") != 0
                || (param->get_type() != "integer"
                    && param->get_type() != "floating_point")))
            {
                throw std::runtime_error(
                      std::string("parameter type mismatch for ")
                    + decls->f_name
                    + ", expected \""
                    + decls->f_type
                    + "\", got \""
                    + param->get_type()
                    + "\" instead.");
            }
        }

        f_state->add_parameter(param);
    }

    if(trace != nullptr)
    {
        trace(*f_state, callback_reason_t::CALLBACK_REASON_BEFORE_CALL);
    }

    inst->func(*f_state);

    if(trace != nullptr)
    {
        trace(*f_state, callback_reason_t::CALLBACK_REASON_AFTER_CALL);
    }

    return step_t::STEP_CONTINUE;
}


variable::pointer_t background_executor::primary_to_variable(expression::pointer_t value, std::string const & name)
{
    variable::pointer_t param;
    token t(value->get_token());
    switch(t.get_token())
    {
    case token_t::TOKEN_VARIABLE:
        param = f_state->get_variable(t.get_string());
        if(param == nullptr)
        {
            // empty string or void?
            //
            param = std::make_shared<variable_string>(name, "string");
        }
        else
        {
            // we need to create a clone because the name is likely different
            //
            param = param->clone(name);
        }
        break;

    case token_t::TOKEN_IDENTIFIER:
        param = std::make_shared<variable_string>(name, "identifier");
        std::static_pointer_cast<variable_string>(param)->set_string(t.get_string());
        break;

    case token_t::TOKEN_FLOATING_POINT:
        param = std::make_shared<variable_floating_point>(name);
        std::static_pointer_cast<variable_floating_point>(param)->set_floating_point(t.get_floating_point());
        break;

    case token_t::TOKEN_INTEGER:
        param = std::make_shared<variable_integer>(name);
        std::static_pointer_cast<variable_integer>(param)->set_integer(t.get_integer());
        break;

    case token_t::TOKEN_REGEX:
        param = std::make_shared<variable_regex>(name);
        std::static_pointer_cast<variable_regex>(param)->set_regex(t.get_string());
        break;

    case token_t::TOKEN_SINGLE_STRING:
        param = std::make_shared<variable_string>(name, "string");
        std::static_pointer_cast<variable_string>(param)->set_string(t.get_string());
        break;

    case token_t::TOKEN_DOUBLE_STRING:
        {
            param = std::make_shared<variable_string>(name, "string");
            std::string original(t.get_string());
            std::string replaced;
            for(std::string::size_type pos(0); pos < original.length();)
            {
                std::string::size_type dollar(original.find('$', pos));
                if(dollar == std::string::npos)
                {
                    replaced += original.substr(pos);
                    break;
                }
                if(dollar > pos)
                {
                    replaced += original.substr(pos, dollar - pos);
                }
                if(dollar + 1 >= original.length())
                {
                    replaced += '$';
                    break;
                }
                if(original[dollar + 1] == '{')
                {
                    // found start of variable declaration
                    //
                    dollar += 2;
                    std::string::size_type end(original.find('}', dollar));
                    if(end == std::string::npos)
                    {
                        throw std::runtime_error("found unclosed variable in \"" + original + "\".");
                    }
                    std::string const var_name(original.substr(dollar, end - dollar));;
                    pos = end + 1;
                    if(var_name.empty())
                    {
                        throw std::runtime_error("found variable without a name in \"" + original + "\".");
                    }
                    variable::pointer_t var(f_state->get_variable(var_name));
                    if(var != nullptr)
                    {
                        if(var->get_type() == "string")
                        {
                            replaced += std::dynamic_pointer_cast<variable_string>(var)->get_string();
                        }
                        else if(var->get_type() == "integer")
                        {
                            replaced += std::to_string(std::dynamic_pointer_cast<variable_integer>(var)->get_integer());
                        }
                        else if(var->get_type() == "floating_point")
                        {
                            std::string floating_point_value(std::to_string(std::dynamic_pointer_cast<variable_floating_point>(var)->get_floating_point()));
                            std::string::size_type const decimal_point_pos(floating_point_value.find('.'));
                            if(decimal_point_pos != std::string::npos)
                            {
                                std::string::size_type zero_pos(floating_point_value.length());
                                while(zero_pos > decimal_point_pos)
                                {
                                    --zero_pos;
                                    if(floating_point_value[zero_pos] != '0')
                                    {
                                        break;
                                    }
                                }
                                if(zero_pos != '.')
                                {
                                    ++zero_pos;
                                }
                                floating_point_value.resize(zero_pos);
                            }
                            replaced += floating_point_value;
                        }
                        else if(var->get_type() == "timestamp")
                        {
                            replaced += std::dynamic_pointer_cast<variable_timestamp>(var)->get_timestamp().to_timestamp();
                        }
                        else if(var->get_type() == "address")
                        {
                            addr::addr const a(std::dynamic_pointer_cast<variable_address>(var)->get_address());
                            replaced += a.to_ipv4or6_string(
                                              addr::STRING_IP_BRACKET_ADDRESS
                                            | (a.get_port() != 0 ? addr::STRING_IP_PORT : 0)
                                            | addr::STRING_IP_MASK_IF_NEEDED);
                        }
                        else
                        {
                            throw std::runtime_error(
                                  "found variable of type \"" 
                                + var->get_type()
                                + "\" which is not yet supported in ${...}.");
                        }
                    }
                }
                else
                {
                    // a variable must be introduced with "${" if not then
                    // it is just a stand alone dollar character
                    //
                    replaced += '$';
                    ++pos;
                }
            }
            std::static_pointer_cast<variable_string>(param)->set_string(replaced);
        }
        break;

    case token_t::TOKEN_ADDRESS:
        {
            addr::addr_parser p;
            p.set_protocol("tcp");
            p.set_allow(addr::allow_t::ALLOW_MASK, true);
            addr::addr_range::vector_t const a(p.parse(t.get_string()));
            param = std::make_shared<variable_address>(name);
            std::static_pointer_cast<variable_address>(param)->set_address(a[0].get_from());
        }
        break;

    case token_t::TOKEN_TIMESPEC:
        {
            param = std::make_shared<variable_timestamp>(name);
            snapdev::timespec_ex timestamp(
                t.get_integer() >> 64,
                t.get_integer()
            );
            std::static_pointer_cast<variable_timestamp>(param)->set_timestamp(timestamp);
        }
        break;

    // LCOV_EXCL_START
    default:
        throw std::runtime_error(
              "support for primary \""
            + std::to_string(static_cast<int>(t.get_token()))
            + "\" not yet implemented.");
    // LCOV_EXCL_STOP

    }

    return param;
}


expression::pointer_t background_executor::compute(expression::pointer_t expr)
{
    if(expr == nullptr)
    {
        throw std::logic_error("compute() called with a nullptr."); // LCOV_EXCL_LINE
    }

    constexpr auto mix_token = [](token_t l, token_t r)
    {
        return (static_cast<int>(l) << 16) | static_cast<int>(r);
    };
    switch(expr->get_operator())
    {
    case operator_t::OPERATOR_PRIMARY:
        {
            token t(expr->get_token());
            if(t.get_token() == token_t::TOKEN_VARIABLE)
            {
                expr = std::make_shared<expression>();
                expr->set_operator(operator_t::OPERATOR_PRIMARY);

                token value;
                variable::pointer_t param(f_state->get_variable(t.get_string()));
                if(param != nullptr)
                {
                    // we need to create a clone because the name is likely different
                    //
                    std::string const & type(param->get_type());
                    if(type == "integer")
                    {
                        value.set_token(token_t::TOKEN_IDENTIFIER);
                        value.set_integer(std::static_pointer_cast<variable_integer>(param)->get_integer());
                    }
                    else if(type == "floating_point")
                    {
                        value.set_token(token_t::TOKEN_FLOATING_POINT);
                        value.set_floating_point(std::static_pointer_cast<variable_floating_point>(param)->get_floating_point());
                    }
                    else if(type == "string")
                    {
                        value.set_token(token_t::TOKEN_SINGLE_STRING);
                        value.set_string(std::static_pointer_cast<variable_string>(param)->get_string());
                    }
                    else if(type == "identifier")
                    {
                        value.set_token(token_t::TOKEN_IDENTIFIER);
                        value.set_string(std::static_pointer_cast<variable_string>(param)->get_string());
                    }
                    else if(type == "regex")
                    {
                        value.set_token(token_t::TOKEN_REGEX);
                        value.set_string(std::static_pointer_cast<variable_regex>(param)->get_regex());
                    }
                    else
                    {
                        throw std::runtime_error(
                              "primary variable type \""
                            + type
                            + "\" not yet supported.");
                    }
                }
                else
                {
                    value.set_token(token_t::TOKEN_SINGLE_STRING);
                }
                expr->set_token(value);
            }
        }
        return expr;

    case operator_t::OPERATOR_ADD:
        if(expr->get_expression_size() != 2)
        {
            throw std::logic_error("+ operator (add) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            expression::pointer_t r(compute(expr->get_expression(1)));
            token const & lt(l->get_token());
            token const & rt(r->get_token());

            token result;
            switch(mix_token(lt.get_token(), rt.get_token()))
            {
            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() + rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() + rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_integer() + rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(lt.get_integer() + rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_TIMESPEC, token_t::TOKEN_INTEGER):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(lt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(rt.get_integer(), 0);
                    timestamp += offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_TIMESPEC):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(rt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(lt.get_integer(), 0);
                    timestamp += offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_TIMESPEC, token_t::TOKEN_FLOATING_POINT):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(lt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(rt.get_floating_point());
                    timestamp += offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_TIMESPEC):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(rt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(lt.get_floating_point());
                    timestamp += offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_IDENTIFIER, token_t::TOKEN_IDENTIFIER):
                result.set_token(token_t::TOKEN_IDENTIFIER);
                result.set_string(lt.get_string() + rt.get_string());
                break;

            case mix_token(token_t::TOKEN_SINGLE_STRING, token_t::TOKEN_SINGLE_STRING):
            case mix_token(token_t::TOKEN_SINGLE_STRING, token_t::TOKEN_DOUBLE_STRING):
            case mix_token(token_t::TOKEN_DOUBLE_STRING, token_t::TOKEN_SINGLE_STRING):
                result.set_token(token_t::TOKEN_SINGLE_STRING);
                result.set_string(lt.get_string() + rt.get_string());
                break;

            case mix_token(token_t::TOKEN_DOUBLE_STRING, token_t::TOKEN_DOUBLE_STRING):
                result.set_token(token_t::TOKEN_DOUBLE_STRING);
                result.set_string(lt.get_string() + rt.get_string());
                break;

            case mix_token(token_t::TOKEN_SINGLE_STRING, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_SINGLE_STRING);
                result.set_string(lt.get_string() + std::to_string(static_cast<std::int64_t>(rt.get_integer())));
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_SINGLE_STRING):
                result.set_token(token_t::TOKEN_SINGLE_STRING);
                result.set_string(std::to_string(static_cast<std::int64_t>(lt.get_integer())) + rt.get_string());
                break;

            case mix_token(token_t::TOKEN_DOUBLE_STRING, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_DOUBLE_STRING);
                result.set_string(lt.get_string() + std::to_string(static_cast<std::int64_t>(rt.get_integer())));
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_DOUBLE_STRING):
                result.set_token(token_t::TOKEN_DOUBLE_STRING);
                result.set_string(std::to_string(static_cast<std::int64_t>(lt.get_integer())) + rt.get_string());
                break;

            case mix_token(token_t::TOKEN_ADDRESS, token_t::TOKEN_INTEGER):
                {
                    result.set_token(token_t::TOKEN_ADDRESS);

                    addr::addr_parser p;
                    p.set_protocol("tcp");
                    p.set_allow(addr::allow_t::ALLOW_MASK, true);
                    addr::addr_range::vector_t const la(p.parse(lt.get_string()));
                    addr::addr const a(la[0].get_from() + rt.get_integer());
                    result.set_string(a.to_ipv4or6_string(
                              addr::STRING_IP_BRACKET_ADDRESS
                            | addr::STRING_IP_PORT
                            | addr::STRING_IP_MASK_IF_NEEDED));
                }
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_ADDRESS):
                {
                    result.set_token(token_t::TOKEN_ADDRESS);

                    addr::addr_parser p;
                    p.set_protocol("tcp");
                    p.set_allow(addr::allow_t::ALLOW_MASK, true);
                    addr::addr_range::vector_t const ra(p.parse(rt.get_string()));
                    addr::addr const a(ra[0].get_from() + lt.get_integer());
                    result.set_string(a.to_ipv4or6_string(
                              addr::STRING_IP_BRACKET_ADDRESS
                            | addr::STRING_IP_PORT
                            | addr::STRING_IP_MASK_IF_NEEDED));
                }
                break;

            default:
                throw std::runtime_error(
                      "unsupported addition (token types: "
                    + std::to_string(static_cast<int>(lt.get_token()))
                    + " + "
                    + std::to_string(static_cast<int>(rt.get_token()))
                    + ").");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_SUBTRACT:
        if(expr->get_expression_size() != 2)
        {
            throw std::logic_error("- operator (subtract) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            expression::pointer_t r(compute(expr->get_expression(1)));
            token const & lt(l->get_token());
            token const & rt(r->get_token());

            token result;
            switch(mix_token(lt.get_token(), rt.get_token()))
            {
            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() - rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() - rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_integer() - rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(lt.get_integer() - rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_TIMESPEC, token_t::TOKEN_INTEGER):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(lt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(rt.get_integer(), 0);
                    timestamp -= offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_TIMESPEC):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(rt.get_integer());
                    snapdev::timespec_ex const timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex offset(lt.get_integer(), 0);
                    offset -= timestamp;
                    result.set_integer((static_cast<__int128>(offset.tv_sec) << 64) | offset.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_TIMESPEC, token_t::TOKEN_FLOATING_POINT):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(lt.get_integer());
                    snapdev::timespec_ex timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex const offset(rt.get_floating_point());
                    timestamp -= offset;
                    result.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_TIMESPEC):
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(rt.get_integer());
                    snapdev::timespec_ex const timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex offset(lt.get_floating_point());
                    offset -= timestamp;
                    result.set_integer((static_cast<__int128>(offset.tv_sec) << 64) | offset.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            case mix_token(token_t::TOKEN_ADDRESS, token_t::TOKEN_ADDRESS):
                {
                    result.set_token(token_t::TOKEN_INTEGER);

                    addr::addr_parser p;
                    p.set_protocol("tcp");
                    p.set_allow(addr::allow_t::ALLOW_MASK, true);
                    addr::addr_range::vector_t const la(p.parse(lt.get_string()));
                    addr::addr_range::vector_t const ra(p.parse(rt.get_string()));
                    result.set_integer(la[0].get_from() - ra[0].get_from());
                }
                break;

            case mix_token(token_t::TOKEN_ADDRESS, token_t::TOKEN_INTEGER):
                {
                    result.set_token(token_t::TOKEN_ADDRESS);

                    addr::addr_parser p;
                    p.set_protocol("tcp");
                    p.set_allow(addr::allow_t::ALLOW_MASK, true);
                    addr::addr_range::vector_t const la(p.parse(lt.get_string()));
                    addr::addr const a(la[0].get_from() - rt.get_integer());
                    result.set_string(a.to_ipv4or6_string(
                              addr::STRING_IP_BRACKET_ADDRESS
                            | addr::STRING_IP_PORT
                            | addr::STRING_IP_MASK_IF_NEEDED));
                }
                break;

            default:
                throw std::runtime_error("unsupported subtraction.");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_IDENTITY:
        if(expr->get_expression_size() != 1)
        {
            throw std::logic_error("+ operator (identity) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            return compute(expr->get_expression(0));
        }
        break;

    case operator_t::OPERATOR_NEGATE:
        if(expr->get_expression_size() != 1)
        {
            throw std::logic_error("+ operator (identity) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            token const & lt(l->get_token());

            token result;
            switch(lt.get_token())
            {
            case token_t::TOKEN_FLOATING_POINT:
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(-lt.get_floating_point());
                break;

            case token_t::TOKEN_INTEGER:
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(-lt.get_integer());
                break;

            case token_t::TOKEN_TIMESPEC:
                {
                    result.set_token(token_t::TOKEN_TIMESPEC);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    __int128 const timestamp_int(lt.get_integer());
                    snapdev::timespec_ex const timestamp(timestamp_int >> 64, timestamp_int & 0xffffffffffffffff);
                    snapdev::timespec_ex neg;
                    neg -= timestamp;
                    result.set_integer((static_cast<__int128>(neg.tv_sec) << 64) | neg.tv_nsec);
#pragma GCC diagnostic pop
                }
                break;

            default:
                throw std::runtime_error("unsupported negation.");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_MULTIPLY:
        if(expr->get_expression_size() != 2)
        {
            throw std::logic_error("* operator (multiply) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            expression::pointer_t r(compute(expr->get_expression(1)));
            token const & lt(l->get_token());
            token const & rt(r->get_token());

            token result;
            switch(mix_token(lt.get_token(), rt.get_token()))
            {
            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() * rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() * rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_integer() * rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(lt.get_integer() * rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_SINGLE_STRING, token_t::TOKEN_INTEGER):
            case mix_token(token_t::TOKEN_DOUBLE_STRING, token_t::TOKEN_INTEGER):
                {
                    result.set_token(lt.get_token());
                    std::string str(lt.get_string());
                    std::size_t const len(str.length());
                    int const count(rt.get_integer());
                    if(count < 0 || count > 1000)
                    {
                        throw std::runtime_error("string repeat needs to be positive and under 1001.");
                    }
                    std::size_t const size(len * count);
                    str.resize(size);
                    char const * s(str.data());
                    char * d(str.data() + len);
                    char const * e(s + size);
                    while(d < e)
                    {
                        *d++ = *s++;
                    }
                    result.set_string(str);
                }
                break;

            default:
                throw std::runtime_error("unsupported multiplication.");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_DIVIDE:
        if(expr->get_expression_size() != 2)
        {
            throw std::logic_error("/ operator (divide) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            expression::pointer_t r(compute(expr->get_expression(1)));
            token const & lt(l->get_token());
            token const & rt(r->get_token());

            token result;
            switch(mix_token(lt.get_token(), rt.get_token()))
            {
            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() / rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_floating_point() / rt.get_integer());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(lt.get_integer() / rt.get_floating_point());
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(lt.get_integer() / rt.get_integer());
                break;

            default:
                throw std::runtime_error("unsupported division.");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_MODULO:
        if(expr->get_expression_size() != 2)
        {
            throw std::logic_error("% operator (modulo) did not receive exactly two parameters."); // LCOV_EXCL_LINE
        }
        else
        {
            expression::pointer_t l(compute(expr->get_expression(0)));
            expression::pointer_t r(compute(expr->get_expression(1)));
            token const & lt(l->get_token());
            token const & rt(r->get_token());

            token result;
            switch(mix_token(lt.get_token(), rt.get_token()))
            {
            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(fmod(lt.get_floating_point(), rt.get_floating_point()));
                break;

            case mix_token(token_t::TOKEN_FLOATING_POINT, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(fmod(lt.get_floating_point(), rt.get_integer()));
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_FLOATING_POINT):
                result.set_token(token_t::TOKEN_FLOATING_POINT);
                result.set_floating_point(fmod(lt.get_integer(), rt.get_floating_point()));
                break;

            case mix_token(token_t::TOKEN_INTEGER, token_t::TOKEN_INTEGER):
                result.set_token(token_t::TOKEN_INTEGER);
                result.set_integer(lt.get_integer() % rt.get_integer());
                break;

            default:
                throw std::runtime_error("unsupported modulo.");

            }
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_PRIMARY);
            result_expr->set_token(result);
            return result_expr;
        }
        break;

    case operator_t::OPERATOR_LIST:
        {
            expression::pointer_t result_expr(std::make_shared<expression>());
            result_expr->set_operator(operator_t::OPERATOR_LIST);
            std::size_t const max(expr->get_expression_size());
            for(std::size_t idx(0); idx < max; ++idx)
            {
                expression::pointer_t named_expr(expr->get_expression(idx));
                if(named_expr->get_operator() != operator_t::OPERATOR_NAMED)
                {
                    throw std::logic_error("only named expressions are allowed in a list."); // LCOV_EXCL_LINE
                }
                expression::pointer_t new_named_expr(std::make_shared<expression>());
                new_named_expr->set_operator(operator_t::OPERATOR_NAMED);
                switch(named_expr->get_expression_size())
                {
                case 1:
                    new_named_expr->add_expression(named_expr->get_expression(0));
                    break;

                case 2:
                    {
                        new_named_expr->add_expression(named_expr->get_expression(0));
                        expression::pointer_t item(compute(named_expr->get_expression(1)));
                        new_named_expr->add_expression(item);
                    }
                    break;

                // LCOV_EXCL_START
                default:
                    throw std::logic_error("named expressions must have a name and an optional expression.");
                // LCOV_EXCL_STOP

                }
                result_expr->add_expression(new_named_expr);
            }
            return result_expr;
        }
        break;

    // LCOV_EXCL_START
    default:
        throw std::logic_error("unsupported expression type in compute().");
    // LCOV_EXCL_STOP

    }
    snapdev::NOT_REACHED();
    return expression::pointer_t();
}


state::pointer_t background_executor::get_state() const
{
    return f_state;
}


class executor_thread_done
    : public ed::thread_done_signal
{
public:
    typedef std::shared_ptr<executor_thread_done>   pointer_t;

    executor_thread_done()
        : thread_done_signal()
    {
        set_name("thread_done");
    }


    // thread_done_signal implementation
    //
    virtual void process_read() override
    {
        thread_done_signal::process_read();
        ed::communicator::instance()->remove_connection(shared_from_this());

        if(f_thread_done_callback != nullptr)
        {
            f_thread_done_callback();
        }
    }

    void set_thread_done_callback(executor::thread_done_callback_t callback)
    {
        f_thread_done_callback = callback;
    }

private:
    executor::thread_done_callback_t    f_thread_done_callback = executor::thread_done_callback_t();
};


} // no name namespace



executor::executor(state::pointer_t s)
    : f_done_signal(std::make_shared<executor_thread_done>())
    , f_runner(std::make_shared<background_executor>(s, f_done_signal))
    , f_thread(std::make_shared<cppthread::thread>("executor_thread", f_runner))
{
    f_thread->set_log_all_exceptions(true);
    ed::communicator::instance()->add_connection(f_done_signal);
}


executor::~executor()
{
    // do an explicit stop() so we can capture exceptions
    // if you do not want terminate() to be called, make sure to do that
    // just after your e->run() call
    //
    stop();
}


/** \brief Start execution.
 *
 * This function starts the script execution up to the run() instruction.
 *
 * When this function returns, you can implement your own code such as
 * create connections you want to test. Once done with that, call the
 * run() or the stop() functions.
 */
void executor::start()
{
    for(;;)
    {
        step_t const step(std::static_pointer_cast<background_executor>(f_runner)->execute_instruction());
        if(step == step_t::STEP_DONE)
        {
            // the thread was never started, but we still need to mark it as
            // done so the client exits the communicator::run() function.
            //
            f_done_signal->thread_done();
            return;
        }
        if(step == step_t::STEP_START)
        {
            break;
        }
    }

    // the thread takes over running the loop
    //
    state::trace_callback_t trace(std::static_pointer_cast<background_executor>(f_runner)->get_state()->get_trace_callback());
    if(trace != nullptr)
    {
        trace(*std::static_pointer_cast<background_executor>(f_runner)->get_state(), callback_reason_t::CALLBACK_REASON_BEFORE_CALL);
    }
    f_thread->start();
    if(trace != nullptr)
    {
        trace(*std::static_pointer_cast<background_executor>(f_runner)->get_state(), callback_reason_t::CALLBACK_REASON_AFTER_CALL);
    }
}


/** \brief Start the communicator loop.
 *
 * This is the next logical step after calling the start() function.
 *
 * This simply starts the communicator run() loop. It will exit once all
 * the connections were removed from the communicator. If your test does
 * not properly clean up a connection, then it will be stuck in this
 * loop.
 *
 * You must have called the start() function first to make sure that
 * the thread was started. If the start function does not start a
 * thread, you still need to call this function because the "thread
 * done" connection is in the list of communicator connections that
 * need clean up.
 *
 * \code
 *     SNAP_CATCH2_NAMESPACE::reporter::executor::pointer_t e(std::make_shared<SNAP_CATCH2_NAMESPACE::reporter::executor>(s));
 *     e->start();
 *
 *     // initialize your object(s)
 *
 *     e->set_thread_done_callback(...)
 *     CATCH_REQUIRE(e->run());
 * \endcode
 */
bool executor::run()
{
    try
    {
        ed::communicator::instance()->run();
        return true;
    }
    catch(std::exception const & ex)
    {
        std::cerr
            << "error: caught unexpected exception in e->run(): "
            << ex.what()
            << "\n";
    }
    catch(...)
    {
        std::cerr << "error: caught unknown exception in executor::run().\n";
    }

    return false;
}


/** \brief Stop the executor thread as soon as possible.
 *
 * The function asks for the runner to stop as soon as possible. It will
 * set the "continue" flag to false so the child stops quickly (i.e. once
 * the current instruction returns).
 *
 * Everything the child does is a quick process or includes a timeout
 * (like the wait() instruction). So it should happen pretty quickly.
 *
 * To instead wait for the thread to terminate normally, just wait for
 * the communicator::run() function to return.
 */
void executor::stop()
{
    f_thread->stop();
    ed::communicator::instance()->remove_connection(f_done_signal);
}


/** \brief Setup a callback to know when the thread is done.
 *
 * If the thread exits too soon (an expected or unexpected error occurred,)
 * you may not have the correct sequence of events to properly clean
 * the client connections. Setting up this function may help your test
 * to exit properly instead of being stuck waiting for more events
 * which would never occur.
 *
 * \param[in] callback  A callback pointer.
 */
void executor::set_thread_done_callback(thread_done_callback_t callback)
{
    executor_thread_done::pointer_t thread_done(dynamic_pointer_cast<executor_thread_done>(f_done_signal));
    if(thread_done == nullptr)
    {
        throw std::logic_error("f_done_signal is not an executor_thread_done object?"); // LCOV_EXCL_LINE
    }
    thread_done->set_thread_done_callback(callback);
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

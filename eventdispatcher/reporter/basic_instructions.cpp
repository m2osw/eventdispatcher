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
#include    "instruction_factory.h"
#include    "state.h"
#include    "variable_address.h"
#include    "variable_floating_point.h"
#include    "variable_integer.h"
#include    "variable_list.h"
#include    "variable_string.h"


// eventdispatcher
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/signal.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C
//
#include    <poll.h>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{


namespace
{



constexpr parameter_declaration const g_call_params[] = 
{
    {
        .f_name = "label",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_exit_params[] = 
{
    {
        .f_name = "error_message",
        .f_type = "string",
        .f_required = false,
    },
    {
        .f_name = "timeout",
        .f_type = "number",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_goto_params[] = 
{
    {
        .f_name = "label",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_has_message_params[] =
{
    {
        .f_name = "command",
        .f_type = "identifier",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_if_params[] = 
{
    {
        .f_name = "unordered",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "less",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "less_or_equal",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "greater",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "greater_or_equal",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "equal",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "false",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "not_equal",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "true",
        .f_type = "identifier",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_label_params[] =
{
    {
        .f_name = "name",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_listen_params[] =
{
    {
        .f_name = "address",
        .f_type = "address",
    },
    {}
};


constexpr parameter_declaration const g_send_message_params[] =
{
    {
        .f_name = "sent_server",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "sent_service",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "server",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "service",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "command",
        .f_type = "identifier",
    },
    {
        .f_name = "parameters",
        .f_type = "list",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_set_variable_params[] = 
{
    {
        .f_name = "name",
        .f_type = "identifier",
    },
    {
        .f_name = "value",
        .f_type = "any",
    },
    {}
};


constexpr parameter_declaration const g_sleep_params[] =
{
    {
        .f_name = "seconds",
        .f_type = "number",
    },
    {}
};


constexpr parameter_declaration const g_verify_message_params[] =
{
    {
        .f_name = "sent_server",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "sent_service",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "server",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "service",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "command",
        .f_type = "identifier",
    },
    {
        .f_name = "required_parameters",
        .f_type = "list",
        .f_required = false,
    },
    {
        .f_name = "optional_parameters",
        .f_type = "list",
        .f_required = false,
    },
    {
        .f_name = "forbidden_parameters",
        .f_type = "list",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_wait_params[] = 
{
    {
        .f_name = "timeout",
        .f_type = "number",
    },
    {
        .f_name = "mode",
        .f_type = "identifier",
        .f_required = false,
    },
    {}
};


} // no name namespace



// CALL
//
class inst_call
    : public instruction
{
public:
    inst_call()
        : instruction("call")
    {
    }

    virtual void func(state & s) override
    {
        s.push_ip();

        variable::pointer_t label_name(s.get_parameter("label", true));
        if(label_name == nullptr)
        {
            throw std::logic_error("label_name not available in call()."); // LCOV_EXCL_LINE
        }
        variable_string::pointer_t name(std::static_pointer_cast<variable_string>(label_name));
        if(name == nullptr)
        {
            throw std::logic_error("label_name -> name not available in call()."); // LCOV_EXCL_LINE
        }
        ip_t const ip(s.get_label_position(name->get_string()));
        s.set_ip(ip);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_call_params;
    }

private:
};
INSTRUCTION(call);


// CLEAR MESSAGE
//
class inst_clear_message
    : public instruction
{
public:
    inst_clear_message()
        : instruction("clear_message")
    {
    }

    virtual void func(state & s) override
    {
        s.clear_message();
    }

private:
};
INSTRUCTION(clear_message);


// DISCONNECT
//
class inst_disconnect
    : public instruction
{
public:
    inst_disconnect()
        : instruction("disconnect")
    {
    }

    virtual void func(state & s) override
    {
        s.disconnect();
    }

    // at some point we may support a "name: <identifier>" parameter...
    //virtual parameter_declaration const * parameter_declarations() const override
    //{
    //    return g_disconnect_params;
    //}
};
INSTRUCTION(disconnect);


// EXIT
//
class inst_exit
    : public instruction
{
public:
    inst_exit()
        : instruction("exit")
    {
    }

    virtual void func(state & s) override
    {
        s.set_exit_code(0);

        variable::pointer_t timeout(s.get_parameter("timeout"));
        variable::pointer_t error_message(s.get_parameter("error_message"));
        if(error_message != nullptr)
        {
            if(timeout != nullptr)
            {
                throw std::runtime_error("\"timeout\" and \"error_message\" from the exit() instruction are mutually exclusive.");
            }

            variable_string::pointer_t message(std::static_pointer_cast<variable_string>(error_message));

            std::cerr
                << "error: "
                << message->get_string()
                << std::endl;

            s.set_exit_code(1);
        }
        else if(timeout != nullptr)
        {
            // wait for timeout seconds, if a message is received before
            // the wait times out, it failed
            //
            snapdev::timespec_ex timeout_duration;
            variable_integer::pointer_t int_seconds(std::dynamic_pointer_cast<variable_integer>(timeout));
            if(int_seconds == nullptr)
            {
                variable_floating_point::pointer_t flt_seconds(std::dynamic_pointer_cast<variable_floating_point>(timeout));
                if(flt_seconds == nullptr)
                {
                    throw std::runtime_error("'timeout' parameter expected to be a number.");
                }
                timeout_duration.set(flt_seconds->get_floating_point());
            }
            else
            {
                timeout_duration.set(int_seconds->get_integer(), 0);
            }
            if(poll(s, timeout_duration) != 0)
            {
                s.set_exit_code(1);
            }
        }

        // jump to the very end so the executor knows it has to quit
        //
        s.set_ip(s.get_statement_size());
    }

    int poll(state & s, snapdev::timespec_ex timeout_duration)
    {
        std::vector<struct pollfd> fds;
        ed::connection::vector_t connections(s.get_connections());
        ed::connection::pointer_t listen(s.get_listen_connection());
        if(listen != nullptr)
        {
            connections.push_back(listen);
        }
        for(auto & c : connections)
        {
            int e(0);
            if(c->is_listener() || c->is_signal())
            {
                e |= POLLIN;
            }
            if(c->is_reader())
            {
                e |= POLLIN | POLLPRI | POLLRDHUP;
            }
            if(c->is_writer())
            {
                e |= POLLOUT | POLLRDHUP;
            }
            if(e == 0)
            {
                continue;
            }

            struct pollfd fd;
            fd.fd = c->get_socket();
            fd.events = e;
            fd.revents = 0;
            fds.push_back(fd);
        }
        if(fds.empty())
        {
            // no connection means we cannot receive invalid data before
            // exiting so all good here
            //
            return 0;
        }

        int const r(ppoll(&fds[0], fds.size(), &timeout_duration, nullptr));
        if(r < 0)
        {
            // TODO: enhance error message
            //
            int const e(errno);
            throw std::runtime_error(
                    "ppoll() returned an error: "
                  + std::to_string(e)
                  + ", "
                  + strerror(e));
        }
        for(auto & fd : fds)
        {
            if(fd.revents != 0)
            {
                return 1;
            }
        }

        // if no events happened, then we timed out which is good in this case
        //
        return 0;
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_exit_params;
    }

private:
};
INSTRUCTION(exit);


// GOTO
//
class inst_goto
    : public instruction
{
public:
    inst_goto()
        : instruction("goto")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t label_name(s.get_parameter("label", true));
        variable_string::pointer_t name(std::static_pointer_cast<variable_string>(label_name));
        ip_t const ip(s.get_label_position(name->get_string()));
        s.set_ip(ip);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_goto_params;
    }

private:
};
INSTRUCTION(goto);


// HAS MESSAGE
//
class inst_has_message
    : public instruction
{
public:
    inst_has_message()
        : instruction("has_message")
    {
    }

    virtual void func(state & s) override
    {
        ed::message const & msg(s.get_message());
        std::string const & command(msg.get_command());
        bool has_command(!command.empty());

        if(has_command)
        {
            variable::pointer_t command_name(s.get_parameter("command"));
            if(command_name != nullptr)
            {
                variable_string::pointer_t name(std::static_pointer_cast<variable_string>(command_name));
                has_command = command == name->get_string();
            }
        }
        s.set_compare(has_command
            ? compare_t::COMPARE_TRUE
            : compare_t::COMPARE_FALSE);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_has_message_params;
    }

private:
};
INSTRUCTION(has_message);


// IF
//
class inst_if
    : public instruction
{
public:
    inst_if()
        : instruction("if")
    {
    }

    virtual void func(state & s) override
    {
        // TODO: verify the potential overlaps
        //
        variable::pointer_t label_name;
        switch(s.get_compare())
        {
        case compare_t::COMPARE_UNDEFINED:
            // this cannot happen since we already throw in get_compare()
            //
            throw std::logic_error("got undefined compare in inst_if::func"); // LCOV_EXCL_LINE

        case compare_t::COMPARE_UNORDERED:
            label_name = s.get_parameter("unordered");
            break;

        case compare_t::COMPARE_LESS:
            label_name = s.get_parameter("less");
            if(label_name == nullptr)
            {
                label_name = s.get_parameter("less_or_equal");
                if(label_name == nullptr)
                {
                    label_name = s.get_parameter("not_equal");
                }
            }
            break;

        case compare_t::COMPARE_EQUAL:
            label_name = s.get_parameter("equal");
            if(label_name == nullptr)
            {
                label_name = s.get_parameter("less_or_equal");
                if(label_name == nullptr)
                {
                    label_name = s.get_parameter("greater_or_equal");
                    if(label_name == nullptr)
                    {
                        label_name = s.get_parameter("false");
                    }
                }
            }
            break;

        case compare_t::COMPARE_GREATER:
            label_name = s.get_parameter("greater");
            if(label_name == nullptr)
            {
                label_name = s.get_parameter("greater_or_equal");
                if(label_name == nullptr)
                {
                    label_name = s.get_parameter("not_equal");
                    if(label_name == nullptr)
                    {
                        label_name = s.get_parameter("true");
                    }
                }
            }
            break;

        }

        // if a matching label was found, act on it
        //
        if(label_name != nullptr)
        {
            variable_string::pointer_t name(std::static_pointer_cast<variable_string>(label_name));
            ip_t const ip(s.get_label_position(name->get_string()));
            s.set_ip(ip);
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_if_params;
    }
};
INSTRUCTION(if);


// LABEL
//
class inst_label
    : public instruction
{
public:
    inst_label()
        : instruction("label")
    {
    }

    virtual void func(state & s) override
    {
        snapdev::NOT_USED(s);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_label_params;
    }
};
INSTRUCTION(label);


// LISTEN
//
class inst_listen
    : public instruction
{
public:
    inst_listen()
        : instruction("listen")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t address(s.get_parameter("address", true));
        s.listen(std::static_pointer_cast<variable_address>(address)->get_address());
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_listen_params;
    }
};
INSTRUCTION(listen);


// RETURN
//
class inst_return
    : public instruction
{
public:
    inst_return()
        : instruction("return")
    {
    }

    virtual void func(state & s) override
    {
        s.pop_ip();
    }

private:
};
INSTRUCTION(return);


// RUN
//
class inst_run
    : public instruction
{
public:
    inst_run()
        : instruction("run")
    {
    }

    virtual void func(state & s) override
    {
        snapdev::NOT_USED(s);
        throw std::logic_error("run::func() was called when it should be intercepted by the executor.");
    }

private:
};
INSTRUCTION(run);


// SEND MESSAGE
//
class inst_send_message
    : public instruction
{
public:
    inst_send_message()
        : instruction("send_message")
    {
    }

    virtual void func(state & s) override
    {
        ed::connection::vector_t v(s.get_connections());
        if(v.empty())
        {
            throw std::runtime_error("send_message() has no connection to send a message to.");
        }
        // TODO: fix the connection selection, if we have more than one,
        //       how do we know which one to select?
        //
        ed::connection_with_send_message::pointer_t c(std::dynamic_pointer_cast<ed::connection_with_send_message>(v[0]));
        if(c == nullptr)
        {
            throw std::runtime_error("send_message() called without a valid listener connection.");
        }

        ed::message msg;

        variable::pointer_t param(s.get_parameter("sent_server"));
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            msg.set_sent_from_server(var->get_string());
        }

        param = s.get_parameter("sent_service");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            msg.set_sent_from_service(var->get_string());
        }

        param = s.get_parameter("server");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            msg.set_server(var->get_string());
        }

        param = s.get_parameter("service");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            msg.set_service(var->get_string());
        }

        param = s.get_parameter("command", true);
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        msg.set_command(var->get_string());

        param = s.get_parameter("parameters");
        if(param != nullptr)
        {
            //variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            //msg.add_parameter(var->get_string());
        }

        c->send_message(msg);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_send_message_params;
    }

private:
};
INSTRUCTION(send_message);


// SET VARIABLE
//
class inst_set_variable
    : public instruction
{
public:
    inst_set_variable()
        : instruction("set_variable")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t name(s.get_parameter("name", true));
        variable::pointer_t value(s.get_parameter("value", true));

        std::string var_name(std::static_pointer_cast<variable_string>(name)->get_string());
        variable::pointer_t var(value->clone(var_name));
        s.set_variable(var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_set_variable_params;
    }

private:
};
INSTRUCTION(set_variable);


// SHOW MESSAGE
//
class inst_show_message
    : public instruction
{
public:
    inst_show_message()
        : instruction("show_message")
    {
    }

    virtual void func(state & s) override
    {
        ed::message const & msg(s.get_message());
        std::cout
            << "--- message: "
            << msg
            << std::endl;
    }

private:
};
INSTRUCTION(show_message);


// SLEEP
//
class inst_sleep
    : public instruction
{
public:
    inst_sleep()
        : instruction("sleep")
    {
    }

    virtual void func(state & s) override
    {
        snapdev::timespec_ex pause_duration;
        variable::pointer_t seconds(s.get_parameter("seconds", true));
        variable_integer::pointer_t int_seconds(std::dynamic_pointer_cast<variable_integer>(seconds));
        if(int_seconds == nullptr)
        {
            variable_floating_point::pointer_t flt_seconds(std::dynamic_pointer_cast<variable_floating_point>(seconds));
            if(flt_seconds == nullptr)
            {
                throw std::runtime_error("'seconds' parameter expected to be a number.");
            }
            pause_duration.set(flt_seconds->get_floating_point());
        }
        else
        {
            pause_duration.set(int_seconds->get_integer(), 0);
        }
        if(nanosleep(&pause_duration, nullptr) != 0)
        {
            int const e(errno);
            std::cerr
                << "error: nanosleep() failed with "
                << strerror(e)
                << ".\n";
            throw std::runtime_error("nanosleep failed.");
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_sleep_params;
    }

private:
};
INSTRUCTION(sleep);


// VERIFY MESSAGE
//
class inst_verify_message
    : public instruction
{
public:
    inst_verify_message()
        : instruction("verify_message")
    {
    }

    virtual void func(state & s) override
    {
        ed::message const & msg(s.get_message());

        variable::pointer_t param(s.get_parameter("sent_server"));
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != msg.get_sent_from_server())
            {
                throw std::runtime_error(
                      "message expected sent from server name \""
                    + var->get_string()
                    + "\" did not match \""
                    + msg.get_server()
                    + "\".");
            }
        }

        param = s.get_parameter("sent_service");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != msg.get_sent_from_service())
            {
                throw std::runtime_error(
                      "message expected sent from service name \""
                    + var->get_string()
                    + "\" did not match \""
                    + msg.get_server()
                    + "\".");
            }
        }

        param = s.get_parameter("server");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != msg.get_server())
            {
                throw std::runtime_error(
                      "message expected server name \""
                    + var->get_string()
                    + "\" did not match \""
                    + msg.get_server()
                    + "\".");
            }
        }

        param = s.get_parameter("service");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != msg.get_service())
            {
                throw std::runtime_error(
                      "message expected service name \""
                    + var->get_string()
                    + "\" did not match \""
                    + msg.get_server()
                    + "\".");
            }
        }

        param = s.get_parameter("command");
        if(param != nullptr)
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != msg.get_command())
            {
                throw std::runtime_error(
                      "message expected command \""
                    + var->get_string()
                    + "\" did not match \""
                    + msg.get_server()
                    + "\".");
            }
        }

        check_parameters(s, msg, "required_parameters", false, false);
        check_parameters(s, msg, "optional_parameters", true, false);
        check_parameters(s, msg, "forbidden_parameters", false, true);
    }

    void check_parameters(
              state & s
            , ed::message const & msg
            , std::string const & list_name
            , bool optional
            , bool forbidden)
    {
        variable::pointer_t param(s.get_parameter(list_name));
        if(param != nullptr)
        {
            variable_list::pointer_t list(std::static_pointer_cast<variable_list>(param));
            std::size_t const max(list->get_item_size());
            for(std::size_t idx(0); idx < max; ++idx)
            {
                variable::pointer_t var(list->get_item(idx));
                std::string const & name(var->get_name());
                if(msg.has_parameter(name))
                {
                    if(forbidden)
                    {
                        throw std::runtime_error(
                              "message forbidden parameter \""
                            + name
                            + "\" was found in this message.");
                    }
                }
                else if(optional || forbidden)
                {
                    continue;
                }
                else // if(required)
                {
                    throw std::runtime_error(
                          "message required parameter \""
                        + name
                        + "\" was not found in this message.");
                }

                std::string const & type(var->get_type());
                if(type == "integer")
                {
                    std::int64_t const value(msg.get_integer_parameter(name));
                    variable_integer::pointer_t int_var(std::static_pointer_cast<variable_integer>(var));
                    if(int_var->get_integer() != value)
                    {
                        throw std::runtime_error(
                              "message expected parameter \""
                            + name
                            + "\" to be an integer set to \""
                            + std::to_string(value)
                            + "\" but found \""
                            + std::to_string(int_var->get_integer())
                            + "\" instead.");
                    }
                }
                else if(type == "string" || type == "identifier")
                {
                    std::string const value(msg.get_parameter(name));
                    variable_string::pointer_t str_var(std::static_pointer_cast<variable_string>(var));
                    if(str_var->get_string() != value)
                    {
                        throw std::runtime_error(
                              "message expected parameter \""
                            + name
                            + "\" to be a string set to \""
                            + value
                            + "\" but found \""
                            + str_var->get_string()
                            + "\" instead.");
                    }
                }
                else if(type == "void")
                {
                    // we already checked that the parameter exists
                    // we don't need to the the value since all values
                    // match "void"
                    ;
                }
                else
                {
                    throw std::runtime_error(
                          "message parameter type \""
                        + type
                        + "\" not supported yet.");
                }
            }
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_verify_message_params;
    }

private:
};
INSTRUCTION(verify_message);


// WAIT
//
class inst_wait
    : public instruction
{
public:
    enum class mode_t
    {
        MODE_WAIT,
        MODE_DRAIN,
    };

    inst_wait()
        : instruction("wait")
    {
    }

    virtual void func(state & s) override
    {
        snapdev::timespec_ex timeout_duration;
        variable::pointer_t timeout(s.get_parameter("timeout", true));
        variable_integer::pointer_t int_seconds(std::dynamic_pointer_cast<variable_integer>(timeout));
        if(int_seconds == nullptr)
        {
            variable_floating_point::pointer_t flt_seconds(std::dynamic_pointer_cast<variable_floating_point>(timeout));
            if(flt_seconds == nullptr)
            {
                throw std::runtime_error("'timeout' parameter expected to be a number.");
            }
            timeout_duration.set(flt_seconds->get_floating_point());
        }
        else
        {
            timeout_duration.set(int_seconds->get_integer(), 0);
        }

        mode_t mode(mode_t::MODE_WAIT);
        variable::pointer_t mode_param(s.get_parameter("mode"));
        if(mode_param != nullptr)
        {
            variable_string::pointer_t mode_name(std::static_pointer_cast<variable_string>(mode_param));
            if(mode_name == nullptr)
            {
                throw std::logic_error("mode_param -> mode_name not available in wait().");
            }
            std::string const & m(mode_name->get_string());
            if(m == "wait")
            {
                mode = mode_t::MODE_WAIT;
            }
            else if(m == "drain")
            {
                mode = mode_t::MODE_DRAIN;
            }
            else
            {
                throw std::runtime_error(
                      "unknown mode \""
                    + m
                    + "\" in wait().");
            }
        }

        for(;;)
        {
            int const r(poll(s, timeout_duration, mode));
            if(r == 0)
            {
                if(mode == mode_t::MODE_DRAIN)
                {
                    break;
                }
                throw std::runtime_error("no connections to wait() on.");
            }
            if(mode != mode_t::MODE_DRAIN)
            {
                break;
            }
        }
    }

    int poll(state & s, snapdev::timespec_ex timeout_duration, mode_t mode)
    {
        std::vector<struct pollfd> fds;
        std::map<ed::connection *, int> position;
        ed::connection::vector_t connections(s.get_connections());
        ed::connection::pointer_t listen(s.get_listen_connection());
        if(listen != nullptr)
        {
            connections.push_back(listen);
        }
        for(auto & c : connections)
        {
            int e(0);
            if(mode != mode_t::MODE_DRAIN)
            {
                if(c->is_listener() || c->is_signal())
                {
                    e |= POLLIN;
                }
                if(c->is_reader())
                {
                    e |= POLLIN | POLLPRI | POLLRDHUP;
                }
            }
            if(c->is_writer())
            {
                e |= POLLOUT | POLLRDHUP;
            }
            if(e == 0)
            {
                continue;
            }

            position[c.get()] = fds.size();
            struct pollfd fd;
            fd.fd = c->get_socket();
            fd.events = e;
            fd.revents = 0;
            fds.push_back(fd);
        }
        if(fds.empty())
        {
            // if draining, this means "DONE" otherwise it's an error
            //
            return 0;
        }

        int const r(ppoll(&fds[0], fds.size(), &timeout_duration, nullptr));
        if(r < 0)
        {
            // TODO: enhance error message
            //
            int const e(errno);
            throw std::runtime_error(
                    "ppoll() returned an error: "
                  + std::to_string(e)
                  + ", "
                  + strerror(e));
        }
        bool timed_out(true);
        for(auto & c : connections)
        {
            struct pollfd * fd(&fds[position[c.get()]]);
            if(fd->revents != 0)
            {
                timed_out = false;

                // an event happened on this one
                //
                if((fd->revents & (POLLIN | POLLPRI)) != 0)
                {
                    // we consider that Unix signals have the greater priority
                    // and thus handle them first
                    //
                    if(c->is_signal())
                    {
                        ed::signal * ss(dynamic_cast<ed::signal *>(c.get()));
                        if(ss != nullptr)
                        {
                            ss->process();
                        }
                    }
                    else if(c->is_listener())
                    {
                        // a listener is a special case and we want
                        // to call process_accept() instead
                        //
                        c->process_accept();
                    }
                    else
                    {
                        c->process_read();
                    }
                }
                if((fd->revents & POLLOUT) != 0)
                {
                    c->process_write();
                }
                if((fd->revents & POLLERR) != 0)
                {
                    c->process_error();
                }
                if((fd->revents & (POLLHUP | POLLRDHUP)) != 0)
                {
                    c->process_hup();
                }
                if((fd->revents & POLLNVAL) != 0)
                {
                    c->process_invalid();
                }
            }
        }
        if(timed_out)
        {
            // if we wake up without any event then we have a timeout
            //
            // TBD: we may need to call the process_timeout() on some
            //      connections? At this point I don't see why the
            //      server side would need such...
            //
            throw std::runtime_error("ppoll() timed out.");
        }

        return fds.size();
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_wait_params;
    }
};
INSTRUCTION(wait);


// `call()` -- DONE / TESTED
// `compare_message_command()`
// `exit()` -- PARTIAL / PARTIAL
// `goto()` -- DONE
// `message_has_parameter()`
// `message_has_parameter_with_value()`
// `if()` -- DONE
// `label()` -- DONE / TESTED
// `listen()`
// `return()` -- DONE / TESTED
// `run()` -- DONE / TESTED
// `save_parameter_value()`
// `send_message()`
// `set_variable()` -- DONE / TESTED
// `show_message()`
// `sleep()` -- DONE / TESTED
// `verify_message()` -- AVAILABLE, NOT IMPLEMENTED
// `wait()`


} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

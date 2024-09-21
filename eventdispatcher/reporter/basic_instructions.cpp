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
#include    "variable_regex.h"
#include    "variable_string.h"
#include    "variable_timestamp.h"



// eventdispatcher
//
#include    <eventdispatcher/connection_with_send_message.h>
#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/signal.h>
#include    <eventdispatcher/signal_handler.h>


// cppthread
//
#include    <cppthread/thread.h>


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// snapdev
//
#include    <snapdev/gethostname.h>
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/not_used.h>
#include    <snapdev/to_upper.h>


// C++
//
#include    <regex>


// C
//
#include    <poll.h>
#include    <signal.h>


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


constexpr parameter_declaration const g_compare_params[] =
{
    {
        .f_name = "expression",
        .f_type = "integer",
        .f_required = true,
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


constexpr parameter_declaration const g_has_type_params[] =
{
    {
        .f_name = "name",
        .f_type = "identifier",
    },
    {
        .f_name = "type",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_hex_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {
        .f_name = "value",
        .f_type = "integer",
    },
    {
        .f_name = "uppercase",
        .f_type = "integer",
        .f_required = false,
    },
    {
        .f_name = "width",
        .f_type = "integer",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_hostname_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_if_params[] =
{
    {
        .f_name = "variable",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "unordered",
        .f_type = "identifier",
        .f_required = false,
    },
    {
        .f_name = "ordered",
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


constexpr parameter_declaration const g_kill_params[] =
{
    {
        .f_name = "signal",
        .f_type = "any",
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


constexpr parameter_declaration const g_max_pid_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_now_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_print_params[] =
{
    {
        .f_name = "message",
        .f_type = "string",
    },
    {}
};


constexpr parameter_declaration const g_random_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {
        .f_name = "negative",
        .f_type = "integer",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_save_parameter_value_params[] =
{
    {
        .f_name = "parameter_name",
        .f_type = "identifier",
    },
    {
        .f_name = "variable_name",
        .f_type = "identifier",
    },
    {
        .f_name = "type",
        .f_type = "identifier",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_send_message_params[] =
{
    {
        .f_name = "sent_server",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "sent_service",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "server",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "service",
        .f_type = "string_or_identifier",
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
    {
        .f_name = "type",
        .f_type = "identifier",
        .f_required = false,
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


constexpr parameter_declaration const g_sort_params[] =
{
    {
        .f_name = "var1",
        .f_type = "string_or_identifier",
    },
    {
        .f_name = "var2",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var3",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var4",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var5",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var6",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var7",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var8",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var9",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var10",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var11",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var12",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var13",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var14",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var15",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var16",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var17",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var18",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var19",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {
        .f_name = "var20",
        .f_type = "string_or_identifier",
        .f_required = false,
    },
    {}
};


constexpr parameter_declaration const g_strlen_params[] =
{
    {
        .f_name = "variable_name",
        .f_type = "string_or_identifier",
    },
    {
        .f_name = "string",
        .f_type = "string",
    },
    {}
};


constexpr parameter_declaration const g_unset_variable_params[] =
{
    {
        .f_name = "name",
        .f_type = "identifier",
    },
    {}
};


constexpr parameter_declaration const g_verify_message_params[] =
{
    {
        .f_name = "sent_server",
        .f_type = "any",
        .f_required = false,
    },
    {
        .f_name = "sent_service",
        .f_type = "any",
        .f_required = false,
    },
    {
        .f_name = "server",
        .f_type = "any",
        .f_required = false,
    },
    {
        .f_name = "service",
        .f_type = "any",
        .f_required = false,
    },
    {
        .f_name = "command",
        .f_type = "any",
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
        variable_string::pointer_t name(std::static_pointer_cast<variable_string>(label_name));
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


// COMPARE
//
class inst_compare
    : public instruction
{
public:
    inst_compare()
        : instruction("compare")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t expr(s.get_parameter("expression", true));
        variable_integer::pointer_t integer(std::static_pointer_cast<variable_integer>(expr));
        int const value(integer->get_integer());

        if(value < -2 || value > 1)
        {
            throw ed::runtime_error(
                  s.get_location()
                + "unsupported integer in compare(), values are limited to -2 to 1.");
        }

        s.set_compare(static_cast<compare_t>(value));
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_compare_params;
    }

private:
};
INSTRUCTION(compare);


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
                throw ed::runtime_error(
                      s.get_location()
                    + "\"timeout\" and \"error_message\" from the exit() instruction are mutually exclusive.");
            }

            variable_string::pointer_t message(std::static_pointer_cast<variable_string>(error_message));

            // TODO: look at making the color optional
            //
            std::cerr
                << "\x1B[31m"
                << "error: "
                << message->get_string()
                << "\x1B[0m"
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
                timeout_duration.set(flt_seconds->get_floating_point());
            }
            else
            {
                timeout_duration.set(int_seconds->get_integer(), 0);
            }
            s.set_exit_code(poll(s, timeout_duration));
        }

        // jump to the very end so the executor knows it has to quit
        //
        s.set_ip(s.get_statement_size());
    }

    int poll(state & s, snapdev::timespec_ex timeout_duration)
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
                e |= POLLOUT | POLLRDHUP; // LCOV_EXCL_LINE
            }
            if(e == 0)
            {
                continue; // LCOV_EXCL_LINE
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
            // no connection means we cannot receive invalid data before
            // exiting so all good here
            //
            return 0;
        }

        int const r(ppoll(&fds[0], fds.size(), &timeout_duration, nullptr));
        if(r < 0)
        {
            // LCOV_EXCL_START
            int const e(errno);
            throw ed::runtime_error(
                    s.get_location()
                  + "ppoll() returned an error: "
                  + std::to_string(e)
                  + ", "
                  + strerror(e));
            // LCOV_EXCL_STOP
        }
        for(auto & c : connections)
        {
            struct pollfd const * fd(&fds[position[c.get()]]);
            if(fd->revents != 0)
            {
                if((fd->revents & (POLLHUP | POLLRDHUP)) != 0)
                {
                    // hang ups are expected, so process them naturally
                    //
                    c->process_hup();
                }
                else
                {
                    return 1;
                }
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
        ed::message const msg(s.get_message());
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


// HAS TYPE
//
class inst_has_type
    : public instruction
{
public:
    inst_has_type()
        : instruction("has_type")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t variable_name(s.get_parameter("name", true));
        variable_string::pointer_t name(std::static_pointer_cast<variable_string>(variable_name));
        variable::pointer_t var(s.get_variable(name->get_string()));

        if(var == nullptr)
        {
            s.set_compare(compare_t::COMPARE_UNORDERED);
        }
        else
        {
            variable::pointer_t variable_type(s.get_parameter("type", true));
            variable_string::pointer_t type(std::static_pointer_cast<variable_string>(variable_type));
            s.set_compare(var->get_type() == type->get_string()
                ? compare_t::COMPARE_TRUE
                : compare_t::COMPARE_FALSE);
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_has_type_params;
    }

private:
};
INSTRUCTION(has_type);


// HEX
//
class inst_hex
    : public instruction
{
public:
    inst_hex()
        : instruction("hex")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t var_name(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(var_name));
        std::string const & variable_name(var->get_string());

        variable::pointer_t i(s.get_parameter("value", true));
        variable_integer::pointer_t integer(std::dynamic_pointer_cast<variable_integer>(i));
        int const value(integer->get_integer());

        bool uppercase_flag(false);
        variable::pointer_t uppercase(s.get_parameter("uppercase"));
        if(uppercase != nullptr)
        {
            variable_integer::pointer_t uc(std::dynamic_pointer_cast<variable_integer>(uppercase));
            uppercase_flag = uc->get_integer() != 0;
        }

        int width_value(1);
        variable::pointer_t w(s.get_parameter("width"));
        variable_integer::pointer_t width(std::dynamic_pointer_cast<variable_integer>(w));
        if(width != nullptr)
        {
            width_value = width->get_integer();
        }

        variable_string::pointer_t new_var(std::make_shared<variable_string>(variable_name));
        new_var->set_string(snapdev::int_to_hex(value, uppercase_flag, width_value));
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_hex_params;
    }

private:
};
INSTRUCTION(hex);


// HOSTNAME
//
class inst_hostname
    : public instruction
{
public:
    inst_hostname()
        : instruction("hostname")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t param(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        std::string const & variable_name(var->get_string());

        variable_string::pointer_t new_var(std::make_shared<variable_string>(variable_name, "string"));
        new_var->set_string(snapdev::gethostname());
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_hostname_params;
    }

private:
};
INSTRUCTION(hostname);


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
        // TODO: verify potential overlaps (i.e. if the instruction has
        //       multiple labels and we could have the choice between
        //       two or more in variable situations)
        //
        variable::pointer_t label_name;
        compare_t compare(compare_t::COMPARE_UNDEFINED);
        variable::pointer_t const var_name(s.get_parameter("variable"));
        if(var_name != nullptr)
        {
            variable_string::pointer_t name(std::dynamic_pointer_cast<variable_string>(var_name));
            variable::pointer_t value(s.get_variable(name->get_string()));
            if(value != nullptr)
            {
                std::string const & type(value->get_type());
                if(type == "integer")
                {
                    variable_integer::pointer_t int_value(std::static_pointer_cast<variable_integer>(value));
                    int const v(int_value->get_integer());
                    if(v == 0)
                    {
                        compare = compare_t::COMPARE_EQUAL;
                    }
                    else if(v < 0)
                    {
                        compare = compare_t::COMPARE_LESS;
                    }
                    else
                    {
                        compare = compare_t::COMPARE_GREATER;
                    }
                }
                else if(type == "floating_point")
                {
                    variable_floating_point::pointer_t int_value(std::static_pointer_cast<variable_floating_point>(value));
                    double const v(int_value->get_floating_point());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(std::isnan(v))
                    {
                        compare = compare_t::COMPARE_UNORDERED;
                    }
                    else if(v == 0.0)
                    {
                        compare = compare_t::COMPARE_EQUAL;
                    }
                    else if(v < 0.0)
                    {
                        compare = compare_t::COMPARE_LESS;
                    }
                    else
                    {
                        compare = compare_t::COMPARE_GREATER;
                    }
#pragma GCC diagnostic pop
                }
                else
                {
                    throw ed::runtime_error("if(variable: ...) only supports variables of type integer or floating point.");
                }
            }
            else
            {
                compare = compare_t::COMPARE_UNORDERED;
            }
        }
        else
        {
            compare = s.get_compare();
        }
        switch(compare)
        {
        // LCOV_EXCL_START
        case compare_t::COMPARE_UNDEFINED:
            // this cannot happen since we already throw in get_compare()
            // and in case of a variable, we throw if we get an invalid type
            //
            throw ed::implementation_error("got undefined compare in inst_if::func"); // LCOV_EXCL_LINE
        // LCOV_EXCL_STOP

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
                    if(label_name == nullptr)
                    {
                        label_name = s.get_parameter("true");
                        if(label_name == nullptr)
                        {
                            label_name = s.get_parameter("ordered");
                        }
                    }
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
                        if(label_name == nullptr)
                        {
                            label_name = s.get_parameter("ordered");
                        }
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
                        if(label_name == nullptr)
                        {
                            label_name = s.get_parameter("ordered");
                        }
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


// KILL
//
class inst_kill
    : public instruction
{
public:
    inst_kill()
        : instruction("kill")
    {
    }

    virtual void func(state & s) override
    {
        int sig(SIGINT);
        variable::pointer_t const signal_name(s.get_parameter("signal"));
        if(signal_name != nullptr)
        {
            std::string const & type(signal_name->get_type());
            if(type == "integer")
            {
                sig = std::static_pointer_cast<variable_integer>(signal_name)->get_integer();
            }
            else if(type == "string" || type == "identifier")
            {
                std::string const name(std::dynamic_pointer_cast<variable_string>(signal_name)->get_string());
                sig = ed::signal_handler::get_signal_number(snapdev::to_upper(name));
            }
            else
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "kill(signal: ...) unsupported parameter type.");
            }
            if(sig < SIGHUP || sig >= NSIG)
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "kill(signal: ...) unknown signal.");
            }
        }

        // send the signal to the main thread
        //
        if(pthread_kill(s.get_server_thread_id(), sig) != 0)
        {
            // LCOV_EXCL_START
            int const e(errno);
            throw ed::runtime_error(
                  s.get_location()
                + "kill(): signal could not be sent (errno: "
                + std::to_string(e)
                + ", "
                + strerror(e)
                + ").");
            // LCOV_EXCL_STOP
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_kill_params;
    }
};
INSTRUCTION(kill);


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


// MAX_PID
//
class inst_max_pid
    : public instruction
{
public:
    inst_max_pid()
        : instruction("max_pid")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t param(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        std::string const & variable_name(var->get_string());

        variable_integer::pointer_t new_var(std::make_shared<variable_integer>(variable_name));
        new_var->set_integer(cppthread::get_pid_max());
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_max_pid_params;
    }

private:
};
INSTRUCTION(max_pid);


// NOW
//
class inst_now
    : public instruction
{
public:
    inst_now()
        : instruction("now")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t param(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        std::string const & variable_name(var->get_string());

        variable_timestamp::pointer_t new_var(std::make_shared<variable_timestamp>(variable_name));
        new_var->set_timestamp(snapdev::now());
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_now_params;
    }

private:
};
INSTRUCTION(now);


// PRINT
//
class inst_print
    : public instruction
{
public:
    inst_print()
        : instruction("print")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t msg(s.get_parameter("message", true));
        std::cout
            << "--- message: "
            << std::static_pointer_cast<variable_string>(msg)->get_string()
            << std::endl;
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_print_params;
    }
};
INSTRUCTION(print);


// RANDOM
//
class inst_random
    : public instruction
{
public:
    inst_random()
        : instruction("random")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t param(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        std::string const & variable_name(var->get_string());

        bool negative(true);
        param = s.get_parameter("negative", false);
        if(param != nullptr)
        {
            variable_integer::pointer_t var_int = std::static_pointer_cast<variable_integer>(param);
            std::int64_t const & boolean(var_int->get_integer());
            negative = boolean != 0;
        }

        variable_integer::pointer_t new_var(std::make_shared<variable_integer>(variable_name));
        std::int64_t result(
               (static_cast<std::int64_t>(rand()) << 48)
             ^ (static_cast<std::int64_t>(rand()) << 32)
             ^ (static_cast<std::int64_t>(rand()) << 16)
             ^ (static_cast<std::int64_t>(rand()) <<  0));
        if(!negative)
        {
            // remove the sign bit
            //
            result &= 0x7FFFFFFFFFFFFFFFLL;
        }
        new_var->set_integer(result);
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_random_params;
    }

private:
};
INSTRUCTION(random);


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
        throw ed::implementation_error("run::func() was called when it should be intercepted by the executor.");
    }

private:
};
INSTRUCTION(run);


// SAVE PARAMETER VALUE
//
class inst_save_parameter_value
    : public instruction
{
public:
    inst_save_parameter_value()
        : instruction("save_parameter_value")
    {
    }

    virtual void func(state & s) override
    {
        ed::message const msg(s.get_message());

        variable::pointer_t param(s.get_parameter("parameter_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
        std::string const & parameter_name(var->get_string());
        std::string value;
        if(msg.has_parameter(parameter_name))
        {
            value = msg.get_parameter(parameter_name);
        }
        else if(parameter_name == "sent_server")
        {
            value = msg.get_sent_from_server();
        }
        else if(parameter_name == "sent_service")
        {
            value = msg.get_sent_from_service();
        }
        else if(parameter_name == "server")
        {
            value = msg.get_server();
        }
        else if(parameter_name == "service")
        {
            value = msg.get_service();
        }
        else if(parameter_name == "command")
        {
            value = msg.get_command();
        }

        param = s.get_parameter("variable_name", true);
        var = std::static_pointer_cast<variable_string>(param);
        std::string const & variable_name(var->get_string());

        std::string type("string");
        param = s.get_parameter("type");
        if(param != nullptr)
        {
            var = std::static_pointer_cast<variable_string>(param);
            type = var->get_string();
        }
        if(type == "string"
        || type == "identifier")
        {
            variable_string::pointer_t new_var(std::make_shared<variable_string>(variable_name, type));
            new_var->set_string(value);
            s.set_variable(new_var);
        }
        else if(type == "integer")
        {
            variable_integer::pointer_t new_var(std::make_shared<variable_integer>(variable_name));
            std::int64_t int_value(0);
            if(!value.empty()
            && !advgetopt::validator_integer::convert_string(value, int_value))
            {
                throw ed::runtime_error(
                      "value \""
                    + value
                    + "\" not recognized as a valid integer.");
            }
            new_var->set_integer(int_value);
            s.set_variable(new_var);
        }
        else if(type == "timestamp")
        {
            variable_timestamp::pointer_t new_var(std::make_shared<variable_timestamp>(variable_name));
            if(!value.empty())
            {
                new_var->set_timestamp(value);
            }
            s.set_variable(new_var);
        }
        else
        {
            throw ed::runtime_error(
                  "unsupported type \""
                + type
                + "\" for save_parameter_value().");
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_save_parameter_value_params;
    }

private:
};
INSTRUCTION(save_parameter_value);


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
            throw ed::runtime_error("send_message() has no connection to send a message to.");
        }
        // TODO: fix the connection selection, if we have more than one,
        //       how do we know which one to select? (i.e. have a connection
        //       name included in the parameters)
        //
        ed::connection_with_send_message::pointer_t c(std::dynamic_pointer_cast<ed::connection_with_send_message>(v[0]));
        if(c == nullptr)
        {
            throw ed::runtime_error("send_message() called without a valid listener connection."); // LCOV_EXCL_LINE
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
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            msg.set_command(var->get_string());
        }
//std::cerr << "--- send message [" << msg.get_command() << "]\n";

        param = s.get_parameter("parameters");
        if(param != nullptr)
        {
            variable_list::pointer_t list(std::static_pointer_cast<variable_list>(param));
            std::size_t const max(list->get_item_size());
            for(std::size_t idx(0); idx < max; ++idx)
            {
                variable::pointer_t var(list->get_item(idx));
                std::string const & name(var->get_name());
                std::string const & type(var->get_type());
                if(type == "integer")
                {
                    variable_integer::pointer_t int_var(std::static_pointer_cast<variable_integer>(var));
                    msg.add_parameter(name, int_var->get_integer());
                }
                else if(type == "string" || type == "identifier")
                {
                    variable_string::pointer_t str_var(std::static_pointer_cast<variable_string>(var));
                    msg.add_parameter(name, str_var->get_string());
                }
                else if(type == "timestamp")
                {
                    variable_timestamp::pointer_t ts_var(std::static_pointer_cast<variable_timestamp>(var));
                    msg.add_parameter(name, ts_var->get_timestamp());
                }
                else
                {
                    throw ed::runtime_error(
                          "message parameter type \""
                        + type
                        + "\" not supported yet.");
                }
            }
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

        std::string cast;
        variable::pointer_t type(s.get_parameter("type"));
        if(type != nullptr)
        {
            cast = std::static_pointer_cast<variable_string>(type)->get_string();
        }

        std::string const var_name(std::static_pointer_cast<variable_string>(name)->get_string());
        variable::pointer_t var(value->clone(var_name));
        if(!cast.empty())
        {
            bool converted(true);
            std::string const & var_type(var->get_type());
            if(var_type == "string")
            {
                variable_string::pointer_t var_string(std::static_pointer_cast<variable_string>(var));
                if(cast == "string")
                {
                    ; // do nothing
                }
                else if(cast == "timestamp") // string -> timestamp
                {
                    // expect the variable value to represent a valid
                    // floating point value
                    //
                    double timestamp(0.0);
                    if(!advgetopt::validator_double::convert_string(var_string->get_string(), timestamp))
                    {
                        throw ed::runtime_error(
                              "invalid timestamp, a valid floating point was expected ("
                            + var_string->get_string()
                            + ").");
                    }
                    variable_timestamp::pointer_t timestamp_var(std::make_shared<variable_timestamp>(var_name));
                    timestamp_var->set_timestamp(timestamp);
                    var = timestamp_var;
                }
                else
                {
                    converted = false;
                }
            }
            else if(var_type == "timestamp")
            {
                if(cast == "timestamp")
                {
                    ; // do nothing
                }
                else
                {
                    converted = false;
                }
            }
            else
            {
                converted = false;
            }
            if(!converted)
            {
                throw ed::runtime_error(
                      "casting from \""
                    + var_type
                    + "\" to \""
                    + cast
                    + "\" is not yet implemented.");
            }
        }
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
        ed::message const msg(s.get_message());
        std::cout
            << "--- script message: "
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
            pause_duration.set(flt_seconds->get_floating_point());
        }
        else
        {
            pause_duration.set(int_seconds->get_integer(), 0);
        }
        if(nanosleep(&pause_duration, nullptr) != 0)
        {
            // LCOV_EXCL_START
            int const e(errno);
            std::cerr
                << "error: nanosleep() failed with "
                << strerror(e)
                << ".\n";
            throw ed::runtime_error("nanosleep failed.");
            // LCOV_EXCL_STOP
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_sleep_params;
    }

private:
};
INSTRUCTION(sleep);


// SORT
//
class inst_sort
    : public instruction
{
public:
    inst_sort()
        : instruction("sort")
    {
    }

    virtual void func(state & s) override
    {
        // check for variable names (var1: name1, var2: name2, ...)
        //
        variable::vector_t array;
        std::string result_type;
        for(int i(1);; ++i)
        {
            std::string var_number("var");
            var_number += std::to_string(i);
            variable::pointer_t param(s.get_parameter(var_number, false));
            if(param == nullptr)
            {
                break;
            }
            variable_string::pointer_t var_string(std::static_pointer_cast<variable_string>(param));
            variable::pointer_t var(s.get_variable(var_string->get_string()));
            if(var == nullptr)
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "variable named \""
                    + var_string->get_string()
                    + "\" not found.");
            }
            std::string const & type(var->get_type());
            if(result_type.empty())
            {
                if(type != "string"
                && type != "integer"
                && type != "floating_point")
                {
                    throw ed::runtime_error(
                          s.get_location()
                        + "sort only supports strings, integers, or floating points.");
                }
                result_type = type;
            }
            else if(type != result_type)
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "sort only supports one type of data (\""
                    + result_type
                    + "\" in this case) for all the specified variables. \""
                    + type
                    + "\" is not compatible.");
            }
            array.push_back(var);
        }

        if(result_type == "string")
        {
            std::map<std::string, bool> values;
            std::for_each(
                  array.begin()
                , array.end()
                , [&values](variable::pointer_t a)
                {
                    variable_string::pointer_t sa(std::dynamic_pointer_cast<variable_string>(a));
                    values[sa->get_string()] = true;
                });
            auto it(values.begin());
            std::for_each(
                  array.begin()
                , array.end()
                , [&values, &it](variable::pointer_t a)
                {
                    variable_string::pointer_t sa(std::dynamic_pointer_cast<variable_string>(a));
                    sa->set_string(it->first);
                    ++it;
                });
        }
        else if(result_type == "integer")
        {
            std::map<std::int64_t, bool> values;
            std::for_each(
                  array.begin()
                , array.end()
                , [&values](variable::pointer_t a)
                {
                    variable_integer::pointer_t sa(std::dynamic_pointer_cast<variable_integer>(a));
                    values[sa->get_integer()] = true;
                });
            auto it(values.begin());
            std::for_each(
                  array.begin()
                , array.end()
                , [&values, &it](variable::pointer_t a)
                {
                    variable_integer::pointer_t sa(std::dynamic_pointer_cast<variable_integer>(a));
                    sa->set_integer(it->first);
                    ++it;
                });
        }
        else if(result_type == "floating_point")
        {
            std::map<double, bool> values;
            std::for_each(
                  array.begin()
                , array.end()
                , [&values](variable::pointer_t a)
                {
                    variable_floating_point::pointer_t sa(std::dynamic_pointer_cast<variable_floating_point>(a));
                    values[sa->get_floating_point()] = true;
                });
            auto it(values.begin());
            std::for_each(
                  array.begin()
                , array.end()
                , [&values, &it](variable::pointer_t a)
                {
                    variable_floating_point::pointer_t sa(std::dynamic_pointer_cast<variable_floating_point>(a));
                    sa->set_floating_point(it->first);
                    ++it;
                });
        }
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_sort_params;
    }

private:
};
INSTRUCTION(sort);


// STRLEN
//
class inst_strlen
    : public instruction
{
public:
    inst_strlen()
        : instruction("strlen")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t st(s.get_parameter("string", true));
        variable_string::pointer_t str(std::dynamic_pointer_cast<variable_string>(st));

        variable::pointer_t var_name(s.get_parameter("variable_name", true));
        variable_string::pointer_t var(std::static_pointer_cast<variable_string>(var_name));
        std::string const & variable_name(var->get_string());

        variable_integer::pointer_t new_var(std::make_shared<variable_integer>(variable_name));
        new_var->set_integer(str->get_string().length());
        s.set_variable(new_var);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_strlen_params;
    }

private:
};
INSTRUCTION(strlen);


// UNSET VARIABLE
//
class inst_unset_variable
    : public instruction
{
public:
    inst_unset_variable()
        : instruction("unset_variable")
    {
    }

    virtual void func(state & s) override
    {
        variable::pointer_t name(s.get_parameter("name", true));
        std::string const var_name(std::static_pointer_cast<variable_string>(name)->get_string());
        s.unset_variable(var_name);
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_unset_variable_params;
    }

private:
};
INSTRUCTION(unset_variable);


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
        ed::message const msg(s.get_message());

        check_value(s, "sent_server", msg.get_sent_from_server());
        check_value(s, "sent_service", msg.get_sent_from_service());
        check_value(s, "server", msg.get_server());
        check_value(s, "service", msg.get_service());
        check_value(s, "command", msg.get_command());

        check_parameters(s, msg, "required_parameters", false, false);
        check_parameters(s, msg, "optional_parameters", true, false);
        check_parameters(s, msg, "forbidden_parameters", false, true);
    }

    void check_value(
              state & s
            , std::string const name
            , std::string const & value)
    {
        variable::pointer_t param(s.get_parameter(name));
        if(param == nullptr)
        {
            return;
        }

        std::string const & type(param->get_type());
        if(type == "string" || type == "identifier")
        {
            variable_string::pointer_t var(std::static_pointer_cast<variable_string>(param));
            if(var->get_string() != value)
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "message expected \""
                    + name
                    + "\", set to \""
                    + value
                    + "\", to match \""
                    + var->get_string()
                    + "\".");
            }
        }
        else if(type == "regex")
        {
            variable_regex::pointer_t regex_var(std::static_pointer_cast<variable_regex>(param));
            std::regex const compiled_regex(regex_var->get_regex());
            if(!std::regex_match(value, compiled_regex))
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "message expected \""
                    + name
                    + "\", set to \""
                    + value
                    + "\", to match regex \""
                    + regex_var->get_regex()
                    + "\".");
            }
        }
        else
        {
            throw ed::runtime_error(
                  s.get_location()
                + "message value \""
                + name
                + "\" does not support type \""
                + type
                + "\".");
        }
    }

    void check_parameters(
              state & s
            , ed::message const & msg
            , std::string const & list_name
            , bool optional
            , bool forbidden)
    {
        variable::pointer_t param(s.get_parameter(list_name));
        if(param == nullptr)
        {
            return;
        }

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
                    throw ed::runtime_error(
                          s.get_location()
                        + "message forbidden parameter \""
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
                throw ed::runtime_error(
                      s.get_location()
                    + "message required parameter \""
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
                    throw ed::runtime_error(
                          s.get_location()
                        + "message expected parameter \""
                        + name
                        + "\" to be an integer set to \""
                        + std::to_string(int_var->get_integer())
                        + "\" but found \""
                        + std::to_string(value)
                        + "\" instead.");
                }
            }
            else if(type == "string" || type == "identifier")
            {
                std::string value(msg.get_parameter(name));
                variable_string::pointer_t str_var(std::static_pointer_cast<variable_string>(var));
                if(str_var->get_string() != value)
                {
                    // if the strings are really long, remove everything
                    // that's considered equal so we can better see
                    // what is not and quickly act on it
                    //
                    std::string expected(str_var->get_string());
                    if(expected.length() > 100
                    || value.length() > 100)
                    {
                        bool erased(false);
                        while(!expected.empty()
                           && !value.empty()
                           && expected[0] == value[0])
                        {
                            expected.erase(0, 1);
                            value.erase(0, 1);
                            erased = true;
                        }
                        if(erased)
                        {
                            expected = "..." + expected;
                            value = "..." + value;
                        }
                    }
                    throw ed::runtime_error(
                          s.get_location()
                        + "message expected parameter \""
                        + name
                        + "\" to be a string set to \""
                        + expected
                        + "\" but found \""
                        + value
                        + "\" instead.");
                }
            }
            else if(type == "regex")
            {
                std::string const value(msg.get_parameter(name));
                variable_regex::pointer_t regex_var(std::static_pointer_cast<variable_regex>(var));
                std::regex const compiled_regex(regex_var->get_regex());
                if(!std::regex_match(value, compiled_regex))
                {
                    throw ed::runtime_error(
                          s.get_location()
                        + "message expected parameter \""
                        + name
                        + "\", set to \""
                        + value
                        + "\", to match regex \""
                        + regex_var->get_regex()
                        + "\".");
                }
            }
            else if(type == "timestamp")
            {
                snapdev::timespec_ex const value(msg.get_timespec_parameter(name));
                variable_timestamp::pointer_t timestamp_var(std::static_pointer_cast<variable_timestamp>(var));
                if(timestamp_var->get_timestamp() != value)
                {
                    throw ed::runtime_error(
                          s.get_location()
                        + "message expected parameter \""
                        + name
                        + "\", set to \""
                        + value.to_string()
                        + "\", to match timestamp \""
                        + timestamp_var->get_timestamp().to_string()
                        + "\".");
                }
            }
            else if(type == "void")
            {
                // we already checked that the parameter exists
                // we don't need to check the value since all values
                // match "void"
                ;
            }
            else
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "message parameter type \""
                    + type
                    + "\" not supported yet.");
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
        MODE_WAIT,      // cannot timeout and it must have connections (default)
        MODE_DRAIN,     // empty list of connections expected
        MODE_TIMEOUT,   // timeout expected
    };

    inst_wait()
        : instruction("wait")
    {
    }

    virtual void func(state & s) override
    {
        if(!s.get_in_thread())
        {
            throw ed::runtime_error("wait() used before run().");
        }

        snapdev::timespec_ex timeout_duration;
        variable::pointer_t timeout(s.get_parameter("timeout", true));
        variable_integer::pointer_t int_seconds(std::dynamic_pointer_cast<variable_integer>(timeout));
        if(int_seconds == nullptr)
        {
            variable_floating_point::pointer_t flt_seconds(std::dynamic_pointer_cast<variable_floating_point>(timeout));
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
            std::string const & m(mode_name->get_string());
            if(m == "wait")
            {
                mode = mode_t::MODE_WAIT;
            }
            else if(m == "drain")
            {
                mode = mode_t::MODE_DRAIN;
            }
            else if(m == "timeout")
            {
                mode = mode_t::MODE_TIMEOUT;
            }
            else
            {
                throw ed::runtime_error(
                      s.get_location()
                    + "unknown mode \""
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
                throw ed::runtime_error("no connections to wait() on.");
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

        for(;;)
        {
            int const r(ppoll(&fds[0], fds.size(), &timeout_duration, nullptr));
            if(r < 0)
            {
                // LCOV_EXCL_START
                int const e(errno);
                if(e == EINTR)
                {
                    std::cerr
                        << "error: got an interrupt while ppoll() in reporter. Trying again.\n";
                    continue;
                }
                throw ed::runtime_error(
                        "ppoll() returned an error: "
                      + std::to_string(e)
                      + ", "
                      + strerror(e));
                // LCOV_EXCL_STOP
            }
            break;
        } // LCOV_EXCL_LINE
        bool timed_out(true);
        for(auto & c : connections)
        {
            struct pollfd const * fd(&fds[position[c.get()]]);
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
                        // LCOV_EXCL_START
                        ed::signal * ss(dynamic_cast<ed::signal *>(c.get()));
                        if(ss != nullptr)
                        {
                            ss->process();
                        }
                        // LCOV_EXCL_STOP
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
                    c->process_error(); // LCOV_EXCL_LINE
                }
                if((fd->revents & (POLLHUP | POLLRDHUP)) != 0)
                {
                    c->process_hup();
                }
                if((fd->revents & POLLNVAL) != 0)
                {
                    c->process_invalid(); // LCOV_EXCL_LINE
                }
            }
        }
        if(timed_out && mode != mode_t::MODE_TIMEOUT)
        {
            // if we wake up without any event then we have a timeout
            //
            // TBD: we may need to call the process_timeout() on some
            //      connections? At this point I don't see why the
            //      server side would need such...
            //
            throw ed::runtime_error("ppoll() timed out.");
        }

        return fds.size();
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_wait_params;
    }
};
INSTRUCTION(wait);



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

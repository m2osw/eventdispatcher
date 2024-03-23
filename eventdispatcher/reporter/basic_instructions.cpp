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
#include    "variable_floating_point.h"
#include    "variable_integer.h"
#include    "variable_string.h"


// snapdev
//
#include    <snapdev/not_used.h>


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
        .f_name = "not_equal",
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
        .f_type = "identifer",
        .f_required = false,
    },
    {
        .f_name = "sent_service",
        .f_type = "identifer",
        .f_required = false,
    },
    {
        .f_name = "server",
        .f_type = "identifer",
        .f_required = false,
    },
    {
        .f_name = "service",
        .f_type = "identifer",
        .f_required = false,
    },
    {
        .f_name = "command",
        .f_type = "identifer",
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
            throw std::logic_error("label_name not available in call().");
        }
        variable_string::pointer_t name(std::static_pointer_cast<variable_string>(label_name));
        if(name == nullptr)
        {
            throw std::logic_error("label_name -> name not available in call().");
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
            // wait until timeout, if the timeout does not happen, we failed
            //
throw std::logic_error("exit(timeout: ...) not yet implemented");
        }

        // jump to the very end so the executor knows it has to quit
        //
        s.set_ip(s.get_statement_size());
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
            if(var->get_string() != msg.get_server())
            {
                throw std::runtime_error("message expected server name mismatch.");
            }
        }
        // TODO: test all params
    }

    virtual parameter_declaration const * parameter_declarations() const override
    {
        return g_verify_message_params;
    }

private:
};
INSTRUCTION(verify_message);


// `call()` -- DONE
// `compare_message_command()`
// `exit()` -- PARTIAL
// `goto()` -- DONE
// `message_has_parameter()`
// `message_has_parameter_with_value()`
// `if()` -- DONE
// `label()` -- DONE
// `listen()`
// `return()` -- DONE
// `run()` -- DONE
// `save_parameter_value()`
// `send_message()`
// `set_variable()`
// `sleep()` -- DONE
// `verify_message()` -- AVAILABLE, NOT IMPLEMENTED
// `wait()`


} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

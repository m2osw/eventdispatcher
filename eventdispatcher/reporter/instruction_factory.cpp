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


instruction::map_t      g_instructions = instruction::map_t();


} // no name namespace


instruction_factory::instruction_factory(instruction::pointer_t i)
    : f_instruction(i)
{
    g_instructions[get_name()] = f_instruction;
}


instruction_factory::~instruction_factory()
{
}


std::string const & instruction_factory::get_name() const
{
    return f_instruction->get_name();
}


/** \brief Get the named instruction.
 *
 * This function searches for the named instruction and return a pointer
 * to it. If the instruction does not exist, then a null pointer is returned.
 *
 * \note
 * This function does not generate any error. It is best for the parser
 * to do so since that way it can give us the filename and line number
 * with the location of the unknown instruction.
 *
 * \param[in] name  The name of the instruction to search.
 *
 * \return A pointer to the instruction or nullptr.
 */
instruction::pointer_t get_instruction(std::string const & name)
{
    auto it(g_instructions.find(name));
    if(it == g_instructions.end())
    {
        return nullptr;
    }

    return it->second;
}


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
};
INSTRUCTION(label);


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
        s.set_ip(f_ip);
    }

private:
    ip_t            f_ip = 0;
};
INSTRUCTION(goto);



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

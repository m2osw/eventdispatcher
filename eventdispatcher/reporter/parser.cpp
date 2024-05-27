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
#include    "parser.h"

#include    <eventdispatcher/reporter/instruction_factory.h>


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



parser::parser(lexer::pointer_t l, state::pointer_t s)
    : f_lexer(l)
    , f_state(s)
{
}


/** \brief Parse the input program.
 *
 * This function parses the input program as defined in the lexer passed
 * to the constructor. The grammar goes as follow:
 *
 * \code
 * start:
 *    statements
 *
 * statements:
 *    one_statement
 *    statements one_statement
 *
 * one_statement:
 *    IDENTIFIER '(' parameters ')'
 *
 * parameters:
 *    one_parameter
 *    parameters ',' one_parameter
 *
 * one_parameter:
 *    IDENTIFIER ':' expression
 *
 * expression:
 *    comparative
 *  | '{' expression_list '}'
 *
 * expression_list:
 *    list_item
 *  | list_item ',' expression_list
 *
 * list_item:
 *    IDENTIFIER
 *  | IDENTIFIER ':' comparative
 *
 * comparative:
 *    additive
 *    comparative '<=>' additive
 *
 * additive:
 *    multiplicative
 *    additive '+' multiplicative
 *    additive '-' multiplicative
 *
 * multiplicative:
 *    primary
 *    multiplicative '*' primary
 *    multiplicative '/' primary
 *    multiplicative '%' primary
 *
 * primary:
 *    IDENTIFIER
 *  | FLOATING_POINT
 *  | INTEGER
 *  | ADDRESS
 *  | TIMESPEC
 *  | VARIABLE
 *  | DOUBLE_STRING
 *  | SINGLE_STRING
 *  | REGEX
 *  | '(' comparative ')'
 *  | '+' comparative
 *  | '-' comparative
 * \endcode
 *
 * \todo
 * Update the grammar to match the actual implementation.
 *
 * \return A vector of statements.
 */
void parser::parse_program()
{
    while(!next_token())
    {
        one_statement();
    }
}


bool parser::next_token()
{
    f_token = f_lexer->next_token();
    if(f_token.get_token() == token_t::TOKEN_ERROR)
    {
        throw std::runtime_error("invalid token.");
    }
    return f_token.get_token() == token_t::TOKEN_EOF;
}


void parser::one_statement()
{
    if(f_token.get_token() != token_t::TOKEN_IDENTIFIER)
    {
        f_lexer->error(f_token, "a statement is expected to start with the name of an instruction (a.k.a. an identifier).");
        throw std::runtime_error("expected identifier.");
    }

    std::string const inst_name(f_token.get_string());
    instruction::pointer_t inst(get_instruction(inst_name));
    if(inst == nullptr)
    {
        f_lexer->error(f_token, "unknown instruction \"" + inst_name + "\".");
        throw std::runtime_error("unknown instruction.");
    }

    f_statement = std::make_shared<statement>(inst);

    if(next_token())
    {
        f_lexer->error(
              f_token
            , "an instruction (\""
            + inst_name
            + "\" here) must include parenthesis, end of file found.");
        throw std::runtime_error("expected '(' parenthesis instead of EOF.");
    }
    if(f_token.get_token() != token_t::TOKEN_OPEN_PARENTHESIS)
    {
        f_lexer->error(f_token, "an instruction name must be followed by '('.");
        throw std::runtime_error("expected '(' parenthesis.");
    }
    if(next_token())
    {
        f_lexer->error(f_token, "an instruction must end with a closing parenthesis, end of file found.");
        throw std::runtime_error("expected ')' parenthesis instead of EOF.");
    }
    if(f_token.get_token() != token_t::TOKEN_CLOSE_PARENTHESIS)
    {
        parameters();
    }
    if(f_token.get_token() != token_t::TOKEN_CLOSE_PARENTHESIS)
    {
        f_lexer->error(f_token, "an instruction parameter list must end with a closing parenthesis.");
        throw std::runtime_error("expected ')' parenthesis to end parameter list.");
    }

    f_statement->verify_parameters();

    f_state->add_statement(f_statement);
    f_statement.reset();
}


void parser::parameters()
{
    for(;;)
    {
        one_parameter();
        if(f_token.get_token() != token_t::TOKEN_COMMA)
        {
            break;
        }
        next_token();
    }
}


void parser::one_parameter()
{
    if(f_token.get_token() != token_t::TOKEN_IDENTIFIER)
    {
        f_lexer->error(f_token, "an instruction parameter must be named using an identifier.");
        throw std::runtime_error("expected identifier to name parameter.");
    }
    std::string const name(f_token.get_string());
    if(next_token())
    {
        f_lexer->error(f_token, "expected ':' after parameter name, not EOF.");
        throw std::runtime_error("expected ':' after parameter name, not EOF.");
    }
    if(f_token.get_token() != token_t::TOKEN_COLON)
    {
        f_lexer->error(f_token, "an instruction parameter must be followed by a ':'.");
        throw std::runtime_error("expected ':' after parameter name.");
    }
    if(next_token())
    {
        f_lexer->error(f_token, "an instruction parameter must be followed by ':' and then an expression; expression missing.");
        throw std::runtime_error("expected expression.");
    }

    expression::pointer_t expr;
    if(f_token.get_token() == token_t::TOKEN_OPEN_CURLY_BRACE)
    {
        next_token();
        expr = expression_list();
    }
    else
    {
        expr = comparative();
    }

    f_statement->add_parameter(name, expr);
}


expression::pointer_t parser::expression_list()
{
    expression::pointer_t list(std::make_shared<expression>());
    list->set_operator(operator_t::OPERATOR_LIST);

    // empty list?
    //
    if(f_token.get_token() == token_t::TOKEN_CLOSE_CURLY_BRACE)
    {
        next_token();
        return list;
    }

    for(;;)
    {
        list->add_expression(list_item());

        if(f_token.get_token() != token_t::TOKEN_COMMA)
        {
            if(f_token.get_token() != token_t::TOKEN_CLOSE_CURLY_BRACE)
            {
                f_lexer->error(f_token, "a list of parameter values must end with '}'.");
                throw std::runtime_error("a list of parameter values must end with '}'.");
            }
            next_token();
            return list;
        }
        if(next_token())
        {
            f_lexer->error(f_token, "end of file found before end of list ('}' missing).");
            throw std::runtime_error("end of file found before end of list ('}' missing).");
        }
    }
}


expression::pointer_t parser::list_item()
{
    if(f_token.get_token() != token_t::TOKEN_IDENTIFIER)
    {
        f_lexer->error(f_token, "a list item must be named using an identifier.");
        throw std::runtime_error("a list item must be named using an identifier.");
    }
    token const name(f_token);
    //std::string const name(f_token.get_string());
    //variable::pointer_t name(std::make_shared<variable>());

    if(next_token())
    {
        f_lexer->error(f_token, "a list must end with a '}'.");
        throw std::runtime_error("a list must end with a '}'.");
    }
    expression::pointer_t item(std::make_shared<expression>());
    item->set_operator(operator_t::OPERATOR_NAMED);

    expression::pointer_t identifier(std::make_shared<expression>());
    identifier->set_operator(operator_t::OPERATOR_PRIMARY);
    identifier->set_token(name);
    item->add_expression(identifier);

    if(f_token.get_token() == token_t::TOKEN_COLON)
    {
        if(next_token())
        {
            f_lexer->error(f_token, "a list item with a colon (:) must be followed by an expression.");
            throw std::runtime_error("a list item with a colon (:) must be followed by an expression.");
        }
        item->add_expression(comparative());
    }

    return item;
}


expression::pointer_t parser::comparative()
{
    operator_t op(operator_t::OPERATOR_NULL);
    expression::pointer_t left(additive());
    for(;;)
    {
        if(f_token.get_token() == token_t::TOKEN_COMPARE)
        {
            op = operator_t::OPERATOR_COMPARE;
        }
        else
        {
            return left;
        }

        next_token();
        expression::pointer_t right(additive());
        expression::pointer_t expr(std::make_shared<expression>());
        expr->set_operator(op);
        expr->add_expression(left);
        expr->add_expression(right);
        left = expr;
    }

    snapdev::NOT_REACHED();
}


expression::pointer_t parser::additive()
{
    operator_t op(operator_t::OPERATOR_NULL);
    expression::pointer_t left(multiplicative());
    for(;;)
    {
        if(f_token.get_token() == token_t::TOKEN_PLUS)
        {
            op = operator_t::OPERATOR_ADD;
        }
        else if(f_token.get_token() == token_t::TOKEN_MINUS)
        {
            op = operator_t::OPERATOR_SUBTRACT;
        }
        else
        {
            return left;
        }

        next_token();
        expression::pointer_t right(multiplicative());
        expression::pointer_t expr(std::make_shared<expression>());
        expr->set_operator(op);
        expr->add_expression(left);
        expr->add_expression(right);
        left = expr;
    }

    snapdev::NOT_REACHED();
}   // LCOV_EXCL_LINE


expression::pointer_t parser::multiplicative()
{
    operator_t op(operator_t::OPERATOR_NULL);
    expression::pointer_t left(primary());
    for(;;)
    {
        if(f_token.get_token() == token_t::TOKEN_MULTIPLY)
        {
            op = operator_t::OPERATOR_MULTIPLY;
        }
        else if(f_token.get_token() == token_t::TOKEN_DIVIDE)
        {
            op = operator_t::OPERATOR_DIVIDE;
        }
        else if(f_token.get_token() == token_t::TOKEN_MODULO)
        {
            op = operator_t::OPERATOR_MODULO;
        }
        else
        {
            return left;
        }

        next_token();
        expression::pointer_t right(primary());
        expression::pointer_t expr(std::make_shared<expression>());
        expr->set_operator(op);
        expr->add_expression(left);
        expr->add_expression(right);
        left = expr;
    }

    snapdev::NOT_REACHED();
}   // LCOV_EXCL_LINE


expression::pointer_t parser::primary()
{
    switch(f_token.get_token())
    {
    case token_t::TOKEN_IDENTIFIER:
    case token_t::TOKEN_FLOATING_POINT:
    case token_t::TOKEN_INTEGER:
    case token_t::TOKEN_ADDRESS:
    case token_t::TOKEN_TIMESPEC:
    case token_t::TOKEN_VARIABLE:
    case token_t::TOKEN_DOUBLE_STRING:
    case token_t::TOKEN_SINGLE_STRING:
    case token_t::TOKEN_REGEX:
        {
            expression::pointer_t expr(std::make_shared<expression>());
            expr->set_operator(operator_t::OPERATOR_PRIMARY);
            expr->set_token(f_token);
            next_token();
            return expr;
        }
        break;

    case token_t::TOKEN_PLUS:
    case token_t::TOKEN_MINUS:
        {
            expression::pointer_t expr(std::make_shared<expression>());
            expr->set_operator(f_token.get_token() == token_t::TOKEN_PLUS
                                        ? operator_t::OPERATOR_IDENTITY
                                        : operator_t::OPERATOR_NEGATE);
            next_token();
            expr->add_expression(primary());
            return expr;
        }
        break;

    case token_t::TOKEN_OPEN_PARENTHESIS:
        {
            if(next_token())
            {
                f_lexer->error(f_token, "an expression between parenthesis must include at least one primary expression.");
                throw std::runtime_error("an expression between parenthesis must include at least one primary expression.");
            }
            expression::pointer_t expr(comparative());
            if(f_token.get_token() != token_t::TOKEN_CLOSE_PARENTHESIS)
            {
                f_lexer->error(f_token, "an expression between parenthesis must include the ')' at the end.");
                throw std::runtime_error("an expression between parenthesis must include the ')' at the end.");
            }
            next_token();
            return expr;
        }
        break;

    default:
        f_lexer->error(f_token, "expected a primary token for expression.");
        throw std::runtime_error("expected a primary token for expression.");

    }
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

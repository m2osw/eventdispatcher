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
#include    "lexer.h"



// snapdev
//
#include    <snapdev/file_contents.h>
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/not_reached.h>
#include    <snapdev/timespec_ex.h>


// advgetopt
//
#include    <advgetopt/validator_double.h>
#include    <advgetopt/validator_integer.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



lexer::lexer(
          std::string const & filename
        , std::string const & program)
    : f_filename(filename)
    , f_program(program)
    , f_iterator(f_program)
{
}


token lexer::next_token()
{
    for(;;)
    {
        token t;
        t.set_line(f_line);
        t.set_column(f_column);
        char32_t c(getc());
        switch(c)
        {
        case libutf8::EOS:
            t.set_token(token_t::TOKEN_EOF);
            return t;

        case U'\n':
        case U' ':
        case U'\t':
        case U'\f':
            // TODO: add all Unicode white spaces
            break;

        case U'(':
        case U')':
        case U'{':
        case U'}':
        case U',':
        case U':':
        //case U'=': -- not currently used
        case U'+':
        case U'-':
        case U'*':
        case U'%':
            t.set_token(static_cast<token_t>(c));
            return t;

        case U'/':
            c = getc();
            if(c == U'/')
            {
                for(;;)
                {
                    c = getc();
                    if(c == U'\n' || c == libutf8::EOS)
                    {
                        break;
                    }
                }
            }
            else
            {
                ungetc(c);
                t.set_token(token_t::TOKEN_DIVIDE);
                return t;
            }
            break;

        case U'$':
            {
                std::string s;
                c = getc();
                if(c == '{')
                {
                    for(;;)
                    {
                        c = getc();
                        if(c == '}')
                        {
                            break;
                        }
                        if((c < 'A' || c > 'Z')
                        && (c < 'a' || c > 'z')
                        && (c < '0' || c > '9')
                        && c != '_')
                        {
                            error(t, "unexpected character to close variable; expected '}'.");
                            return t;
                        }
                        s += libutf8::to_u8string(c);
                    }
                }
                else
                {
                    while((c >= 'A' && c <= 'Z')
                       || (c >= 'a' && c <= 'z')
                       || (c >= '0' && c <= '9')
                       || c == '_')
                    {
                        s += libutf8::to_u8string(c);
                        c = getc();
                    }
                }
                if(s.empty())
                {
                    error(t, "unexpected '$' without a variable name.");
                    return t;
                }

                // unless we accept special characters, the smallest is '0'
                // anyway...
                //
                if(/*s[0] >= '0' &&*/ s[0] <= '9')
                {
                    error(t, "variable name cannot start with a digit.");
                    return t;
                }

                t.set_token(token_t::TOKEN_VARIABLE);
                t.set_string(s); // variable name
                return t;
            }
            snapdev::NOT_REACHED();

        case U'@':
            {
                std::string s;
                c = getc();
                if(c == U'\'' || c == U'"')
                {
                    // timestamp is a date and time including spaces
                    //
                    char32_t const quote(c);
                    for(;;)
                    {
                        c = getc();
                        if(c == libutf8::EOS)
                        {
                            error(t, "unterminated date.");
                            return t;
                        }
                        if(c == quote)
                        {
                            break;
                        }
                        s += libutf8::to_u8string(c);
                    }
                    snapdev::timespec_ex timestamp;
                    timestamp.from_string(s, "%m/%d/%Y %T"); // fixed US date, no tv_nsec support
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    t.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                else
                {
                    // timestamp is a double (where the period and decimal
                    // parts are optional)
                    //
                    s += libutf8::to_u8string(c);
                    for(;;)
                    {
                        c = getc();
                        if((c < '0' || c > '9')
                        && c != '.'
                        && c != 's')
                        {
                            ungetc(c);
                            break;
                        }
                        s += libutf8::to_u8string(c);
                    }
                    snapdev::timespec_ex timestamp(s);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
                    t.set_integer((static_cast<__int128>(timestamp.tv_sec) << 64) | timestamp.tv_nsec);
#pragma GCC diagnostic pop
                }
                if(s.empty())
                {
                    error(t, "unexpected '@' without a timestamp.");
                    return t;
                }

                t.set_token(token_t::TOKEN_TIMESPEC);
                t.set_string(s);
                return t;
            }
            snapdev::NOT_REACHED();

        case U'<':
            {
                std::string s;
                for(;;)
                {
                    c = getc();
                    if(c == libutf8::EOS)
                    {
                        error(t, "unterminated IP address.");
                        return t;
                    }
                    if(c == U'>')
                    {
                        break;
                    }
                    s += libutf8::to_u8string(c);
                }
                if(s.empty())
                {
                    error(t, "an empty IP address is not a valid address.");
                    return t;
                }
                if(s == "=")
                {
                    // this is the <=> operator
                    //
                    t.set_token(token_t::TOKEN_COMPARE);
                }
                else
                {
                    addr::addr_parser p;
                    p.set_protocol("tcp");
                    p.set_allow(addr::allow_t::ALLOW_MASK, true);
                    addr::addr_range::vector_t addresses(p.parse(s));
                    if(p.has_errors())
                    {
                        error(t, "error parsing IP address " + s + ".");
                        return t;
                    }
                    if(addresses[0].is_range()
                    || !addresses[0].has_from())
                    {
                        // the parsing is expected to fail against ranges
                        // so we should not get here
                        //
                        throw std::logic_error("parsing of address did not fail with a range."); // LCOV_EXCL_LINE
                    }

                    t.set_token(token_t::TOKEN_ADDRESS);
                    t.set_string(addresses[0].get_from().to_ipv4or6_string(
                              addr::STRING_IP_BRACKET_ADDRESS
                            | addr::STRING_IP_PORT
                            | addr::STRING_IP_MASK_IF_NEEDED));
                }
                return t;
            }
            snapdev::NOT_REACHED();

        case U'"':
        case U'\'':
        case U'`':
            {
                std::string s;
                char32_t const quote(c);
                for(;;)
                {
                    c = getc();
                    if(c == libutf8::EOS)
                    {
                        error(t, "unterminated string.");
                        return t;
                    }
                    if(c == '\n')
                    {
                        error(t, "strings cannot be written on multiple lines.");
                        return t;
                    }
                    if(c == quote)
                    {
                        break;
                    }
                    if(c == '\\')
                    {
                        c = getc();
                        if(c == libutf8::EOS)
                        {
                            error(t, "unterminated backslash sequence in string.");
                            return t;
                        }
                        switch(c)
                        {
                        case '\\':
                        case '\'':
                        case '"':
                        case '`':
                            // as is characters
                            break;

                        case U'a':
                            c = U'\a';
                            break;

                        case U'b':
                            c = U'\b';
                            break;

                        case U'f':
                            c = U'\f';
                            break;

                        case U'n':
                            c = U'\n';
                            break;

                        case U'r':
                            c = U'\r';
                            break;

                        case U't':
                            c = U'\t';
                            break;

                        case U'v':
                            c = U'\v';
                            break;

                        // TODO: add support for \x, \u, \U, \[0-9]{1,3}
                        case 'x':
                        case 'u':
                        case 'U':
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            throw libexcept::fixme("sorry, the \\... with a number to define a character are not yet supported.");
                            break;

                        default:
                            throw std::runtime_error(
                                  "invalid escape character '"
                                + libutf8::to_u8string(c)
                                + "'");

                        }
                    }
                    s += libutf8::to_u8string(c);
                }
                t.set_token(static_cast<token_t>(quote));
                t.set_string(s);
                return t;
            }
            snapdev::NOT_REACHED();

        case U'0':
        case U'1':
        case U'2':
        case U'3':
        case U'4':
        case U'5':
        case U'6':
        case U'7':
        case U'8':
        case U'9':
        case U'.':
            {
                if(c == U'0')
                {
                    c = getc();
                    if(c == 'x' || c == 'X')
                    {
                        std::int64_t value(0);
                        for(;;)
                        {
                            c = getc();
                            if(!snapdev::is_hexdigit(c))
                            {
                                break;
                            }
                            value <<= 4;
                            value |= snapdev::hexdigit_to_number(c);
                        }
                        ungetc(c);
                        t.set_token(token_t::TOKEN_INTEGER);
                        t.set_integer(value);
                        return t;
                    }
                    ungetc(c);
                    c = U'0';
                }
                bool is_floating_point(false);
                std::string s;
                while((c >= U'0' && c <= U'9')
                   || c == '+'
                   || c == '-'
                   || c == '.'
                   || c == 'e'
                   || c == 'E')
                {
                    if(c == '.'
                    || c == 'e'
                    || c == 'E')
                    {
                        is_floating_point = true;
                    }
                    s += libutf8::to_u8string(c);
                    c = getc();
                }
                ungetc(c);
                if(is_floating_point)
                {
                    // our language accepts ".3" as "0.3" but the advgetopt
                    // validator does not, so make sure to add the "0" if
                    // missing
                    //
                    if(s[0] == '.')
                    {
                        s = '0' + s;
                    }
                    double value(0.0);
                    if(!advgetopt::validator_double::convert_string(s, value))
                    {
                        error(t, "invalid floating point (" + s + ").");
                        return t;
                    }
                    t.set_token(token_t::TOKEN_FLOATING_POINT);
                    t.set_floating_point(value);
                }
                else
                {
                    std::int64_t value(0);
                    if(!advgetopt::validator_integer::convert_string(s, value))
                    {
                        error(t, "invalid integer (" + s + ").");
                        return t;
                    }
                    t.set_token(token_t::TOKEN_INTEGER);
                    t.set_integer(value);
                }
                return t;
            }
            snapdev::NOT_REACHED();


        default:
            if((c >= U'A' && c <= U'Z')
            || (c >= U'a' && c <= U'z')
            || c == U'_')
            {
                std::string s;
                for(;;)
                {
                    s += libutf8::to_u8string(c);
                    c = getc();
                    if((c < U'A' || c > U'Z')
                    && (c < U'a' || c > U'z')
                    && (c < U'0' || c > U'9')
                    && c != U'_')
                    {
                        break;
                    }
                }
                ungetc(c);
                if(s == "NaN")
                {
                    t.set_token(token_t::TOKEN_FLOATING_POINT);
                    t.set_floating_point(std::numeric_limits<double>::quiet_NaN());
                }
                else
                {
                    t.set_token(token_t::TOKEN_IDENTIFIER);
                    t.set_string(s);
                }
                return t;
            }

            error(t, "unexpected character (" + libutf8::to_u8string(c) + ").");
            return t;

        }
    }
    snapdev::NOT_REACHED();
}


void lexer::error(token & t, std::string const & msg)
{
    std::cerr
        << "error:"
        << f_filename
        << ':'
        << t.get_line()
        << ':'
        << t.get_column()
        << ": "
        << msg
        << "\n";

    t.set_token(token_t::TOKEN_ERROR);
}


char32_t lexer::getc()
{
    if(f_unget_pos > 0)
    {
        --f_unget_pos;
        return f_unget[f_unget_pos];
    }

    char32_t c(*f_iterator++);
    if(c == '\r')
    {
        c = *f_iterator;
        if(c == '\n')
        {
            ++f_iterator;
        }
        else
        {
            c = '\n';
        }
    }

    if(c == '\n')
    {
        ++f_line;
        f_column = 1;
    }
    else
    {
        ++f_column;
    }

    return c;
}


void lexer::ungetc(char32_t c)
{
    // no need to unget the end of string marker
    //
    if(c == libutf8::EOS)
    {
        return;
    }

    if(f_unget_pos >= sizeof(f_unget))
    {
        throw std::logic_error("too many ungetc() calls."); // LCOV_EXCL_LINE
    }

    f_unget[f_unget_pos] = c;
    ++f_unget_pos;
}


/** \brief Create a lexer object from a file.
 *
 * This function tries to open the specified \p filename as is and with
 * an additional suffix, .rprtr. If either file exists, it gets loaded
 * in memory and passed to a lexer object as the program input. Then
 * the lexer pointer is returned.
 *
 * If no file can be loaded, then the function returns a null pointer.
 *
 * \param[in] filename  The name of the file to load.
 *
 * \return A lexer object or nullptr if no program could be loaded.
 */
lexer::pointer_t create_lexer(std::string const & filename)
{
    std::shared_ptr<snapdev::file_contents> input(std::make_shared<snapdev::file_contents>(filename));
    if(!input->exists())
    {
        input = std::make_shared<snapdev::file_contents>(filename + ".rprtr");
        if(!input->exists())
        {
            return lexer::pointer_t();
        }
    }

    if(!input->read_all())
    {
        return lexer::pointer_t(); // LCOV_EXCL_LINE
    }

    return std::make_shared<lexer>(filename, input->contents());
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et

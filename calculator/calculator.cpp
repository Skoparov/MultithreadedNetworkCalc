#include "calculator.h"

namespace calc
{

namespace detail
{

static constexpr char char_plus{ '+' };
static constexpr char char_minus{ '-' };
static constexpr char char_mult{ '*' };
static constexpr char char_div{ '/' };
static constexpr char char_opening_bracket{ '(' };
static constexpr char char_closing_bracket{ ')' };
static constexpr char char_expr_end{ '\n' };

int get_precedence( const operator_type& type ) noexcept
{
    int result{ -1 };
    switch( type )
    {
    case operator_type::subexpr_start:
    case operator_type::subexpr_first_num: result = -1; break;
    case operator_type::end: result = 0; break;
    case operator_type::addition: result = 1; break;
    case operator_type::substraction: result = 1; break;
    case operator_type::multiplication: result = 2; break;
    case operator_type::division: result = 2; break;
    default: assert( false );
    }

    return result;
}

operator_type get_oper_type( char c )
{
    operator_type type{ operator_type::subexpr_start };

    switch ( c )
    {
    case char_plus: type = operator_type::addition; break;
    case char_minus: type = operator_type::substraction; break;
    case char_mult: type = operator_type::multiplication; break;
    case char_div: type = operator_type::division; break;
    case char_opening_bracket: type = operator_type::subexpr_start; break;
    default: throw std::invalid_argument{ "Invalid operator" };
    }

    return type;
}

entry_type get_entry_type( char c )
{
    entry_type type;

    if( c == char_plus || c == char_minus ||
        c == char_mult || c == char_div )
    {
        type = entry_type::math;
    }
    else if( c == char_opening_bracket )
    {
        type = entry_type::opening_bracket;
    }
    else if( c == char_closing_bracket )
    {
        type = entry_type::closing_bracket;
    }
    else if( c >= '0' && c <= '9' )
    {
        type = entry_type::number;
    }
    else if( c == char_expr_end )
    {
        type = entry_type::expr_end;
    }
    else
    {
        throw std::invalid_argument{ "Invalid character" };
    }

    return type;
}

}// detail

}// calc


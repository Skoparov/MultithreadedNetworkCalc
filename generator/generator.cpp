#include "generator.h"

#include <fstream>
#include <tuple>
#include <random>
#include <stack>
#include <cassert>
#include <type_traits>

template< typename type >
type get_random_int( type min, type max )
{
    static_assert( std::is_integral< type >::value, "Type should be integral" );

    static std::random_device rd;
    static std::mt19937 mt{ rd() };
    std::uniform_int_distribution< type > dist{ min, max };

    return dist( mt );
}

template< typename type >
const type& select( const type& first, const type& second )
{
    return get_random_int( 0, 1 )? first : second;
}

//

enum class action_type{ put_number, open_bracket, close_bracket, math };
enum class math_action{ add, substract, multiply, divide };

math_action get_random_math_action()
{
    using type = std::underlying_type< math_action >::type;
    type rand { get_random_int< type >( static_cast< type >( math_action::add ),
                                        static_cast< type >( math_action::divide ) ) };

    return static_cast< math_action >( rand );
}

action_type math_or_close( bool can_close )
{
    action_type type{ action_type::math };
    if( can_close && get_random_int( 0, 1 ) )
    {
        type = action_type::close_bracket;
    }

    return type;
}

action_type math_or_number()
{
    return select( action_type::math, action_type::put_number );
}

action_type number_or_open()
{
    return select( action_type::open_bracket, action_type::put_number );
}

action_type get_next_action_type( const action_type& prev, bool can_close )
{
    action_type next_action_type;

    switch( prev )
    {
    case action_type::open_bracket: next_action_type = action_type::put_number; break;
    case action_type::close_bracket:
    case action_type::put_number: next_action_type = math_or_close( can_close ); break;
    case action_type::math: next_action_type = number_or_open(); break;
    default: throw std::invalid_argument{ "Unimplemented action type" };
    }

    return next_action_type;
}

std::string rand_int_as_str()
{
    int result{ 0 };
    do
    {
        result = get_random_int( 1, std::numeric_limits< int >::max() );
    }
    while( result == 0 ); // to avoid division by zero cases

    return std::to_string( result );
}

std::string rand_math_as_str()
{
    std::string result;

    math_action action{ get_random_math_action() };
    switch( action )
    {
    case math_action::add: result = "+"; break;
    case math_action::substract: result = "-"; break;
    case math_action::multiply: result = "*"; break;
    case math_action::divide: result = "/"; break;
    default: throw std::invalid_argument{ "Unimplemented math_action type" };
    }

    return result;
}

std::string action_type_to_str( const action_type& type )
{
    std::string result;

    switch( type )
    {
    case action_type::open_bracket: result = "("; break;
    case action_type::close_bracket: result = ")"; break;
    case action_type::put_number: result = rand_int_as_str(); break;
    case action_type::math: result = rand_math_as_str(); break;
    default: throw std::invalid_argument{ "Unimplemented action type" };
    }

    return result;
}

void update_bracket_data( const action_type& next_action,
                          uint64_t& brackets_to_close,
                          std::stack< bool >& math_within_brackets )
{
    if( next_action == action_type::open_bracket )
    {
        ++brackets_to_close;
        math_within_brackets.push( false );
    }
    else if( next_action == action_type::close_bracket )
    {
        assert( brackets_to_close && !math_within_brackets.empty() );
        --brackets_to_close;
        math_within_brackets.pop();
    }
}

void generate_expression(const std::string& dest_file, uint64_t approx_max_size )
{
    if( approx_max_size < 3 )
    {
        throw std::invalid_argument{ "Max size should be greater or equal to 3" };
    }

    std::ofstream file{ dest_file };
    if( !file.is_open() )
    {
        throw std::ios_base::failure{ "Failed to open file" };
    }

    uint64_t size{ 0 };
    uint64_t brackets_to_close{ 0 };
    std::stack< bool > has_math_within_brackets;
    action_type prev_action_type;
    action_type next_action_type{ select( action_type::put_number, action_type::open_bracket ) };

    while( size < ( approx_max_size - brackets_to_close ) )
    {
        update_bracket_data( next_action_type, brackets_to_close, has_math_within_brackets );
        if( next_action_type == action_type::math && !has_math_within_brackets.empty() )
        {
            has_math_within_brackets.top() = true;
        }

        std::string action_str{ action_type_to_str( next_action_type ) };
        file << action_str;
        if( action_str == "/" )
        {
            file << 1; // to avoid divisions by zero
            next_action_type = action_type::put_number;
        }

        size += action_str.length();
        prev_action_type = next_action_type;

        bool can_close_bracket{ brackets_to_close && has_math_within_brackets.top() };
        next_action_type = get_next_action_type( prev_action_type, can_close_bracket );
    }

    if( prev_action_type == action_type::math )
    {
        file << action_type_to_str( action_type::put_number );
    }

    for( uint64_t i{ 0 }; i < brackets_to_close; ++i )
    {
        file << ")";
    }

    file << "\n";
}

void repeat_expression_for_size( const std::string& dest_file, const std::string& source_file, uint64_t approx_size )
{
    std::ifstream in{ source_file };
    if( !in.is_open() )
    {
        throw std::ios_base::failure{ "Failed to open source file" };
    }

    std::ofstream out{ dest_file };
    if( !out.is_open() )
    {
        throw std::ios_base::failure{ "Failed to open dest file" };
    }

    std::string expression{ ( std::istreambuf_iterator< char >{ in } ),
                            std::istreambuf_iterator< char >() };

    in.close();

    if( expression.empty() )
    {
        throw std::logic_error{ "Source file is empty" };
    }

    size_t num_to_repeat{ approx_size / ( expression.length() + 1 ) };
    if( num_to_repeat )
    {
        for( size_t i{ 0 }; i < num_to_repeat; ++i )
        {
            out << expression;
            if( i != num_to_repeat - 1 )
            {
                out << (i % 2)? "+" : "-";
            }
        }

        out << "\n";
    }
}

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stack>
#include <queue>
#include <future>

#include <boost/lexical_cast.hpp>

namespace calc
{

namespace detail
{

enum class entry_type{ number, math, opening_bracket, closing_bracket, expr_end };

// subexpr_start used to indicate the start of a subexpression
// subexpr_first_num indicates that at least one number of the subexpr has been but on stack
// the latter used to distinguish between negative values following '(' and minus operators
// to know when it's possible to unwind the stack
enum class operator_type{ subexpr_start, subexpr_first_num, end, addition, substraction, multiplication, division, };

int get_precedence( const operator_type& type ) noexcept;
operator_type get_oper_type( char c );
entry_type get_entry_type( char c );

}// detail

class calculation_aborted : public std::exception{};

template < typename type >
class async_calculator
{
public:
    // Start new calculation
    std::future< type > start( const std::string& expr_beginning )
    {
        return start_impl( expr_beginning );
    }

    std::future< type > start( std::string&& expr_beginning )
    {
        return start_impl( std::move( expr_beginning ) );
    }

    // Add more data to the current calculation
    void add_expr_part( const std::string& expr_part )
    {
       add_expr_part_impl( expr_part );
    }

    void add_expr_part( std::string&& expr_part )
    {
       add_expr_part_impl( std::move( expr_part ) );
    }

    void abort()
    {
        std::lock_guard< std::mutex > l{ m_mutex };
        if( m_running )
        {
            m_running = false;
            m_cv.notify_one();
        }
    }

    void reset()
    {
        std::lock_guard< std::mutex > l{ m_mutex };
        if( m_running )
        {
            throw std::logic_error{ "Calculation is running" };
        }

        clean_all();
    }

    bool running() const noexcept{ return m_running; }
    bool finished() const noexcept{ return m_calculation_finished; }
    bool error_occured() const noexcept{ return m_error_occured; }

private:
    template< typename string_type >
    std::future< type > start_impl( string_type&& expr_beginning )
    {
        if( expr_beginning.empty() )
        {
            throw std::invalid_argument{ "Empty expression" };
        }

        std::lock_guard< std::mutex > l{ m_mutex };
        if( !m_running )
        {
            clean_all();

            m_running = true;

            m_expression_parts.push( std::forward< string_type >( expr_beginning ) );
            return std::async( std::launch::async,
                               &async_calculator< type >::calculate, this );
        }
        else
        {
            throw std::logic_error{ "Another calculation is in progress" };
        }
    }

    template< typename string_type >
    void add_expr_part_impl( string_type&& expr_part )
    {
        if( expr_part.empty() )
        {
            throw std::invalid_argument{ "Empty expression" };
        }

        std::lock_guard< std::mutex > l{ m_mutex };
        if( m_running )
        {
            m_expression_parts.push( std::forward< string_type >( expr_part ) );
            m_cv.notify_one();
        }
        else
        {
            throw std::logic_error{ "Calculation is not running" };
        }
    }

    void clean_parse_data()
    {
        while( !m_operator_stack.empty() )
        {
            m_operator_stack.pop();
        }

        while( !m_numbers.empty() )
        {
            m_numbers.pop();
        }

        while( !m_expression_parts.empty() )
        {
            m_expression_parts.pop();
        }
    }

    void clean_all()
    {
        clean_parse_data();

        m_read_pos = 0;
        m_running = false;
        m_error_occured = false;
        m_calculation_finished = false;
    }

    type calc_math( type& first, type& second,
                    const detail::operator_type& oper_type ) const
    {
        using namespace detail;

        if( oper_type == detail::operator_type::division && second == type{ 0 } )
        {
            throw std::logic_error{ "Division by zero" };
        }

        type result{ std::move( first ) };

        switch( oper_type )
        {
        case operator_type::addition: result += second; break;
        case operator_type::substraction: result -= second; break;
        case operator_type::multiplication: result *= second; break;
        case operator_type::division: result /= second; break;
        default: throw std::invalid_argument{ "Unimplemented math operator" }; break;
        }

        return result;
    }

    char get_character()
    {
        // remove ws and find first valid character or wait for more data
        std::unique_lock< std::mutex > l { m_mutex };

        while( true )
        {
            if( m_expression_parts.empty() )
            {
                m_cv.wait( l, [ this ](){ return ( !m_expression_parts.empty() || !m_running ); } );
                if( !m_running )
                {
                    throw calculation_aborted{};
                }
            }

            std::string& curr_part = m_expression_parts.front();
            m_read_pos = curr_part.find_first_not_of( ' ', m_read_pos );

            if( m_read_pos >= curr_part.length() )
            {
                m_read_pos = 0;
                m_expression_parts.pop();
            }
            else
            {
                break;
            }
        }

        return m_expression_parts.front()[ m_read_pos ];
    }

    type parse_number()
    {
        std::string number;

        char c{ get_character() };
        if( c == '-' )
        {
            number += c;
            ++m_read_pos;
            c = get_character();
        }

        while( detail::get_entry_type( c ) == detail::entry_type::number )
        {
            number += c;
            ++m_read_pos;
            c = get_character();
        }

        if( number.empty() || ( number.length() == 1 && number.front() == '-' ) )
        {
            throw std::logic_error{ std::string{ "Invalid expression: number parse failed at " } + std::to_string( m_read_pos ) };
            //throw std::logic_error{ "Invalid expression: number parse failed" };
        }

        --m_read_pos;

        return boost::lexical_cast< type >( number );
    }

    void maybe_swap_top_subexpr_start()
    {
        using namespace detail;
        if( !m_operator_stack.empty() )
        {
            if( m_operator_stack.top() == operator_type::subexpr_start )
            {
                m_operator_stack.pop();
                m_operator_stack.push( operator_type::subexpr_first_num );
            }
        }
        else
        {
            throw std::logic_error{ "Invalid expression: invalid brackets or operator position" };
        }
    }

    void calc_subexpression( const detail::operator_type& oper )
    {
        using namespace detail;

        // Maybe unwind stack and push new operator
        while( !m_operator_stack.empty() &&
               get_precedence( oper ) <= get_precedence( m_operator_stack.top() ) )
        {
            // All the supported operators reqiure at least two numbers
            if( m_numbers.size() < 2 )
            {
                throw std::logic_error{ "Invalid expression: not enough operator arguments provided" };
            }

            operator_type& top_oper = m_operator_stack.top();
            m_operator_stack.pop();

            type value{ std::move( m_numbers.top() ) };
            m_numbers.pop();

            type older_value{ std::move( m_numbers.top() ) };
            m_numbers.pop();

            m_numbers.push( calc_math( older_value, value, top_oper ) );
            maybe_swap_top_subexpr_start();
        }
    }

    void parse_subexpression()
    {
        using namespace detail;
        assert( !m_operator_stack.empty() );

        bool subexpr_parse_finished{ false };

        do
        {
            char curr_char{ get_character() };
            entry_type entry{ get_entry_type( curr_char ) };

            if( entry == entry_type::opening_bracket )
            {
                m_operator_stack.push( operator_type::subexpr_start );
                //parse_finished = true;
            }
            else if( entry == entry_type::closing_bracket || entry == entry_type::expr_end )
            {
                calc_subexpression( operator_type::end );

                if( m_operator_stack.top() != operator_type::subexpr_first_num )
                {
                    throw std::logic_error{ "Invalid expression: empty subexpression" };
                }

                m_operator_stack.pop(); // pop subexpr_with_num

                if( entry == entry_type::expr_end )
                {
                    m_calculation_finished = true;
                }
                else
                {
                    maybe_swap_top_subexpr_start();
                }

                subexpr_parse_finished = true;
            }
            else if( entry == entry_type::number ||
                     ( m_operator_stack.top() == operator_type::subexpr_start && curr_char == '-' ) )
            {
                m_numbers.push( parse_number() );
                maybe_swap_top_subexpr_start();
            }
            else if( entry == entry_type::math )
            {
                if( m_operator_stack.top() == operator_type::subexpr_start )
                {
                    throw std::logic_error{ "Invalid expression: math sighs follows start of subexpression" };
                }

                operator_type new_oper{ get_oper_type( curr_char ) };
                if( m_operator_stack.top() != operator_type::subexpr_start &&
                    m_operator_stack.top() != operator_type::subexpr_first_num )
                {
                    calc_subexpression( new_oper );
                }

                m_operator_stack.push( new_oper );
            }

            ++m_read_pos;
        }
        while( !subexpr_parse_finished );
    }

    type calculate()
    {
        type result{};

        try
        {
            m_operator_stack.push( detail::operator_type::subexpr_start );

            while( !m_operator_stack.empty() && !m_calculation_finished )
            {
                parse_subexpression();
            }

            std::lock_guard< std::mutex > l { m_mutex };

            if( m_expression_parts.size() != 1 ||
                !m_operator_stack.empty() ||
                m_numbers.size() != 1 )
            {
                throw std::logic_error{ "Invalid expression: end" };
            }

            result = m_numbers.top();
            m_numbers.pop();
            m_running = false;
        }
        catch( ... )
        {
            m_running = false;
            m_error_occured = true;
            m_calculation_finished = true;

            std::lock_guard< std::mutex > l { m_mutex };
            clean_parse_data();

            // propagate exception to future
            throw;
        }

        return result;
    }

private:
    std::stack< type > m_numbers;
    std::stack< detail::operator_type > m_operator_stack;

    std::size_t m_read_pos{ 0 };
    std::queue< std::string > m_expression_parts;

    // not sure why, but simple m_running{ false }
    // causes gcc 4.8.4 to call deleted
    // copy constructon instead of direct initialization
    // looks like a bug, = {} fixes it
    std::atomic_bool m_running = { false };
    std::atomic_bool m_error_occured = { false };
    std::atomic_bool m_calculation_finished = { false };

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
};

} // calc

#endif

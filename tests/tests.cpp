#define BOOST_TEST_MODULE "Tests"

#include <map>
#include <boost/test/included/unit_test.hpp>

#include "generator.h"

#include "mocks.h"

BOOST_AUTO_TEST_CASE( calc_full_expr )
{
    std::map< std::string, int64_t > expr_res_map
    {
        { "1 + 2\n", 3 },
        { "1 - 2\n", -1 },
        { "1 * 2\n", 2 },
        { "4 / 2\n", 2 },
        { "(4 - 2 ) - ( 5 * 3 )\n", -13 },
        { "1 + 2 *( 3 - 4 / ( 5 -3 ) )\n", 3 }
    };

    for( const auto& expr_res : expr_res_map )
    {
        calc::async_calculator< int64_t > c;
        std::future< int64_t > f;
        int64_t result;

        BOOST_REQUIRE_NO_THROW( f = std::move( c.start( expr_res.first ) ) )
        BOOST_REQUIRE_NO_THROW( result = f.get() )
        BOOST_REQUIRE( result = expr_res.second );
    }
}

void check_expr_throw( const std::string& expr )
{
    calc::async_calculator< int64_t > c;
    std::future< int64_t > f;

    BOOST_REQUIRE_NO_THROW( f = c.start( expr ) )
    BOOST_REQUIRE_THROW( f.get(), std::logic_error )
    BOOST_REQUIRE( c.error_occured() );
    BOOST_REQUIRE( !c.running() );
    BOOST_REQUIRE( c.finished() );
}

// Load and check args
BOOST_AUTO_TEST_CASE( calc_invalid_expr )
{

    check_expr_throw( "\n" );
    check_expr_throw( "(\n" );
    check_expr_throw( "1(\n" );
    check_expr_throw( "1(\n" );
    check_expr_throw( "1)" );
    check_expr_throw( ")\n" );
    check_expr_throw( "+\n" );
    check_expr_throw( "+ 1" );
    check_expr_throw( "(+1" );
    check_expr_throw( "1 +\n" );
    check_expr_throw( "1 + +" );
    check_expr_throw( "+ + 1" );
    check_expr_throw( "- 1 +\n" );
    check_expr_throw( " - - 1" );
    check_expr_throw( "1 + )" );
    check_expr_throw( "1 + 2 )\n" );
    check_expr_throw( "1 + 2 * (\n" );
    check_expr_throw( "1 / 0\n" );

    calc::async_calculator< int64_t > c;
    std::future< int64_t > f;

    BOOST_REQUIRE_THROW( c.start( "" ), std::invalid_argument )
    BOOST_REQUIRE_THROW( c.add_expr_part( "" ), std::invalid_argument )
    BOOST_REQUIRE_THROW( c.add_expr_part( "1+2\n" ), std::logic_error )

    // add_expr_part

    BOOST_REQUIRE_NO_THROW( f = c.start( "1 + 2 * (" ) )
    BOOST_REQUIRE_THROW( c.add_expr_part( "" );, std::invalid_argument )
    BOOST_REQUIRE_NO_THROW( c.add_expr_part( "\n" ) )
    BOOST_REQUIRE_THROW( f.get(), std::logic_error )
}

BOOST_AUTO_TEST_CASE( calc_sequential_expr_supply )
{
    std::vector< std::string > expr_parts;
    expr_parts.emplace_back( "1 + 2 *" );
    expr_parts.emplace_back( "( 3 - 4 /" );
    expr_parts.emplace_back( " ( 5 -3 ) )" );
    expr_parts.emplace_back( "\n" );

    calc::async_calculator< int64_t > c;
    std::future< int64_t > f;
    int64_t result;

    BOOST_REQUIRE_NO_THROW( f = c.start( expr_parts[ 0 ] ) )

    for( size_t i{ 1 }; i < expr_parts.size(); ++i )
    {
        BOOST_REQUIRE_NO_THROW( c.add_expr_part( expr_parts[ i ] ) )
    }

    BOOST_REQUIRE_NO_THROW( result = f.get() )
    BOOST_REQUIRE( result = 3 );
    BOOST_REQUIRE( !c.error_occured() );
    BOOST_REQUIRE( !c.running() );
    BOOST_REQUIRE( c.finished() );
}

BOOST_AUTO_TEST_CASE( parser_abort )
{
    calc::async_calculator< int64_t > c;
    std::future< int64_t > f;

    BOOST_REQUIRE_NO_THROW( f = std::move( c.start( "1 + " ) ) )
    std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
    BOOST_REQUIRE( c.running() == true );
    BOOST_REQUIRE_NO_THROW( c.abort() )

    BOOST_REQUIRE_THROW( f.get(), calc::calculation_aborted )
    BOOST_REQUIRE( c.running() == false );
}

void verify_handle( calc::calc_handle< int64_t >& h, int64_t correct_result )
{
    std::string result;

    BOOST_REQUIRE( !h.error_occured() );
    BOOST_REQUIRE( !h.running() );
    BOOST_REQUIRE( h.finished() );
    BOOST_REQUIRE_NO_THROW( result = h.get_result() );
    BOOST_REQUIRE( result == std::to_string( correct_result ) );
}

BOOST_AUTO_TEST_CASE( calc_handle_test )
{
    {// full expr
        calc::calc_handle< int64_t > h;
        std::future< int64_t > f;

        std::string expr{ "1 + 2\n" };

        BOOST_REQUIRE_NO_THROW( h.on_data( expr.data(), expr.length() ) )
        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
        verify_handle( h, 3 );
    }

    {// part expr
        calc::calc_handle< int64_t > h;
        std::future< int64_t > f;
        int64_t result{};

        std::string expr1{ "1 + " };
        std::string expr2{ "2\n" };

        BOOST_REQUIRE_NO_THROW( h.on_data( expr1.data(), expr1.length() ) )
        BOOST_REQUIRE_NO_THROW( h.on_data( expr2.data(), expr2.length() ) )
        std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
        verify_handle( h, 3 );
    }
}

BOOST_AUTO_TEST_CASE( session_test_normal_data_addition )
{
    using namespace network::detail;

    BOOST_REQUIRE_THROW( mock_session{ nullptr }, std::invalid_argument )
    auto handle = std::make_unique< mock_calc_handle >();
    mock_calc_handle* handle_ptr{ handle.get() };

    mock_session s{ std::move( handle ) };
    BOOST_REQUIRE( !s.finished() );
    BOOST_REQUIRE_NO_THROW( s.start() )
    BOOST_REQUIRE( s.reads_occured == 1 );
    BOOST_REQUIRE( !s.write_occured );

    std::string test{ "test" };
    BOOST_REQUIRE_NO_THROW( s.on_data_accessor( test.data(), test.length(), false ) )
    BOOST_REQUIRE( s.reads_occured == 2 );
    BOOST_REQUIRE( handle_ptr->_on_data_calls == 1 );
    BOOST_REQUIRE( !s.write_occured );

    handle_ptr->_finished = true;
    BOOST_REQUIRE_NO_THROW( s.on_data_accessor( test.data(), test.length(), true ) )
    BOOST_REQUIRE( handle_ptr->_on_data_calls == 2 );
    BOOST_REQUIRE( s.finished() );
    BOOST_REQUIRE( s.write_occured );
    BOOST_REQUIRE( handle_ptr->_result_taken );
}

BOOST_AUTO_TEST_CASE( session_test_on_error )
{
    using namespace network::detail;

    auto handle = std::make_unique< mock_calc_handle >();
    mock_calc_handle* handle_ptr{ handle.get() };

    mock_session s{ std::move( handle ) };
    handle_ptr->_error_occured = true;
    handle_ptr->_finished = true;

    std::string test{ "test" };
    BOOST_REQUIRE_NO_THROW( s.on_data_accessor( test.data(), test.length(), true ) )
    BOOST_REQUIRE( s.finished() );
    BOOST_REQUIRE( s.write_occured );
    BOOST_REQUIRE( handle_ptr->_result_taken );
}

BOOST_AUTO_TEST_CASE( server_test )
{
    using namespace network::detail;

    boost::asio::io_service s;
    mock_handle_factory factory;
    test_server server{ factory, s, 1 };

    auto& running_sessions = server.get_running_sessions();
    auto& waiting_session = server.get_waiting_session();

    // test start()
    server.start();
    BOOST_REQUIRE( server._accept_next_connection_called = 1 );
    BOOST_REQUIRE( running_sessions.size() == 0 );
    BOOST_REQUIRE( waiting_session != nullptr );

    // test handle_connection()
    boost::system::error_code e{};
    BOOST_REQUIRE_NO_THROW( server.handle_connection_accessor( waiting_session, e ) )
    BOOST_REQUIRE( server._accept_next_connection_called == 2 );
    BOOST_REQUIRE( waiting_session != nullptr );
    BOOST_REQUIRE( running_sessions.size() == 1 );

    // test max connections logic
    BOOST_REQUIRE_NO_THROW( server.handle_connection_accessor( waiting_session, e ) )
    BOOST_REQUIRE( running_sessions.size() == 1 );

    // test session removal
    std::string test{ "test" };
    auto test_session = std::dynamic_pointer_cast< mock_session >( running_sessions.back() );
    BOOST_REQUIRE_NO_THROW( test_session->on_data_accessor( test.data(), test.length(), true ) )
    BOOST_REQUIRE( test_session->finished() );

    BOOST_REQUIRE_NO_THROW( server.handle_connection_accessor( waiting_session, e ) )
    BOOST_REQUIRE( server.get_running_sessions().size() == 1 );
}

bool is_negative_num( const std::string& expr, size_t pos )
{
    using namespace calc::detail;
    return pos <= expr.length() - 2 &&
            expr[ pos ] == '-' &&
            get_entry_type( expr[ pos + 1 ] ) == entry_type::number;
}

bool character_valid( const std::string& expr, size_t& pos )
{
    using namespace calc::detail;

    char c{ expr[ pos ] };
    entry_type char_type{ get_entry_type( c ) };

    bool valid{ true };

    char next;
    entry_type next_char_type;
    if( pos != expr.length() - 1 )
    {
        next = expr[ pos + 1 ];
        next_char_type = get_entry_type( next );
    }

    if( char_type == entry_type::math )
    {
        if( next_char_type != entry_type::opening_bracket &&
            next_char_type != entry_type::number )
        {
            valid = false;
        }
    }
    else if( char_type == entry_type::number ||
             ( char_type == entry_type::math && is_negative_num( expr, pos ) ) )
    {
        // eat number
        if( c == '-' )
        {
            ++pos;
        }

        while( pos < expr.length() &&
               get_entry_type( expr[ pos ] ) == entry_type::number )
        {
            ++pos;
        }

        --pos;

        if( pos == expr.length() )
        {
            valid = false;
        }
        else
        {
            std::string asd{ expr[ pos + 1 ] };
            entry_type next_char_type{ get_entry_type( expr[ pos + 1 ] ) };

            if( next_char_type != entry_type::math &&
                next_char_type != entry_type::closing_bracket &&
                next_char_type != entry_type::expr_end )
            {
                valid = false;
            }
        }
    }
    else if( char_type == entry_type::opening_bracket )
    {
        if( next_char_type != entry_type::opening_bracket &&
            next_char_type != entry_type::number &&
            !is_negative_num( expr, pos + 1 ) )
        {
            valid = false;
        }
    }
    else if( char_type == entry_type::closing_bracket )
    {
        if( next_char_type != entry_type::closing_bracket &&
            next_char_type != entry_type::math &&
            next_char_type != entry_type::expr_end )
        {
            valid  = false;
        }
    }
    else if( char_type == entry_type::expr_end )
    {
        if( pos != expr.length() - 1 )
        {
            valid  = false;
        }
    }

    if( !valid )
    {
    int  i =0;
    }

    return valid;
}

BOOST_AUTO_TEST_CASE( generator_test )
{
    std::string dest_file{ "dest" };
    uint64_t size{ 100 };

    BOOST_REQUIRE_THROW( generate_expression( "", size ), std::ios_base::failure )
    BOOST_REQUIRE_THROW( generate_expression( "dest", 2 ), std::invalid_argument )

    BOOST_REQUIRE_NO_THROW( generate_expression( dest_file, size ) )

    std::ifstream in{ dest_file, std::ifstream::ate };
    BOOST_REQUIRE( in.is_open() );
    BOOST_REQUIRE( in.tellg() <= size * 2 );

    in.seekg( 0 );
    std::string str{ ( std::istreambuf_iterator<char>{ in } ),
                       std::istreambuf_iterator<char>{} };

    for( size_t i{ 0 }; i < str.length(); ++i )
    {
        bool valid{ false };
        BOOST_REQUIRE_NO_THROW( valid = character_valid( str, i ) )
        BOOST_REQUIRE( valid );
    }
}

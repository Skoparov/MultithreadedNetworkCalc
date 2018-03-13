#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "generator.h"

enum class work_mode{ generate, repeat };

work_mode get_mode( const std::string& mode_str )
{
    work_mode result;
    if( mode_str == "generate" )
    {
        result = work_mode::generate;
    }
    else if( mode_str == "repeat" )
    {
        result = work_mode::repeat;
    }
    else
    {
        throw std::invalid_argument{ "Invalid mode: should be \"generate\" or \"repeat\"" };
    }

    return result;
}

struct settings
{
    work_mode mode{ work_mode::generate };
    std::string dest_file;
    uint64_t approx_size{ 0 };
    std::string source_file;
    bool only_show_help{ false };
};

settings get_settings( int argc, char** argv )
{
    namespace bpo = boost::program_options;
    settings s;
    std::string work_mode_str;

    bpo::options_description desc{ "Usage" };
    desc.add_options()
            ( "help,h", "show usage" )
            ( "mode,m", bpo::value( &work_mode_str ), "work mode(generate/repeat)" )
            ( "dest,d", bpo::value( &s.dest_file ), "destination file name" )
            ( "approx_size,s", bpo::value( &s.approx_size ), "Approximate size in bytes" )
            ( "from,f", bpo::value( &s.source_file ), "Source file with math expression(only for repeat)" );

    bpo::variables_map map;
    bpo::store( bpo::parse_command_line( argc, argv, desc ), map );

    if( map.count( "help" ) )
    {
        std::cout << desc << std::endl;
        s.only_show_help = true;
    }
    else
    {
        if( map.count( "mode" ) )
        {
            s.mode = get_mode( map[ "mode" ].as< std::string >() );
        }
        else
        {
            throw std::logic_error{ "Mode not specified" };
        }

        if( map.count( "dest" ) )
        {
            s.dest_file = map[ "dest" ].as< std::string >();
        }
        else
        {
            throw std::logic_error{ "Destination file not specified" };
        }

        if( map.count( "approx_size" ) )
        {
            s.approx_size = map[ "approx_size" ].as< uint64_t >();
        }
        else
        {
            throw std::logic_error{ "Approximate size not specified" };
        }

        if( map.count( "from" ) )
        {
            s.source_file = map[ "from" ].as< std::string >();
        }
        else if( s.mode == work_mode::repeat )
        {
            throw std::logic_error{ "Expression source file not specified" };
        }
    }

    return s;
}

int main( int argc, char *argv[] )
{
    try
    {
        settings s = get_settings( argc, argv );
        if( s.only_show_help )
        {
            return 0;
        }

        if( s.mode == work_mode::generate )
        {
            generate_expression( s.dest_file, s.approx_size );
        }
        else if( s.mode == work_mode::repeat )
        {
            repeat_expression_for_size( s.dest_file, s.source_file, s.approx_size );
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

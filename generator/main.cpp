#include <iostream>

#include "generator.h"
#include <stdexcept>

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

enum arg_pos{ mode_pos = 1, dest_file_pos, size_pos, source_file_pos };

struct settings
{
    work_mode mode;
    std::string dest_file;
    uint64_t approx_size;
    std::string source_file;
};

settings get_settings( int argc, char** argv )
{
    if( argc < size_pos + 1 )
    {
        throw std::logic_error{
            "Usage: ./generator  %mode(generate, repeat) %dest_file_name %approx_max_size_in_bytes %source_file(only for repeat)" };
    }

    settings sett;

    sett.mode = get_mode( argv[ mode_pos ] );
    sett.dest_file = argv[ dest_file_pos ];
    sett.approx_size =  std::stoul( argv[ size_pos ] );

    if( sett.mode == work_mode::repeat )
    {
        if( argc == source_file_pos + 1 )
        {
            sett.source_file = argv[ source_file_pos ];
        }
        else
        {
            throw std::logic_error{ "Should provide source file to repeat" };
        }
    }

    return sett;

}

int main( int argc, char *argv[] )
{
    try
    {
        settings s = get_settings( argc, argv );

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

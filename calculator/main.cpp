#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

#include "server.h"
#include "calc_handle_factory.h"

#include "big_int/BigIntegerUtils.hh"

static constexpr uint16_t default_port{ 6666 };

struct settings
{
    uint16_t port{ default_port };
    uint32_t max_connections{ std::thread::hardware_concurrency() };
    bool only_show_help{ false };
};

settings get_settings( int argc, char** argv )
{
    namespace bpo = boost::program_options;
    settings s;

    bpo::options_description desc{ "Usage" };
    desc.add_options()
            ( "help,h", "show usage" )
            ( "port,p", bpo::value( &s.port ), "server port, default = 6666" )
            ( "max_connections,c", bpo::value( &s.max_connections ),
              "maximum connection, default = hardware concurrency" );

    bpo::variables_map map;
    bpo::store( bpo::parse_command_line( argc, argv, desc ), map );

    if( map.count( "help" ) )
    {
        std::cout << desc << std::endl;
        s.only_show_help = true;
    }
    else
    {
        if( map.count( "port" ) )
        {
            s.port = map[ "port" ].as< uint16_t >();
        }

        if( map.count( "max_connections" ) )
        {
            s.max_connections = map[ "max_connections" ].as< uint32_t >();
        }
    }

    return s;
}

#include <queue>

int main( int argc, char *argv[] )
{
    try
    {
        settings s{ get_settings( argc, argv ) };
        if( s.only_show_help )
        {
            return 0;
        }

        boost::asio::io_service io_service;
        calc::calc_handle_factory< BigInteger > factory;
        network::tcp_calc_server server{ factory, io_service, s.port, s.max_connections };

        boost::asio::signal_set signals_to_handle{ io_service, SIGINT, SIGTERM };
        signals_to_handle.async_wait( [ & ]( const boost::system::error_code&, int )
        {
            server.stop();
            io_service.stop();
        } );

        std::cout << "Listening to port " << s.port << std::endl;

        server.start();
    }
    catch( const calc::calculation_aborted& )
    {
        std::cerr << "Calculation aborted" << std::endl;
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

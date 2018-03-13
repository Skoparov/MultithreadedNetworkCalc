#include <iostream>
#include <fstream>

#include "server.h"
#include "calculator.h"
#include "calc_handle_factory.h"

#include "big_int/BigIntegerUtils.hh"

static constexpr uint16_t default_port{ 6666 };

void show_help()
{
    std::cerr << "Usage: ./calc_server  %port(optional, default = 6666)" << std::endl;
}

int main( int argc, char *argv[] )
{
    if( argc > 2 )
    {
        show_help();
        return 1;
    }

    uint16_t port{ default_port };
    if( argc == 2 )
    {
        try
        {
            port = static_cast< uint16_t >( std::stoul( argv[ 1 ] ) );
        }
        catch( ... )
        {
            std::cerr << "Invalid port" << std::endl;
            show_help();
            return 1;
        }
    }

    boost::asio::io_service io_service;

    try
    {
        calc::calc_handle_factory< BigInteger > factory;
        network::tcp_calc_server server{ factory, io_service, port };

        boost::asio::signal_set signals_to_handle{ io_service, SIGINT, SIGTERM };
        signals_to_handle.async_wait( [ & ]( const boost::system::error_code&, int )
        {
            server.stop();
            io_service.stop();
        } );

        std::cout << "Server will start on port " << port << std::endl;

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

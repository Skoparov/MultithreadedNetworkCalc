#include <iostream>
#include <fstream>

#include "server.h"
#include "calculator.h"
#include "calc_handle_factory.h"

#include "big_int/BigIntegerUtils.hh"

static constexpr uint16_t default_port{ 6666 };

int main( int argc, char *argv[] )
{
    if( argc > 2 )
    {
        std::cerr << "Usage: ./calc_server  %port(optional, default = 6666)";
        return 1;
    }

    try
    {
        uint16_t port{ argc == 2?
                        static_cast< uint16_t >( std::stoul( argv[ 1 ] ) ) :
                        default_port };


        boost::asio::io_service io_service;
        calc::calc_handle_factory< BigInteger > factory;
        network::tcp_calc_server server{ factory, io_service, port };

        boost::asio::signal_set signals_to_handle{ io_service, SIGINT, SIGTERM };
        signals_to_handle.async_wait( [ & ]( const boost::system::error_code&, int )
        {
            server.stop();
            io_service.stop();
        } );

        server.start();

        std::cout << "Server started on port " << port << std::endl;

        io_service.run();
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

#include "server.h"

#include <iostream>

#include "calc_handle_factory.h"

namespace ba = boost::asio;
namespace bs = boost::system;

namespace network
{

namespace detail
{

abstract_calc_session::abstract_calc_session( std::unique_ptr< calc::abstract_calc_handle > handle ) :
    m_handle( std::move( handle ) )
{
    if( !m_handle )
    {
        throw std::invalid_argument{ "Handle not initialized" };
    }
}

abstract_calc_session::~abstract_calc_session()
{
    try
    {
        if( m_handle->running() )
        {
            m_handle->abort();
            m_handle->get_result();
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
    }
}

void abstract_calc_session::start()
{
    read_next();
}

bool abstract_calc_session::finished() const noexcept
{
    return m_finished;
}

void abstract_calc_session::on_data( const char* data, uint64_t size, bool eof )
{
    assert( data );

    bool transmit_complete{ false };
    bool error_happened{ m_handle->error_occured() };

    if( size && !error_happened )
    {
        transmit_complete = ( eof || data[ size - 1 ] == '\n' );
        m_handle->on_data( data, size, transmit_complete );
    }

    if( transmit_complete || error_happened )
    {
        write_result();
    }

    // Close session if eof has been received, continue listening otherwise
    if( !eof )
    {
        read_next();
    }
    else
    {
        m_finished = true;
    }
}

void abstract_calc_session::write_result()
{
    std::string result;

    try
    {
        result = m_handle->get_result();
    }
    catch( const std::exception& e )
    {
        // report error to client
        result = e.what();
    }

    result += '\n';

    write( result );
    m_handle->reset();
}

tcp_calc_session::tcp_calc_session( ba::io_service& io_service,
                                    std::unique_ptr< calc::abstract_calc_handle > handler ) :
    abstract_calc_session( std::move( handler ) ),
    m_socket( io_service ),
    m_strand( io_service ){}

ba::ip::tcp::socket& tcp_calc_session::socket() noexcept
{
    return m_socket;
}

void tcp_calc_session::read_next()
{
    auto handler = std::bind( &tcp_calc_session::on_socket_data,
                              shared_from_this(),
                              std::placeholders::_1,
                              std::placeholders::_2 );

    m_socket.async_read_some( ba::buffer( m_buffer.data(), m_buffer.size() ),
                              m_strand.wrap( handler ) );
}

void tcp_calc_session::write( const std::string& result )
{
    ba::write( m_socket, ba::buffer( result.data(), result.length() ) );
}

void tcp_calc_session::on_socket_data( const bs::error_code& err, uint64_t bytes_transferred )
{
    if( !err || err.value() == boost::asio::error::eof )
    {
        on_data( m_buffer.data(), bytes_transferred, err );
    }
    else
    {
        std::cerr << err.message() << std::endl;
    }
}

}// detail

//

abstract_calc_server::abstract_calc_server( calc::abstract_calc_handle_factory& factory,
                                            ba::io_service& io_service,
                                            uint32_t max_sessions ) noexcept:
    m_handle_factory( factory ),
    m_io_service( io_service ),
    m_max_sessions( max_sessions ){}

void abstract_calc_server::start()
{
    m_waiting_session = create_new_session( m_handle_factory.create() );
    accept_next_connection();

    for( size_t i{ 0 }; i < m_max_sessions; ++i )
    {
        m_pool.create_thread( [ & ](){ m_io_service.run(); } );
    }

    m_pool.join_all();
}

void abstract_calc_server::handle_connection( std::shared_ptr< detail::abstract_calc_session >& session,
                                              const bs::error_code& err )
{
    if( !err )
    {
        m_running_sessions.erase( std::remove_if( m_running_sessions.begin(), m_running_sessions.end(),
                              []( const std::shared_ptr< detail::abstract_calc_session >& session )
                              {
                                 return session->finished();
                              } ), m_running_sessions.end() );

        if(  m_running_sessions.size() < m_max_sessions )
        {
            session->start();
            m_running_sessions.push_back( session );
            m_waiting_session = create_new_session( m_handle_factory.create() );
        }

        accept_next_connection();
    }
    else
    {
        std::cerr << err.message() << std::endl;
    }
}

tcp_calc_server::tcp_calc_server( calc::abstract_calc_handle_factory& factory,
                                  boost::asio::io_service& io_service,
                                  uint16_t port,
                                  uint32_t max_connections ):
    abstract_calc_server( factory, io_service, max_connections ),
    m_acceptor( m_io_service,
                ba::ip::tcp::endpoint{ ba::ip::tcp::v4(), port },
                false ){}

void tcp_calc_server::stop()
{
    if( m_acceptor.is_open() )
    {
        m_acceptor.close();
    }

    m_running_sessions.clear();
}

bool tcp_calc_server::running() const
{
    return m_acceptor.is_open();
}

std::shared_ptr< detail::abstract_calc_session > tcp_calc_server::create_new_session(
        std::unique_ptr< calc::abstract_calc_handle > handle )
{
    return std::make_shared< detail::tcp_calc_session >( m_io_service, std::move( handle ) );
}

void tcp_calc_server::accept_next_connection()
{
    assert( m_waiting_session );

    auto tcp_session = std::dynamic_pointer_cast< detail::tcp_calc_session >( m_waiting_session );
    auto handler = std::bind( &tcp_calc_server::handle_connection,
                              this,
                              m_waiting_session,
                              std::placeholders::_1 );

    m_acceptor.async_accept( tcp_session->socket(), handler );
}

}// network

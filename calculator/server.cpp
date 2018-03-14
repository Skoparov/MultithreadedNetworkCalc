#include "server.h"

#include "logger.h"
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
        logger::log( e.what(), logger::to::cerr );
    }
}

void abstract_calc_session::start()
{
    m_handle->reset();
    read_next();
}

void abstract_calc_session::stop() noexcept
{
    m_handle->abort();
}

bool abstract_calc_session::finished() const noexcept
{
    return m_finished;
}

void abstract_calc_session::on_data( const char* data, uint64_t size, bool eof )
{
    assert( data );

    bool transmit_complete{ false };
    bool error_occured{ m_handle->error_occured() };

    if( size && !error_occured )
    {
        transmit_complete = ( eof || data[ size - 1 ] == '\n' );
        m_handle->on_data( data, size, transmit_complete );
    }

    if( transmit_complete || error_occured )
    {
        write_result();
    }

    // Close session if client has closed it, continue reading otherwise
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
    std::string result{ m_handle->get_result() + '\n' };
    write( result );
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
    auto handler = std::bind( &tcp_calc_session::start,
                              shared_from_this() );

    m_socket.async_write_some( ba::buffer( result.data(), result.length() ), m_strand.wrap( handler ) );
}

void tcp_calc_session::on_socket_data( const bs::error_code& err, uint64_t bytes_transferred )
{
    if( !err || err.value() == boost::asio::error::eof )
    {
        on_data( m_buffer.data(), bytes_transferred, err );
    }
    else
    {
        logger::log( err.message(), logger::to::cerr );
    }
}

}// detail

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

    std::exception_ptr e_ptr;

    try
    {
        for( size_t i{ 0 }; i < m_max_sessions; ++i )
        {
            m_pool.create_thread( [ & ](){ m_io_service.run(); } );
        }
    }
    catch( ... )
    {
        m_io_service.stop();
        e_ptr = std::current_exception();
    }

    m_pool.join_all();

    if( e_ptr )
    {
        std::rethrow_exception( e_ptr );
    }
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
        }
        else
        {
            logger::log( "Connection refused: limit reached", logger::to::cerr );
        }

        m_waiting_session = create_new_session( m_handle_factory.create() );
        accept_next_connection();
    }
    else
    {
        logger::log( err.message(), logger::to::cerr );
    }
}

tcp_calc_server::tcp_calc_server( calc::abstract_calc_handle_factory& factory,
                                  boost::asio::io_service& io_service,
                                  uint16_t port,
                                  uint32_t max_connections ):
    abstract_calc_server( factory, io_service, max_connections ),
    m_acceptor( m_io_service,
                ba::ip::tcp::endpoint{ ba::ip::tcp::v4(), port }, false ){}

void tcp_calc_server::stop()
{
    std::lock_guard< std::mutex > l{ m_mutex };

    for( auto& session : m_running_sessions )
    {
        session->stop();
    }

    m_running_sessions.clear();
    m_waiting_session.reset();

    if( m_acceptor.is_open() )
    {
        m_acceptor.close();
    }
}

bool tcp_calc_server::running() const
{
    std::lock_guard< std::mutex > l{ m_mutex };
    return m_acceptor.is_open();
}

std::shared_ptr< detail::abstract_calc_session > tcp_calc_server::create_new_session(
        std::unique_ptr< calc::abstract_calc_handle > handle )
{
    return std::make_shared< detail::tcp_calc_session >( m_io_service, std::move( handle ) );
}

void tcp_calc_server::accept_next_connection()
{
    std::lock_guard< std::mutex > l{ m_mutex };
    assert( m_waiting_session );

    auto handler = std::bind( &tcp_calc_server::handle_connection,
                              this,
                              m_waiting_session,
                              std::placeholders::_1 );

    auto tcp_session = std::dynamic_pointer_cast< detail::tcp_calc_session >( m_waiting_session );
    m_acceptor.async_accept( tcp_session->socket(), handler );
}

}// network

#include "server.h"

#include <iostream>

#include "calc_handle_factory.h"

namespace network
{

namespace ba = boost::asio;
namespace bs = boost::system;

namespace detail
{

abstract_calc_session::abstract_calc_session( std::unique_ptr<calc::abstract_calc_handle> handle ) :
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

            assert( !m_result_waiter.valid() );
        }
        else if( m_result_waiter.valid() )
        {
            m_result_waiter.get();
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
    }
}

void abstract_calc_session::start()
{
    m_finished = false;
    read_next();
}

bool abstract_calc_session::finished() const noexcept
{
    return m_finished;
}

void abstract_calc_session::on_data( const char* data, uint64_t size, bool end )
{
    if( !data )
    {
        throw std::invalid_argument{ "data is not initialized" };
    }

    if( m_handle->finished() && m_handle->error_occured() )
    {
        // don't wait for transfer finish, just end it here
        write_result();
        return;
    }

    m_handle->on_data( data, size, end );

    if( end )
    {
        if( m_handle->finished() )
        {
            // send result right away
            write_result();
        }
        else
        {
            // wait for result asynchronously and send it
            m_result_waiter = std::async( std::launch::async,
                                          &abstract_calc_session::write_result, this );
        }
    }
    else
    {
        read_next();
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

    write( result );
    m_finished = true;
}

tcp_calc_session::tcp_calc_session( ba::io_service& io_service,
                                    std::unique_ptr<calc::abstract_calc_handle> handler ) :
    abstract_calc_session( std::move( handler ) ),
    m_socket( io_service ){}

ba::ip::tcp::socket& tcp_calc_session::socket() noexcept
{
    return m_socket;
}

void tcp_calc_session::read_next()
{
    ba::async_read( m_socket,
                    ba::buffer( m_buffer.data(), m_buffer.size() ),
                    std::bind( &tcp_calc_session::on_socket_data,
                               shared_from_this(),
                               std::placeholders::_1,
                               std::placeholders::_2 ) );
}

void tcp_calc_session::write( const std::string& result )
{
    ba::write( m_socket, ba::buffer( result.data(), result.length() ) );
}

void tcp_calc_session::on_socket_data(const bs::error_code& err, uint64_t bytes_transferred )
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

abstract_calc_server::abstract_calc_server( calc::abstract_calc_handle_factory& factory ) noexcept:
    m_factory( factory ){}

void abstract_calc_server::start()
{
    // Remove finished sessions
    m_sessions.erase( std::remove_if( m_sessions.begin(), m_sessions.end(),
                      []( const std::shared_ptr< detail::abstract_calc_session >& session )
                      {
                         return session->finished();
                      } ), m_sessions.end() );

    m_sessions.emplace_back( create_new_session( m_factory.create() ) );

    wait_for_connection();
}

void abstract_calc_server::handle_connection( std::shared_ptr< detail::abstract_calc_session >& session,
                                              const bs::error_code& err )
{
    if( !err )
    {
        session->start();
        start();
    }
    else
    {
        std::cerr << err.message() << std::endl;
    }
}

tcp_calc_server::tcp_calc_server( calc::abstract_calc_handle_factory& factory,
                boost::asio::io_service& io_service,
                uint16_t port ):
    abstract_calc_server( factory ),
    m_io_service( io_service ),
    m_acceptor( io_service,
                ba::ip::tcp::endpoint{ ba::ip::tcp::v4(), port },
                false ){}

void tcp_calc_server::stop()
{
    if( m_acceptor.is_open() )
    {
        m_acceptor.close();
    }
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

void tcp_calc_server::wait_for_connection()
{
    assert( !m_sessions.empty() );
    auto& current_session = m_sessions.back();
    auto tcp_session = std::dynamic_pointer_cast< detail::tcp_calc_session >( current_session );

    m_acceptor.async_accept( tcp_session->socket(),
                             std::bind( &tcp_calc_server::handle_connection,
                                        this,
                                        current_session,
                                        std::placeholders::_1 ) );
}

}// network

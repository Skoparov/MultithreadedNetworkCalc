#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace calc
{

class abstract_calc_handle_factory;
class abstract_calc_handle;

}// calc

namespace network
{

namespace detail
{

class abstract_calc_session
{
public:
    abstract_calc_session( std::unique_ptr< calc::abstract_calc_handle > handle );
    virtual ~abstract_calc_session();

    virtual void start();
    void stop() noexcept;
    bool finished() const noexcept;

protected:
    void on_data( const char* data, uint64_t size, bool eof );
    virtual void read_next() = 0;
    virtual void write( const std::string& result ) = 0;

private:
    void write_result();

private:
    std::atomic_bool m_finished{ false };
    std::unique_ptr< calc::abstract_calc_handle > m_handle;
};

class tcp_calc_session : public std::enable_shared_from_this< tcp_calc_session >,
                         public abstract_calc_session
{
public:
    tcp_calc_session( boost::asio::io_service& io_service,
                      std::unique_ptr< calc::abstract_calc_handle > handler );

    boost::asio::ip::tcp::socket& socket() noexcept;

protected:
    void read_next() override;
    void write( const std::string& result ) override;

private:
    void on_socket_data( const boost::system::error_code& err,
                         uint64_t bytes_transferred );

private:
    std::array< char, 8192 > m_buffer;
    boost::asio::ip::tcp::socket m_socket;
    boost::asio::io_service::strand m_strand;
};

}// detail

class abstract_calc_server
{
public:
    abstract_calc_server( calc::abstract_calc_handle_factory& factory,
                          boost::asio::io_service& io_service,
                          uint32_t max_sessions ) noexcept;

    virtual ~abstract_calc_server() = default;

    void start();
    virtual void stop() = 0;
    virtual bool running() const = 0;

protected:
    virtual std::shared_ptr< detail::abstract_calc_session > create_new_session(
                    std::unique_ptr< calc::abstract_calc_handle > handle )  = 0;

    virtual void accept_next_connection() = 0;
    void handle_connection( std::shared_ptr< detail::abstract_calc_session >& session,
                            const boost::system::error_code& e );

protected:
    boost::thread_group m_pool;
    boost::asio::io_service& m_io_service;
    calc::abstract_calc_handle_factory& m_handle_factory;

    uint32_t m_max_sessions{ 0 };
    std::shared_ptr< detail::abstract_calc_session > m_waiting_session;
    std::list< std::shared_ptr< detail::abstract_calc_session > > m_running_sessions;
};

class tcp_calc_server : public abstract_calc_server
{
public:
    tcp_calc_server( calc::abstract_calc_handle_factory& factory,
                     boost::asio::io_service& io_service,
                     uint16_t port,
                     uint32_t max_sessions = boost::thread::hardware_concurrency() );

    void stop() override;
    bool running() const override;

protected:
    std::shared_ptr< detail::abstract_calc_session > create_new_session(
                std::unique_ptr< calc::abstract_calc_handle > handle ) override;

    void accept_next_connection() override;

private:
    boost::asio::ip::tcp::acceptor m_acceptor;
    mutable std::mutex m_mutex;
};

}// network

#endif

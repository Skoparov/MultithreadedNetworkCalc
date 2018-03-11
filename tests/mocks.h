#ifndef MOCKS_H
#define MOCKS_H

#include "calc_handle_factory.h"
#include "server.h"

class mock_calc_handle : public calc::abstract_calc_handle
{
public:
    void on_data( const char* data, uint64_t size, bool end = false ) override
    {
        ++_on_data_calls;
        if( end )
        {
            _finished = true;
        }
    }

    bool running() const noexcept override{ return _running; }
    bool finished() const noexcept override{ return _finished; }
    bool error_occured() const noexcept override{ return _error_occured; }
    void abort() override{ _running = false; }

    std::string get_result() override
    {
        _result_taken = true;
        return "test";
    }

    bool _running{ false };
    bool _finished{ false };
    bool _error_occured{ false };
    bool _result_taken{ false };
    uint64_t _on_data_calls{ 0 };
};

class mock_session : public network::detail::abstract_calc_session
{
public:
    using network::detail::abstract_calc_session::abstract_calc_session;
    void on_data_accessor( const char* data, uint64_t size, bool end )
    {
        on_data( data, size, end );
    }

protected:
    void read_next() override
    {
        ++reads_occured;
    }

    void write( const std::string& ) override
    {
        write_occured = true;
    }

public:
    uint64_t reads_occured{ 0 };
    bool write_occured{ false };
};

class test_server : public network::abstract_calc_server
{
public:
    using network::abstract_calc_server::abstract_calc_server;

    void stop() override{}
    bool running() const override
    {
        return true;
    }

    std::shared_ptr< network::detail::abstract_calc_session > create_new_session(
                std::unique_ptr< calc::abstract_calc_handle > handle ) override
    {
        return std::make_shared< mock_session >( std::move( handle ) );
    }

    void wait_for_connection()
    {
        ++_wait_for_connection_called;
    }

    std::list< std::shared_ptr< network::detail::abstract_calc_session > >& get_sessions() noexcept
    {
        return m_sessions;
    }

    void handle_connection_accessor( std::shared_ptr< network::detail::abstract_calc_session >& session,
                                     const boost::system::error_code& e )
    {
        handle_connection( session, e );
    }

    uint64_t _wait_for_connection_called{ 0 };
};

class mock_handle_factory : public calc::abstract_calc_handle_factory
{
public:
    std::unique_ptr< calc::abstract_calc_handle > create() const override
    {
        return std::make_unique< mock_calc_handle >();
    }
};

#endif

#ifndef CALC_HANDLE_FACTORY_H
#define CALC_HANDLE_FACTORY_H

#include <unordered_map>

#include "calc_handle.h"

namespace std
{

template< typename type, typename... ctor_args >
std::unique_ptr< type > make_unique( ctor_args&&... args )
{
    return std::unique_ptr< type >( new type{ std::forward< ctor_args >( args )... } );
}

}// std

namespace calc
{

class abstract_calc_handle_factory
{
public:
    virtual ~abstract_calc_handle_factory() = default;
    virtual std::unique_ptr< abstract_calc_handle > create() const = 0;
};

template < typename type >
class calc_handle_factory : public abstract_calc_handle_factory
{
public:
    std::unique_ptr< abstract_calc_handle > create() const override
    {
        return std::make_unique< calc_handle< type > >();
    }
};

} // calc

#endif

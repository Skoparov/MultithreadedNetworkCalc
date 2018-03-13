#include "logger.h"

#include <mutex>
#include <iostream>

namespace logger
{

std::mutex log_mutex;

void log( const std::string& message, const to& direction )
{
    std::lock_guard< std::mutex > l{ log_mutex };

    if( direction == to::cout )
    {
        std::cout << message << std::endl;
    }
    else
    {
        std::cerr << message << std::endl;
    }
}

}// logger

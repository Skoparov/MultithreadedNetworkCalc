#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace logger
{

enum class to{ cout, cerr };

void log( const std::string& message, const to& direction = to::cout );

}// logger

#endif

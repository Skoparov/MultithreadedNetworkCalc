#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>

// generates random expression
void generate_expression( const std::string& dest_file, uint64_t approx_max_size );

// reads expression from file, concatenates it using + and - until size is reached
void repeat_expression_for_size( const std::string& dest_file,
                                 const std::string& source_file,
                                 uint64_t approx_size );

#endif

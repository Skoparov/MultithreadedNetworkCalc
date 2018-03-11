#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>

void generate_expression( const std::string& dest_file, uint64_t approx_max_size );
void repeat_expression_for_size( const std::string& dest_file,
                                 const std::string& source_file,
                                 uint64_t approx_size );

#endif

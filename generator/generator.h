#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>

// generates random expression
void generate_expression( const std::string& dest_file, uint64_t approx_max_size );

// reads expression from file, concatenates it using + and - until size is reached
// Useful for debugging the claculator, since if the source expression is wrapped into
// parentheses, regardless of the resulting expression size it would still be equal to
// either thre result of the source expr or zero(depending on the number of repeats)
void repeat_expression_for_size( const std::string& dest_file,
                                 const std::string& source_file,
                                 uint64_t approx_size );

#endif

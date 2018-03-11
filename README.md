A small network calculator capable of receiving huge math expressions over network and sending the result or error to the client.
Supports multiple simultaneous clients. It takes 4 to 6 minutes to process 1 Gb of data in release build(depending on the resuling number length, transmission time is not taken into account). Uses 3rd party BigInteger class(https://mattmccutchen.net/bigint), but the parser itself is templated, so it can work with regular integral values as well(obviously it would speed up the calculation, but may result in overflow). As for now only addition, substraction, multiplication, division and nested parentheses are supported.

Usage: ./calc_server  %port(optional, default = 6666)

The repository also contains a math expression generator.
Generation modes:
1) generate: generates a random math expression while trying to keep it's size close to the one provided by user
2) repeat: reads the math expression from a file and repeats it, concatenating the parts using + or - until the required size is reached.

Both modes save the result to a file.

Usage: ./generator  %mode(generate, repeat) %dest_file_name %approx_max_size_in_bytes %source_file(only for repeat)

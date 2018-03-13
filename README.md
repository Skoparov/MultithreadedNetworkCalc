A small network calculator capable of receiving huge math expressions over tcp and sending the result/error back to the client.
Supports multiple simultaneous clients, doesn't close sessions upon calculation finish, to it's possible to use telnet to communicate. Allows to specify maximum number of clients. It takes 4 to 5 minutes to process 1 Gb of data in release build(depending on the resuling number length, transmission time is not taken into account). Uses 3rd party BigInteger class(https://mattmccutchen.net/bigint) with small modifications(move semantics), but the parser itself is templated, so it can work with regular integral values as well(obviously it would drastically speed up the calculation(up to 20 sec per 1 Gb), but may(and most likely will) result in overflow). As for now only addition, substraction, multiplication, division and nested parentheses are supported.

Usage:
  -h [ --help ]                show usage
  -p [ --port ] arg            server port, default = 6666
  -c [ --max_connections ] arg maximum connection, default = hardware concurrency

The repository also contains a math expression generator.
Generation modes:
1) generate: generates a random math expression while trying to keep it's size close to the one provided by user
2) repeat: reads the math expression from a file and repeats it, concatenating the parts using + or - until the required size is reached. Useful for debugging the calculator, since if the source expression is wrapped into parentheses, regardless of the resulting expression's size it will still be equal to either the result of the source expr or zero(depending on the number of repeats)

Both modes save the result to the destination file path provided by used.

Usage:
  -h [ --help ]            show usage
  -m [ --mode ] arg        work mode(generate/repeat)
  -d [ --dest ] arg        destination file name
  -s [ --approx_size ] arg Approximate size in bytes
  -f [ --from ] arg        Source file with math expression(only for repeat)
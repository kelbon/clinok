#define program_options_file "../tests/program_options.def"
#define store_main_args
#include "cli_interface.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
  cli::options o;
  o = cli::parse_or_exit(argc, argv);
  o.print_to([](auto&& x) { std::cerr << x; });
}
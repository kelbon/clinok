#define program_options_file "../tests/program_options.def"
#define store_main_args
#include "cli_interface.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
  cli::options o;
  if (o.hello_world != "hello, man")
    return -1;
  cli::error_code ec;
  o = cli::parse(argc, argv, ec);
  if (ec)
    return (std::cout << -2), -2;
  if (!o.mytag)
    return (std::cout << -3), -3;
  if (o.hello_world != "ITS HELLO WORLD")
    return (std::cout << -4), -4;
  if (!o.works)
    return (std::cout << -5), -5;
  std::cout << cli::program_name() << '\n';
  for (auto arg : cli::program_args())
    std::cout << arg << '\n';
  std::cout << "\noptions:\n";
  // TODO print("{}", o);
}
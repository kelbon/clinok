#define program_options_file "../tests/program_options.def"
#define store_main_args
#include "cli_interface.hpp"
#include <iostream>
// TODO если "краткая" опция через -, то искать среди всех, (starts_with), и если однозначно, то вызывать. Если неоднозначно, то нет(ошибка)
// TODO ?PATH?
// теперь после определения что такое база и что нужно можно улучшить реализацию и довести до идеала этот ограниченный функционал
// TODO quick exit?
int main(int argc, char* argv[]) {
  cli::options o;
  if (o.hello_world != "hello, man")
    return -1;
  o = cli::parse_or_exit(argc, argv);
  if (!o.mytag)
    return -3;
  if (o.hello_world != "ITS HELLO WORLD")
    return -4;
  if (!o.works)
    return -5;
  std::cout << cli::program_name() << '\n';
  for (auto arg : cli::program_args())
    std::cout << arg << '\n';
  std::cout << "\noptions:\n";
  // TODO print("{}", o);
}
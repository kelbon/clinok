#define program_options_file "../examples/program_options.def"
#include <clinok/cli_interface.hpp>

void your_logic(cli::color_e color, std::size_t count) {
  switch (color) {
    case cli::color_e::black:
      std::cout << "black: " << count;
      break;
    case cli::color_e::green:
      std::cout << "green: " << count;
      break;
    case cli::color_e::red:
      std::cout << "red: " << count;
      break;
    case cli::color_e::blue:
      std::cout << "blue: " << count;
      break;
  }
}

int main(int argc, char* argv[]) {
  cli::error_code ec;
  cli::options o = cli::parse(argc, argv, ec);
  if (ec) {
    cli::print_err(ec);
    return -1;
  }
  your_logic(o.color, o.count);
  return 0;
}

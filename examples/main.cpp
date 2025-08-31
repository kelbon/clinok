#include <iostream>
#include <string_view>

#include "point.hpp"  //include your type BEFORE #include <clinok/cli_interface.hpp>

#define program_options_file "../examples/program_options.def"
#include <clinok/cli_interface.hpp>

namespace clinok {

// override
template <>
struct descriptor_field<cli::timeout_o> : default_descriptor_field<cli::timeout_o> {
  constexpr static std::string_view placeholder() {
    return "<seconds>";
  }
};

template <>
struct descriptor_field<cli::log_level_o> : default_descriptor_field<cli::log_level_o> {
  consteval static std::string_view name() {
    return "log-level";
  }
};

}  // namespace clinok

void run_program(cli::options opt) {
  if (opt.version) {
    std::cout << "cli-example 1.0.0\n";
    return;
  }

  std::cout << std::boolalpha;

  std::cout << "User: " << opt.user << "\n";
  std::cout << "Output file: " << opt.output << "\n";
  std::cout << "Verbose: " << opt.verbose << "\n";
  std::cout << "Debug: " << opt.debug << "\n";
  std::cout << "Level: " << opt.level << "\n";
  std::cout << "Mode: " << opt.mode << "\n";
  std::cout << "Color: " << to_string_view(opt.color) << "\n";
  std::cout << "Retries: " << opt.retries << "\n";
  std::cout << "Timeout: " << opt.timeout << "sec \n";
  std::cout << "Point: " << opt.point << "\n";
  std::cout << "Log level: " << to_string_view(opt.log_level) << "\n";
}

int main(int argc, char* argv[]) {
  cli::error_code ec;
  cli::options opt = cli::parse(argc, argv, ec);
  if (ec) {
    cli::print_err(ec);
    return -1;
  }

  run_program(opt);
  return 0;
}

#pragma once

#include <string_view>
#include <span>
#include <cassert>
#include <string>
#include <limits>
#include <cstdint>

namespace clinok {

double levenshtein_distance(std::string_view a, std::string_view b);

// takes into account the distance on the qwerty keyboard
double damerau_levenshtein_distance(std::string_view a, std::string_view b);

struct best_match_result_t {
  std::string found;
  double diff = std::numeric_limits<double>::max();
};

// returns the most similar string available
// 'possibles' should be range of string_view convertible values
[[nodiscard]] best_match_result_t best_match_str(std::string_view str, auto&& possibles) {
  best_match_result_t res;

  // min_element with storing 'diff'
  for (std::string_view alt : possibles) {
    double d = damerau_levenshtein_distance(str, alt);
    if (d < res.diff) {
      res.diff = d;
      res.found = alt;
    }
  }

  return res;
}

using arg = const char*;

using args_t = std::span<const arg>;

// context of current parsing
struct context {
  std::string typed;          // what user typed (includeing -- or -)
  std::string resolved_name;  // resolved alias or just name
};

enum struct errc {
  ok,
  argument_missing,   // option reqires arg and it is missing
  arg_parsing_error,  // from_cli(string_view, option&) returns 'false'
  invalid_argument,   // argument is presented, but its invalid (not in enum or not bool etc)
  unknown_option,     // unknown option parsed
  impossible_enum_value,
  not_a_number,                 // parse int argument impossible
  required_option_not_present,  // option without default value not present in arguments
};

std::string_view errc2str(errc) noexcept;

struct error_code {
  errc what = errc::ok;
  context ctx;

  constexpr void set_error(errc ec, context ctx_) noexcept {
    what = ec;
    ctx = std::move(ctx_);
  }
  constexpr void clear() noexcept {
    what = errc::ok;
    ctx = {};
  }
  constexpr explicit operator bool() const noexcept {
    return what != errc::ok;
  }
};

// assumes first arg as program name
constexpr inline args_t args_range(int argc, const arg* argv) noexcept {
  assert(argc >= 0);
  if (argc == 0)
    return args_t{};
  return args_t(argv + 1, argv + argc);
}

template <typename...>
struct typelist {};

namespace noexport {
struct null_option {};
}  // namespace noexport

errc from_cli(std::string_view raw_arg, std::string_view& s) noexcept;
errc from_cli(std::string_view raw_arg, std::int_least64_t& s) noexcept;
errc from_cli(std::string_view raw_arg, bool& b) noexcept;

}  // namespace clinok

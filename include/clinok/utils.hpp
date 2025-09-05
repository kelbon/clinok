#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <span>
#include <string_view>

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
  option_missing,    // invalid - or -- without option name
  argument_missing,  // option reqires arg and it is missing
  invalid_argument,  // argument is presented, but its invalid (not in enum or not bool etc)
  unknown_option,    // unknown option parsed
  // if arg without - or -- passed and ALLOW_ADDITIONAL_ARGS not present in declarations file
  disallowed_free_arg,
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

template <typename, typename... Options>
using all_options_impl = clinok::typelist<Options...>;

consteval std::size_t count_enum_entities(std::string_view values) {
  if (std::count(values.begin(), values.end(), '=') > 1)
    throw "Incorrect STRING_ENUM";
  if (values.empty())
    throw "Incorrect STRING_ENUM without values";
  return std::count(values.begin(), values.end(), ',') + 1;
}

constexpr std::string_view trim_ws(std::string_view str) noexcept {
  auto pos = str.find_first_not_of(" \t\n\r");
  auto lastpos = str.find_last_not_of(" \t\n\r");
  if (pos == str.npos)
    return "";
  return std::string_view(str.begin() + pos, str.begin() + lastpos + 1);
}

constexpr std::string_view* split_str_by_comma(std::string_view values, std::string_view* out) {
  values = trim_ws(values);
  if (values.empty())
    return out;
  for (;;) {
    auto pos = values.find(',');
    if (pos == values.npos) {
      values = trim_ws(values);
      if (values.empty())
        throw "invalid syntax: trailing comma";
      *out = values;
      ++out;
      break;
    }
    *out = trim_ws(values.substr(0, pos));
    ++out;

    values.remove_prefix(pos + 1);
  }
  return out;
}

template <std::size_t N>
consteval std::array<std::string_view, N> split_enum(std::string_view values) {
  std::array<std::string_view, N> result;
  if (count_enum_entities(values) != N)
    throw "Something got wrong";
  split_str_by_comma(values, result.data());
  return result;
}

template <typename T, std::size_t N>
constexpr auto drop_dummy(const std::array<T, N>& from) {
  constexpr std::size_t K = N > 0 ? N - 1 : 0;
  std::array<T, K> res;
  std::copy(from.begin() + N - K, from.end(), res.begin());
  return res;
}

}  // namespace noexport

// assumes first arg as program name, second arg as subprogram name
// like 'git status' and selects by name.
// `programname` - main program, is case git status/git branch programname == git
// `names` - names of subprogram to select
// `out` - stream for printing program usage on error
//
// returns index of selected name, also changes argc/argv to be passed into 'parse' as if
// 'parse' was for subprogram
// returns < 0 on failure
[[nodiscard]] inline int select_subprogram(int& argc, char**& argv, std::string_view programname,
                                           std::initializer_list<std::string_view> names,
                                           std::ostream& out = std::cout) {
  auto print_usage_and_exit = [&]() {
    out << "Usage: " << programname << " <subprogram>\n";
    out << "valid subprograms list:\n";
    for (std::string_view s : names)
      out << s << std::endl;
  };
  if (argc < 2) {
    print_usage_and_exit();
    return -1;
  }
  std::string_view n = argv[1];
  auto it = std::find(names.begin(), names.end(), n);
  if (it == names.end()) {
    print_usage_and_exit();  // no such names
    return -2;
  }
  if (std::find(std::next(it), names.end(), n) != names.end()) {
    print_usage_and_exit();  // several names match
    return -3;
  }
  // cut program name arg
  --argc;
  ++argv;
  return std::distance(names.begin(), it);
}

}  // namespace clinok

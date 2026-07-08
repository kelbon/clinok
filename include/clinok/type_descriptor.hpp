#pragma once

#include <charconv>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <version>

#include "clinok/utils.hpp"

#if __has_include(<format>) && defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
  #define CLINOK_HAS_FORMAT 1
#else
  #define CLINOK_HAS_FORMAT 0
#endif

#if CLINOK_HAS_FORMAT
  #include <format>
#endif

namespace clinok {

namespace noexport {

// неявно полагается на то, что оператор<< даёт то же самое что и format
template <typename T>
std::string to_string_helper(const T& value) {
#if CLINOK_HAS_FORMAT
  if constexpr (requires { std::formatter<T>{}; }) {
    return std::format("{}", value);
  } else {
    std::ostringstream oss;
    oss << value;
    return oss.str();
  }
#else
  std::ostringstream oss;
  oss << value;
  return oss.str();
#endif
}

}  // namespace noexport

template <typename T>
struct type_descriptor {
  // required
  // used for help message, printed as expected value, e.g. <string>
  // std::string_view placeholder()

  // required
  // used to parse T, returns iterator to first unparsed arg
  // parse_option(b, e, T&, errc&) } -> args_t::iterator;

  // optional
  // used for help message, e.g. for enum "[red, green, blue]"
  // possible_values_description()
};

template <std::integral T>
struct type_descriptor<T> {
  static constexpr std::string_view placeholder() noexcept {
    if constexpr (std::unsigned_integral<T>) {
      return "<unsigned int>";
    } else
      return "<int>";
  }

  static args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, T& out, errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    std::string_view raw_arg = std::string_view(*it);
    er = [&] {
      auto [_, ec] = std::from_chars(raw_arg.data(), raw_arg.data() + raw_arg.size(), out);
      if (ec != std::errc{})
        return errc::not_a_number;
      return errc::ok;
    }();
    return ++it;
  }
};

template <>
struct type_descriptor<bool> {
  static constexpr std::string_view placeholder() {
    return "<bool>";
  }

  static constexpr args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, bool& out,
                                                 errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    std::string_view raw_arg = std::string_view(*it);

    if (raw_arg == "on" || raw_arg == "ON" || raw_arg == "1" || raw_arg == "yes" || raw_arg == "YES" ||
        raw_arg == "true") {
      out = true;
      er = errc::ok;
    } else if (raw_arg == "off" || raw_arg == "OFF" || raw_arg == "0" || raw_arg == "no" || raw_arg == "NO" ||
               raw_arg == "false") {
      out = false;
      er = errc::ok;
    } else {
      er = errc::invalid_argument;
    }

    return ++it;
  }
};

template <>
struct type_descriptor<std::string> {
  static constexpr std::string_view placeholder() {
    return "<string>";
  }

  static args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, std::string& out,
                                       errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    out = std::string{*it};
    return ++it;
  }
};

template <>
struct type_descriptor<std::string_view> {
  static constexpr std::string_view placeholder() {
    return "<string>";
  }

  static args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, std::string_view& out,
                                       errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    out = std::string_view{*it};
    return ++it;
  }
};

template <>
struct type_descriptor<void> {
  static constexpr std::string_view placeholder() {
    return "";
  }
  static constexpr args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, bool& out,
                                                 errc& er) {
    out = true;
    return it;
  }
};

// при указании типом в OPTION множественные указания опции
// складываются в один и тот же список (в порядке их появления в списке аргументов)
template <typename T>
struct multiarg {
  std::vector<T> args;
};

template <typename T>
struct type_descriptor<multiarg<T>> {
  static constexpr std::string_view placeholder() {
    return type_descriptor<T>::placeholder();
  }
  static constexpr args_t::iterator parse_option(args_t::iterator it, args_t::iterator end, multiarg<T>& out,
                                                 errc& er) {
    return type_descriptor<T>::parse_option(it, end, out.args.emplace_back(), er);
  }
};

}  // namespace clinok

namespace std {

#if CLINOK_HAS_FORMAT

template <typename T>
struct formatter<::clinok::multiarg<T>> : std::formatter<std::string_view> {
  auto format(const ::clinok::multiarg<T>& v, auto& ctx) const -> decltype(ctx.out()) {
    auto out = std::format_to(ctx.out(), "[");
    if (!v.args.empty()) {
      auto b = v.args.begin();
      auto e = v.args.end();
      goto in;
      for (; b != e; ++b) {
        out = std::format_to(out, ", ");
      in:
        out = std::format_to(out, "{}", ::clinok::noexport::to_string_helper(*b));
      }
    }
    out = std::format_to(out, "]");
    return out;
  }
};

#endif

template <typename T>
std::ostream& operator<<(std::ostream& out, const ::clinok::multiarg<T>& v) {
  out << '[';
  if (!v.args.empty()) {
    auto b = v.args.begin();
    auto e = v.args.end();
    goto in;
    for (; b != e; ++b) {
      out << ", ";
    in:
      out << ::clinok::noexport::to_string_helper(*b);
    }
  }
  out << ']';
  return out;
}

}  // namespace std

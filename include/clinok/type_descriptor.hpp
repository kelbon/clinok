#pragma once

#include <charconv>
#include <string>
#include <string_view>

#include "clinok/utils.hpp"

namespace clinok {

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

}  // namespace clinok

#pragma once

#include <charconv>
#include <numeric>
#include <string>
#include <string_view>

#include "clinok/utils.hpp"

namespace clinok {

struct default_descriptor_type {
  constexpr static std::string_view placeholder() {
    return "";
  }

  static constexpr bool has_default() {
    return false;
  }

  constexpr static std::string_view default_value_desc() {
    return "";
  }

  static void set_default_value(auto&) {
  }

  constexpr static bool has_possible_values() {
    return false;
  }

  constexpr static std::string_view possible_values_desc() {
    return "";
  }
};

template <typename T>
struct descriptor_type : default_descriptor_type {
  // static args_t::iterator parse(args_t::iterator it, args_t::iterator end, T& out, errc& er);
};

template <std::integral T>
struct descriptor_type<T> : default_descriptor_type {
  constexpr static std::string_view placeholder() noexcept {
    if constexpr (std::unsigned_integral<T>) {
      return "<unsigned int>";
    } else
      return "<int>";
  }

  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, T& out, errc& er) {
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
struct descriptor_type<bool> : default_descriptor_type {
  constexpr static std::string_view placeholder() {
    return "<bool>";
  }

  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, bool& out, errc& er) {
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
struct descriptor_type<std::string> : default_descriptor_type {
  constexpr static std::string_view placeholder() {
    return "<string>";
  }

  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, std::string& out, errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    out = std::string{*it};
    return ++it;
  }
};

template <>
struct descriptor_type<std::string_view> : default_descriptor_type {
  constexpr static std::string_view placeholder() {
    return "<string>";
  }

  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, std::string_view& out, errc& er) {
    if (it == end) {
      er = errc::argument_missing;
      return it;
    }
    out = std::string_view{*it};
    return ++it;
  }
};

template <>
struct descriptor_type<void> : default_descriptor_type {
  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, bool& out, errc& er) {
    out = true;
    return it;
  }
};

}  // namespace clinok

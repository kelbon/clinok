
// NO pragma once / include guard.
// this header may be included many times in different .cpp files
// to generate different parsers and options structs

#include "clinok/cli.hpp"
#include "clinok/descriptor_field.hpp"
#ifndef CLINOK_NAMESPACE_NAME
  #define CLINOK_NAMESPACE_NAME cli
#endif

// file with description of program options, required format for this file described in README
// ( https://github.com/kelbon/clinok )
#ifndef program_options_file
  #error program_options_file must be defined
#endif

#include <array>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <iostream>
#include <type_traits>
#include <vector>
#include <span>
#include <string_view>

#include <clinok/descriptor_field.hpp>
#include <clinok/descriptor_type.hpp>
#include <clinok/utils.hpp>

namespace CLINOK_NAMESPACE_NAME {

using namespace ::clinok;

#define DD_CLI_STR
#define DD_CLI_STRdefault(...) "default: " #__VA_ARGS__ ", "

#define OPTION(TYPE, NAME, DESCRIPTION, ...)                             \
  struct NAME##_o {                                                      \
    using value_type = TYPE;                                             \
    static consteval std::string_view name() {                           \
      return #NAME;                                                      \
    }                                                                    \
    static consteval std::string_view description() {                    \
      (void)"" DESCRIPTION; /*validate description is a string literal*/ \
      return DD_CLI_STR##__VA_ARGS__ DESCRIPTION;                        \
    }                                                                    \
    static constexpr auto& get(auto& x) {                                \
      return x.NAME;                                                     \
    }                                                                    \
    static consteval bool has_default() noexcept {                       \
      return std::string_view(#__VA_ARGS__).starts_with("default");      \
    }                                                                    \
  };
#define TAG(NAME, DESCRIPTION)                                           \
  struct NAME##_o {                                                      \
    using value_type = void;                                             \
    static consteval std::string_view name() {                           \
      return #NAME;                                                      \
    }                                                                    \
    static consteval std::string_view description() {                    \
      (void)"" DESCRIPTION; /*validate description is a string literal*/ \
      return DESCRIPTION;                                                \
    }                                                                    \
    static constexpr auto& get(auto& x) {                                \
      return x.NAME;                                                     \
    }                                                                    \
    static constexpr bool has_default() noexcept {                       \
      return true;                                                       \
    }                                                                    \
  };

#define ENUM(NAME, DESCRIPTION, ...)                                                                       \
  struct NAME##_o {                                                                                        \
    using value_type = std::conditional_t<std::is_integral_v<decltype((__VA_ARGS__))>, std::int_least64_t, \
                                          std::string_view>;                                               \
    static constexpr auto values = std::to_array<value_type>({__VA_ARGS__});                               \
                                                                                                           \
    static consteval std::string_view name() {                                                             \
      return #NAME;                                                                                        \
    }                                                                                                      \
                                                                                                           \
    static consteval std::string_view description() {                                                      \
      (void)"" DESCRIPTION; /*validate description is a string literal*/                                   \
      return DESCRIPTION;                                                                                  \
    }                                                                                                      \
                                                                                                           \
    static constexpr auto& get(auto& x) {                                                                  \
      return x.NAME;                                                                                       \
    }                                                                                                      \
                                                                                                           \
    static constexpr bool has_default() noexcept {                                                         \
      return true;                                                                                         \
    }                                                                                                      \
  };

#define STRING_ENUM(NAME, DESCRIPTION, ...)                                           \
  enum struct NAME##_e{__VA_ARGS__};                                                  \
  struct NAME##_o {                                                                   \
    using value_type = NAME##_e;                                                      \
    using enum NAME##_e;                                                              \
                                                                                      \
    static constexpr auto values = std::to_array({__VA_ARGS__});                      \
    static constexpr auto values_count = noexport::count_enum_entities(#__VA_ARGS__); \
                                                                                      \
    static constexpr std::array<std::string_view, values_count> names =               \
        noexport::split_enum<values_count>(#__VA_ARGS__);                             \
                                                                                      \
    static consteval std::string_view name() {                                        \
      return #NAME;                                                                   \
    }                                                                                 \
                                                                                      \
    static consteval std::string_view description() {                                 \
      (void)"" DESCRIPTION; /*validate description is a string literal*/              \
      return DESCRIPTION;                                                             \
    }                                                                                 \
                                                                                      \
    static constexpr auto& get(auto& x) {                                             \
      return x.NAME;                                                                  \
    }                                                                                 \
                                                                                      \
    static constexpr bool has_default() noexcept {                                    \
      return true;                                                                    \
    }                                                                                 \
  };                                                                                  \
                                                                                      \
  constexpr std::string_view to_string_view(NAME##_e e) {                             \
    auto idx = (size_t)e;                                                             \
    if (idx >= NAME##_o::names.size())                                                \
      throw std::runtime_error("Unknown enum value ");                                \
    return NAME##_o::names[idx];                                                      \
  }

#include <clinok/generate.hpp>

#undef DD_CLI_STR
#undef DD_CLI_STRdefault

}  // namespace CLINOK_NAMESPACE_NAME

namespace clinok {

#define OPTION(...)

#define ENUM(NAME, DESCRIPTION, ...)                                                                      \
  template <>                                                                                             \
  struct descriptor_field<CLINOK_NAMESPACE_NAME::NAME##_o>                                                \
      : default_descriptor_field<CLINOK_NAMESPACE_NAME::NAME##_o> {                                       \
    using O = CLINOK_NAMESPACE_NAME::NAME##_o;                                                            \
    using value_type = O::value_type;                                                                     \
    static constexpr auto values = O::values;                                                             \
                                                                                                          \
    constexpr static std::string_view placeholder() {                                                     \
      return descriptor_type<value_type>::placeholder();                                                  \
    }                                                                                                     \
                                                                                                          \
    constexpr static bool has_possible_values() {                                                         \
      return true;                                                                                        \
    }                                                                                                     \
                                                                                                          \
    static std::string possible_values_desc() {                                                           \
      auto to_string = []<typename T>(const T& value) {                                                   \
        if constexpr (std::is_same_v<T, std::string_view>) {                                              \
          return value;                                                                                   \
        } else {                                                                                          \
          return std::to_string(value);                                                                   \
        }                                                                                                 \
      };                                                                                                  \
      std::string result = "[";                                                                           \
      auto it = values.begin(), end = values.end();                                                       \
      if (it != end)                                                                                      \
        result += to_string(*it++);                                                                       \
      for (; it != end; it++) {                                                                           \
        result.append(", ").append(to_string(*it));                                                       \
      }                                                                                                   \
      result += "]";                                                                                      \
      return result;                                                                                      \
    }                                                                                                     \
                                                                                                          \
    static args_t::iterator parse(args_t::iterator it, args_t::iterator end, value_type& out, errc& er) { \
      if (it == end) {                                                                                    \
        er = errc::argument_missing;                                                                      \
        return it;                                                                                        \
      }                                                                                                   \
      it = descriptor_type<value_type>::parse(it, end, out, er);                                          \
      if (er != errc::ok || std::find(values.begin(), values.end(), out) == values.end()) {               \
        er = errc::invalid_argument;                                                                      \
      }                                                                                                   \
      return it;                                                                                          \
    }                                                                                                     \
  };

#define STRING_ENUM(NAME, DESCRIPTION, ...)                                                  \
  template <>                                                                                \
  struct descriptor_type<CLINOK_NAMESPACE_NAME::NAME##_e> : default_descriptor_type {        \
    using enum CLINOK_NAMESPACE_NAME::NAME##_e;                                              \
    using O = CLINOK_NAMESPACE_NAME::NAME##_o;                                               \
    static constexpr auto values = O::values;                                                \
    static constexpr auto names = O::names;                                                  \
                                                                                             \
    constexpr static std::string_view placeholder() {                                        \
      return "<string>";                                                                     \
    }                                                                                        \
                                                                                             \
    constexpr static bool has_possible_values() {                                            \
      return true;                                                                           \
    }                                                                                        \
                                                                                             \
    static std::string possible_values_desc() {                                              \
      std::string result = "[";                                                              \
      auto it = values.begin(), end = values.end();                                          \
      if (it != end)                                                                         \
        result += to_string_view(*it++);                                                     \
      for (; it != end; it++) {                                                              \
        result.append(", ").append(to_string_view(*it));                                     \
      }                                                                                      \
      result += "]";                                                                         \
      return result;                                                                         \
    }                                                                                        \
                                                                                             \
    static args_t::iterator parse(args_t::iterator it, args_t::iterator end,                 \
                                  CLINOK_NAMESPACE_NAME::NAME##_e& out, errc& er) {          \
      if (it == end) {                                                                       \
        er = errc::argument_missing;                                                         \
        return it;                                                                           \
      }                                                                                      \
      std::string_view raw_arg = std::string_view(*it);                                      \
      if (auto it_f = std::find(names.begin(), names.end(), raw_arg); it_f != names.end()) { \
        std::size_t i = it_f - begin(names);                                                 \
        out = values[i];                                                                     \
      } else {                                                                               \
        er = errc::invalid_argument;                                                         \
      }                                                                                      \
      return ++it;                                                                           \
    }                                                                                        \
  };

#define REQUIRED(NAME) \
  template <>          \
  struct is_required_option<CLINOK_NAMESPACE_NAME::NAME##_o> : std::true_type {};

#include <clinok/generate.hpp>
}  // namespace clinok

namespace CLINOK_NAMESPACE_NAME {

struct cli_t {
  using all_options = clinok::noexport::all_options_impl<::clinok::noexport::null_option
#define OPTION(type, name, ...) , name##_o
#define ENUM(name, ...) , name##_o
#include <clinok/generate.hpp>
                                                         >;

  struct options {
    // zero initialization if no default here, but 'parse' should return error if no value provided for option
    // without default value

#define DD_CLI = {}
#define DD_CLIdefault(...) = __VA_ARGS__
#define TAG(name, description) bool name = false;
#define BOOLEAN(name, description, ...) bool name DD_CLI##__VA_ARGS__;
#define STRING(name, description, ...) std::string_view name DD_CLI##__VA_ARGS__;
#define ENUM(name, description, ...) name##_o ::value_type name = name##_o::values[0];
#define STRING_ENUM(name, description, ...) name##_o ::value_type name = name##_o::values[0];
#define INTEGER(name, description, ...) std::int_least64_t name DD_CLI##__VA_ARGS__;
#define OPTION(type, name, description, ...) type name DD_CLI##__VA_ARGS__;
#define ALLOW_ADDITIONAL_ARGS std::vector<std::string_view> additional_args;
#include <clinok/generate.hpp>

#undef DD_CLI
#undef DD_CLIdefault

    constexpr auto print_to(auto out) {
      for_each_option([&](auto o) {
        out(o.name()), out(" = ");
        constexpr bool int_like = requires { o.get(*this) == 42; };
        int_like ? void() : (void)out('\"');
        out(o.get(*this));
        int_like ? void() : (void)out('\"');
        out('\n');
      });
      return std::move(out);
    }
  };

  // contains 'bool' fields for checking if option present during parse
  struct presented_options {
    // zero initialization if no default here, but 'parse' should return error if no value provided for option
    // without default value

#define OPTION(type, name, ...) bool name = false;
#define ENUM(name, ...) bool name = false;

#include <clinok/generate.hpp>
  };

  static constexpr auto aliases = noexport::drop_dummy(std::to_array({
      // dummy value for handling case when there are no aliases
      std::pair<std::string_view, std::string_view>("", ""),
#define ALIAS(a, b) std::pair<std::string_view, std::string_view>{#a, #b},
#define OPTION(...)
#include <clinok/generate.hpp>
  }));

  static constexpr bool allow_additional_args = 0
#define ALLOW_ADDITIONAL_ARGS +1
#include <clinok/generate.hpp>
      ;
};

using options = cli_t::options;
using presented_options = cli_t::presented_options;
using all_options = cli_t::all_options;
inline constexpr bool allow_additional_args = cli_t::allow_additional_args;
constexpr auto aliases = cli_t::aliases;

template <CLI_like CLI = cli_t, typename Out>
inline Out print_help_message_to(Out out) {
  return clinok::print_help_message_to<CLI>(out);
}

template <CLI_like CLI = cli_t>
constexpr typename CLI::options parse(args_t args, error_code& ec) noexcept {
  return clinok::parse<CLI>(args, ec);
}

template <CLI_like CLI = cli_t>
// assumes first arg as program name
inline typename CLI::options parse(int argc, char* argv[], error_code& ec) noexcept {
  return clinok::parse<CLI>(argc, argv, ec);
}

template <CLI_like CLI = cli_t, typename Out>
constexpr Out print_err_to(const error_code& err, Out out) {
  return clinok::print_err_to<CLI>(err, out);
}

template <clinok::CLI_like CLI = cli_t>
void print_err(const error_code& err, std::ostream& out = std::cerr) {
  return clinok::print_err<CLI>(err, out);
}

template <clinok::CLI_like CLI = cli_t>
inline typename CLI::options parse_or_exit(int argc, char* argv[]) {
  return clinok::parse_or_exit<CLI>(argc, argv);
}

}  // namespace CLINOK_NAMESPACE_NAME

#undef program_options_file
#undef CLINOK_NAMESPACE_NAME


// NO pragma once / include guard.
// this header may be included many times in different .cpp files
// to generate different parsers and options structs

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

#include <clinok/type_descriptor.hpp>
#include <clinok/utils.hpp>
#include <clinok/cli.hpp>

namespace CLINOK_NAMESPACE_NAME {

using namespace ::clinok;

// decalre enums first

#define DECLARE_STRING_ENUM(NAME, ...)                                                \
  enum struct NAME { __VA_ARGS__ };                                                   \
  struct NAME##_enum_description {                                                    \
    using enum NAME;                                                                  \
                                                                                      \
    static constexpr auto values = std::to_array({__VA_ARGS__});                      \
    static constexpr auto values_count = noexport::count_enum_entities(#__VA_ARGS__); \
                                                                                      \
    static constexpr std::array<std::string_view, values_count> names =               \
        noexport::split_enum<values_count>(#__VA_ARGS__);                             \
  };                                                                                  \
                                                                                      \
  constexpr std::string_view e2str(NAME e) {                                          \
    size_t i = static_cast<size_t>(e);                                                \
    if (i >= NAME##_enum_description::names.size())                                   \
      throw std::runtime_error("Unknown enum value");                                 \
    return NAME##_enum_description::names[i];                                         \
  }

#include <clinok/generate.hpp>

#define DD_CLI_STR
#define DD_CLI_STRdefault(...)                         \
  static ::clinok::arg default_strs[] = {__VA_ARGS__}; \
  static_assert(sizeof(#__VA_ARGS__) > 1);             \
  return ::clinok::args_t(default_strs)

#define OPTION(TYPE, NAME, DESCRIPTION, ...)                        \
  struct NAME##_o {                                                 \
    using logic_type = TYPE;                                        \
    using cpp_type = logic_type;                                    \
    static consteval std::string_view name() {                      \
      return #NAME;                                                 \
    }                                                               \
    static consteval std::string_view description() {               \
      return DESCRIPTION;                                           \
    }                                                               \
    static constexpr auto& get(auto& x) {                           \
      return x.NAME;                                                \
    }                                                               \
    static consteval bool has_default() noexcept {                  \
      return std::string_view(#__VA_ARGS__).starts_with("default"); \
    }                                                               \
    static auto default_args() noexcept {                           \
      DD_CLI_STR##__VA_ARGS__;                                      \
    }                                                               \
  };

// similar to OPTION, but forbids default(...) and has_default == true
#define TAG(NAME, DESCRIPTION)                        \
  struct NAME##_o {                                   \
    using logic_type = void;                          \
    using cpp_type = bool;                            \
    static consteval std::string_view name() {        \
      return #NAME;                                   \
    }                                                 \
    static consteval std::string_view description() { \
      return DESCRIPTION;                             \
    }                                                 \
    static constexpr auto& get(auto& x) {             \
      return x.NAME;                                  \
    }                                                 \
    static constexpr bool has_default() noexcept {    \
      return true;                                    \
    }                                                 \
  };

#include <clinok/generate.hpp>

#undef DD_CLI_STR
#undef DD_CLI_STRdefault

}  // namespace CLINOK_NAMESPACE_NAME

namespace clinok {

#define OPTION(...)

#define DECLARE_STRING_ENUM(NAME, ...)                                                                \
  template <>                                                                                         \
  struct type_descriptor<::CLINOK_NAMESPACE_NAME::NAME> {                                             \
    using E = ::CLINOK_NAMESPACE_NAME::NAME##_enum_description;                                       \
                                                                                                      \
    constexpr static std::string_view placeholder() {                                                 \
      return "<string>";                                                                              \
    }                                                                                                 \
                                                                                                      \
    static std::string possible_values_description() {                                                \
      return noexport::join_comma(std::span<const ::CLINOK_NAMESPACE_NAME::NAME>(E::values),          \
                                  [](auto x) { return e2str(x); });                                   \
    }                                                                                                 \
                                                                                                      \
    static args_t::iterator parse_option(args_t::iterator it, args_t::iterator end,                   \
                                         ::CLINOK_NAMESPACE_NAME::NAME& out, errc& er) {              \
      if (it == end) {                                                                                \
        er = errc::argument_missing;                                                                  \
        return it;                                                                                    \
      }                                                                                               \
      std::string_view raw_arg = std::string_view(*it);                                               \
      if (auto it_f = std::find(E::names.begin(), E::names.end(), raw_arg); it_f != E::names.end()) { \
        std::size_t i = it_f - begin(E::names);                                                       \
        out = E::values[i];                                                                           \
      } else {                                                                                        \
        er = errc::invalid_argument;                                                                  \
      }                                                                                               \
      return ++it;                                                                                    \
    }                                                                                                 \
  };

#include <clinok/generate.hpp>
}  // namespace clinok

namespace CLINOK_NAMESPACE_NAME {

struct options {
  // zero initialization

#define TAG(name, description) bool name = false;
#define OPTION(type, name, description, ...) type name = type{};
#define ALLOW_ADDITIONAL_ARGS std::vector<std::string_view> additional_args;
#include <clinok/generate.hpp>
};

struct cli_t {
  using all_options = clinok::noexport::biteoff_first_arg<::clinok::noexport::null_option
#define OPTION(type, name, ...) , name##_o
#include <clinok/generate.hpp>
                                                          >;

  using options = ::CLINOK_NAMESPACE_NAME::options;
  // contains 'bool' fields for checking if option present during parse
  struct presented_options {
#define OPTION(type, name, ...) size_t name = 0;

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

template <typename Out, CLI_like CLI = cli_t>
inline Out print_help_message_to(Out out) {
  return clinok::print_help_message_to<CLI>(out);
}

template <CLI_like CLI = cli_t>
constexpr options parse(args_t args, error_code& ec) noexcept {
  return clinok::parse<CLI>(args, ec);
}

// assumes first arg as program name
template <CLI_like CLI = cli_t>
inline options parse(int argc, char* argv[], error_code& ec) noexcept {
  return clinok::parse<CLI>(argc, argv, ec);
}

template <typename Out, CLI_like CLI = cli_t>
constexpr Out print_err_to(const error_code& err, Out out) {
  return clinok::print_err_to<CLI>(err, out);
}

template <clinok::CLI_like CLI = cli_t>
void print_err(const error_code& err, std::ostream& out = std::cerr) {
  return clinok::print_err<CLI>(err, out);
}

template <clinok::CLI_like CLI = cli_t>
inline options parse_or_exit(int argc, char* argv[]) {
  return clinok::parse_or_exit<CLI>(argc, argv);
}

}  // namespace CLINOK_NAMESPACE_NAME

namespace clinok {

#define RENAME(OLDNAME, NEWNAME) \
  template <>                    \
  constexpr inline std::string_view name_of<::CLINOK_NAMESPACE_NAME::OLDNAME##_o> = NEWNAME;

#define SET_LOGIC_TYPE(NAME, ...) \
  template <>                     \
  struct logic_type<::CLINOK_NAMESPACE_NAME::NAME##_o> : ::std::type_identity<__VA_ARGS__> {};

#define SET_PLACEHOLDER(NAME, ...) \
  template <>                      \
  constexpr inline std::string_view placeholder_of<::CLINOK_NAMESPACE_NAME::NAME##_o> = __VA_ARGS__;

#include <clinok/generate.hpp>

}  // namespace clinok

#undef program_options_file
#undef CLINOK_NAMESPACE_NAME

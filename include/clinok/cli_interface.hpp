
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
    static consteval std::string_view name() {                                                             \
      return #NAME;                                                                                        \
    }                                                                                                      \
    static consteval std::string_view description() {                                                      \
      (void)"" DESCRIPTION; /*validate description is a string literal*/                                   \
      return "one of: [" #__VA_ARGS__ "] " DESCRIPTION;                                                    \
    }                                                                                                      \
    static constexpr auto values = std::to_array({__VA_ARGS__});                                           \
    using value_type = std::conditional_t<std::is_integral_v<decltype((__VA_ARGS__))>, std::int_least64_t, \
                                          std::string_view>;                                               \
    static constexpr auto& get(auto& x) {                                                                  \
      return x.NAME;                                                                                       \
    }                                                                                                      \
    static constexpr bool has_default() noexcept {                                                         \
      return true;                                                                                         \
    }                                                                                                      \
  };

#define STRING_ENUM(NAME, DESCRIPTION, ...)                                           \
  enum struct NAME##_e{__VA_ARGS__};                                                  \
  struct NAME##_o {                                                                   \
    using value_type = NAME##_e;                                                      \
    using enum NAME##_e;                                                              \
    static consteval std::string_view name() {                                        \
      return #NAME;                                                                   \
    }                                                                                 \
    static consteval std::string_view description() {                                 \
      (void)"" DESCRIPTION; /*validate description is a string literal*/              \
      return "one of: [" #__VA_ARGS__ "] " DESCRIPTION;                               \
    }                                                                                 \
    static constexpr auto values = std::to_array({__VA_ARGS__});                      \
    static constexpr auto values_count = noexport::count_enum_entities(#__VA_ARGS__); \
                                                                                      \
    static constexpr std::array<std::string_view, values_count> names =               \
        noexport::split_enum<values_count>(#__VA_ARGS__);                             \
    static constexpr auto& get(auto& x) {                                             \
      return x.NAME;                                                                  \
    }                                                                                 \
    static constexpr bool has_default() noexcept {                                    \
      return true;                                                                    \
    }                                                                                 \
  };

#include <clinok/generate.hpp>

namespace noexport {

using all_options = clinok::typelist<::clinok::noexport::null_option
#define OPTION(type, name, ...) , name##_o
#define ENUM(name, ...) , name##_o
#include <clinok/generate.hpp>
                                     >;

}  // namespace noexport

// passes empty option description object to 'foo'
constexpr void for_each_option(auto foo) {
  [&]<typename... Options>(clinok::typelist<::clinok::noexport::null_option, Options...>) {
    (foo(Options{}), ...);
  }(noexport::all_options{});
}

// passes all options as empty option description objects to 'foo'
constexpr decltype(auto) apply_to_options(auto foo) {
  return [&]<typename... Options>(
             clinok::typelist<::clinok::noexport::null_option, Options...>) -> decltype(auto) {
    return foo(Options{}...);
  }(noexport::all_options{});
}

struct options {
  // zero initialization if no default here, but 'parse' should return error if no value provided for option
  // without default value

#define DD_CLI = {}
#define DD_CLIdefault(...) = __VA_ARGS__
#define TAG(name, description) bool name = false;
#define BOOLEAN(name, description, ...) bool name DD_CLI##__VA_ARGS__;
#define STRING(name, description, ...) std::string_view name DD_CLI##__VA_ARGS__;
#define ENUM(name, description, ...) name##_o ::value_type name = name##_o ::values[0];
#define STRING_ENUM(name, description, ...) name##_o ::value_type name = name##_o::values[0];
#define INTEGER(name, description, ...) std::int_least64_t name DD_CLI##__VA_ARGS__;

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
#define STRING_ENUM(name, ...) bool name = false;

#include <clinok/generate.hpp>
};

consteval bool has_option(std::string_view name) {
  return apply_to_options([&](auto... opts) { return ((opts.name() == name) || ...); });
}

// accepts function which acceps std::string_view to out
template <typename Out>
inline Out print_help_message_to(Out out) noexcept {
  out("\n");
  auto option_arg_str = []<typename O>(O) {
    if (std::is_same_v<typename O::value_type, std::string_view>)
      return "<string>";
    else if (std::is_same_v<typename O::value_type, bool>)
      return "<bool>";
    else if (std::is_integral_v<typename O::value_type>)
      return "<int>";
    else
      return "";
  };
  auto option_string_len = [&](auto o) -> size_t {
    return sizeof("--") + o.name().size() + std::strlen(option_arg_str(o));
  };
  std::size_t largest_help_string =
      apply_to_options([&](auto... opts) { return std::max({size_t(0), option_string_len(opts)...}); });
  for_each_option([&](auto o) {
    out(" --"), out(o.name()), out(' '), out(option_arg_str(o));
    const int whitespace_count = 2 + largest_help_string - option_string_len(o);
    for (int i = 0; i < whitespace_count; ++i)
      out(' ');
    out(o.description()), out('\n');
  });
#define ALIAS(a, b) out(" -"), out(#a), out(" is an alias to "), out(#b), out('\n');
#define OPTION(...)
#include <clinok/generate.hpp>
  return std::move(out);
}

#undef DD_CLI_STR
#undef DD_CLI_STRdefault

// assumes first arg as program name
constexpr options parse(args_t args, error_code& ec) noexcept {
  options o;
  presented_options po;

  auto set_error = [&](std::string_view typed, errc what, std::string_view resolved) {
    ec.set_error(what, context{std::string(typed), std::string(resolved)});
  };
  auto try_parse = [&](auto it, auto& option_value) -> errc {
    if (it == args.end())
      return errc::argument_missing;
    return from_cli(std::string_view(*it), option_value);
  };
  for (auto it = args.begin(); it != args.end(); ++it) {
    std::string_view typed = *it;
    std::string_view s = *it;
    if (s.starts_with("--")) {
      s.remove_prefix(2);
    } else if (s.starts_with('-') && s.size() > 1) {
      s.remove_prefix(1);
      constexpr auto aliases = std::to_array({
          // dummy value for handling case when there are no aliases
          std::pair<std::string_view, std::string_view>("", ""),
#define ALIAS(a, b) std::pair<std::string_view, std::string_view>{#a, #b},
#define OPTION(...)
#include <clinok/generate.hpp>
      });
      if (auto it2 = std::find_if(std::begin(aliases) + 1 /*dummy*/, std::end(aliases),
                                  [&](auto& x) { return x.first == s; });
          it2 != std::end(aliases)) {
        s = it2->second;
      } else {
        set_error(typed, errc::unknown_option, std::string(s));
        return o;
      }
    }
#define TAG(name, ...)                \
  if (s == std::string_view(#name)) { \
    o.name = true;                    \
    po.name = true;                   \
    continue;                         \
  }
#define STRING(name, ...)                                    \
  if (s == std::string_view(#name)) {                        \
    po.name = true;                                          \
    if (errc ec = try_parse(++it, o.name); ec != errc::ok) { \
      set_error(typed, ec, std::string(s));                  \
      return o;                                              \
    }                                                        \
    continue;                                                \
  }
#define ENUM(name, ...)                                                 \
  if (s == std::string_view(#name)) {                                   \
    po.name = true;                                                     \
    if (errc ec = try_parse(++it, o.name); ec != errc::ok) {            \
      set_error(typed, ec, std::string(s));                             \
      return o;                                                         \
    }                                                                   \
    constexpr auto& values = name##_o ::values;                         \
    if (std::find(begin(values), end(values), o.name) == end(values)) { \
      set_error(typed, errc::impossible_enum_value, std::string(s));    \
      return o;                                                         \
    }                                                                   \
    continue;                                                           \
  }
#define STRING_ENUM(name, ...)                                                    \
  if (s == std::string_view(#name)) {                                             \
    po.name = true;                                                               \
    std::string_view value;                                                       \
    if (errc ec = try_parse(++it, value); ec != errc::ok) {                       \
      set_error(typed, ec, std::string(s));                                       \
      return o;                                                                   \
    }                                                                             \
    constexpr auto& names = name##_o ::names;                                     \
    constexpr auto& values = name##_o ::values;                                   \
    if (auto it = std::find(begin(names), end(names), value); it != end(names)) { \
      std::size_t i = it - begin(names);                                          \
      o.name = values[i];                                                         \
    } else {                                                                      \
      set_error(typed, errc::impossible_enum_value, std::string(s));              \
      return o;                                                                   \
    }                                                                             \
    continue;                                                                     \
  }
#define INTEGER(...) STRING(__VA_ARGS__)
#define BOOLEAN(...) STRING(__VA_ARGS__)
#include <clinok/generate.hpp>

    // if 'continue' not reached
    set_error(typed, errc::unknown_option, std::string(s));
    return o;
  }  // parse loop end

  for_each_option([&](auto o) {
    // is required and not present in arg list
    if (!o.has_default() && !o.get(po)) {
      set_error(o.name(), errc::required_option_not_present, o.name());
    }
  });
  return o;
}

// assumes first arg as program name
inline options parse(int argc, char* argv[], error_code& ec) noexcept {
  assert(argc >= 0);
  options o = parse(args_range(argc, argv), ec);
  if (o.help) {
    print_help_message_to([](auto s) { std::cout << s; });
    std::flush(std::cout);
    std::exit(0);
  }
  return o;
}

template <typename Out>
constexpr Out print_err_to(const error_code& err, Out out) {
  if (err.what == errc::ok)
    return std::move(out);
  if (err.what == errc::required_option_not_present) {
    out("required option ");
    // its not from arguments, 'parse' should set it to correct missing option
    out(err.ctx.resolved_name);
    out(" is missing\n");
    return std::move(out);
  }
  out(errc2str(err.what));
  if (err.what != errc::unknown_option)  // for better error message
    out(" when parsing ");
  else
    out(" ");
  out(err.ctx.typed);
  if (err.ctx.typed.starts_with("-") && !err.ctx.typed.starts_with("--")) {
    out(" resolved as ");
    out(err.ctx.resolved_name);
  }
  switch (err.what) {
    case errc::impossible_enum_value:
      for_each_option([&](auto o) {
        if constexpr (requires { o.names; }) {
          if (o.name() == err.ctx.resolved_name) {
            out(". Possible values are: ");
            for (auto& x : o.names)
              out(x), out(' ');
          }
        } else if constexpr (requires { o.values; }) {
          if (o.name() == err.ctx.resolved_name) {
            out(". Possible values are: ");
            for (auto& x : o.values)
              out(x), out(' ');
          }
        }
      });
      break;
    case errc::unknown_option: {
      std::vector<std::string> optionnames;
#define OPTION(name, ...) optionnames.push_back("--" #name);
#define ENUM(name, ...) optionnames.push_back("--" #name);
#define ALIAS(name, ...) optionnames.push_back("-" #name);
#include <clinok/generate.hpp>

      auto [str, diff] = clinok::best_match_str(err.ctx.typed, optionnames);
      if (diff < 3) {  // heuristic about misspelling
        out(" you probably meant the ");
        out(str);
        if (str.starts_with("--"))
          out(" option");
        else
          out(" alias");
      }
      break;
    }
    default:
      break;
  }
  out("\n");
  return std::move(out);
}

void print_err(const error_code& err, std::ostream& out = std::cerr) {
  print_err_to(err, [&](auto&& x) { out << x; });
}

// assumes first arg as program name
// parses args, dumps error and terminates program if error occured
inline options parse_or_exit(int argc, char* argv[]) {
  error_code ec;
  options o = parse(argc, argv, ec);
  if (ec) {
    std::cerr << '\n';
    print_err(ec);
    std::endl(std::cerr);
    std::exit(1);
  }
  return o;
}

}  // namespace CLINOK_NAMESPACE_NAME

#undef program_options_file
#undef CLINOK_NAMESPACE_NAME

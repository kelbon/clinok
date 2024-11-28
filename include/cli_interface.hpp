
// This header consists of two parts:
// * cli generator part, which using helper C++ code, included once
// * recursive include part, which used by geenrator part many times,
//   its required, because i want cli_interface to be only one header

#ifndef DAVID_DINAMIT_CLI_INTERFACE
#define DAVID_DINAMIT_CLI_INTERFACE

#include <cstring>
#include <span>
#include <string_view>
#include <optional>
#include <type_traits>
#include <cassert>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <initializer_list>
#include <array>
#include <charconv>
#include <cstdint>

namespace cli {

template <typename T, typename Ret = T>
struct string_switch {
  const std::string_view s;
  std::optional<Ret> result;

  constexpr string_switch(std::string_view s) noexcept : s(s) {
  }
  string_switch(string_switch&&) = delete;
  void operator=(string_switch&&) = delete;
  ~string_switch() = default;

  constexpr string_switch&& case_(std::string_view str,
                                  T value) && noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (!result && s == str)
      result.emplace(std::move(value));
    return std::move(*this);
  }
  [[nodiscard]] constexpr Ret default_(T Value) && noexcept(std::is_nothrow_move_constructible_v<Ret>) {
    if (result)
      return std::move(*result);
    return Value;
  }
  [[nodiscard]] constexpr operator Ret() && noexcept(std::is_nothrow_move_constructible_v<Ret>) {
    assert(!!result);
    return std::move(*result);
  }
};

using arg = const char*;

using args_t = std::span<const arg>;

// context of current parsing
struct context {
  args_t args;  // may be used to continue parsing
  size_t position = size_t(-1);
};

enum struct errc {
  ok,
  argument_missing,   // option reqires arg and it is missing
  arg_parsing_error,  // from_cli(string_view, option&) returns 'false'
  invalid_argument,   // argument is presented, but its invalid (not in enum or not bool etc)
  unknown_option,     // unknown option parsed
  impossible_enum_value,
  not_a_number  // parse int argument impossible
};

constexpr std::string_view to_string(errc e) noexcept {
  switch (e) {
    case errc::arg_parsing_error:
      return "arg parsing error";
    case errc::invalid_argument:
      return "invalid argument";
    case errc::argument_missing:
      return "argument missing";
    case errc::unknown_option:
      return "unknown option";
    case errc::impossible_enum_value:
      return "impossible enum value";
    case errc::not_a_number:
      return "not a number";
    case errc::ok:
    default:
      return "";
  }
}

struct error_code {
  errc what = errc::ok;
  context ctx;

  constexpr void set_error(errc ec, const context& ctx_) noexcept {
    what = ec;
    ctx = ctx_;
  }
  constexpr void clear() noexcept {
    what = errc::ok;
  }
  constexpr explicit operator bool() const noexcept {
    return what != errc::ok;
  }
  template <typename Out>
  constexpr Out print_to(Out out) {
    if (what == errc::ok)
      return std::move(out);
    out(to_string(what));
    out(" when parsing option ");
    out(*std::next(ctx.args.begin(), ctx.position - 1));
    if (what == errc::impossible_enum_value) {
      std::string_view nm = *std::next(ctx.args.begin(), ctx.position - 1);
      if (nm.starts_with("--"))
        nm.remove_prefix(2);
      for_each_option([&](auto o) {
        if constexpr (requires { o.values; }) {
          if (o.name() == nm) {
            out(". Possible values are: ");
            for (auto& x : o.values)
              out(x), out(' ');
          }
        }
      });
    }
    // TODO suggest best match? etc? (TODO fint nearest by расстояние левенштейна)
    return std::move(out);
  }
};

constexpr inline args_t args_range(int argc, const arg* argv) noexcept {
  assert(argc >= 0);
  if (argc == 0)
    return args_t{};
  return args_t(argv + 1, argv + argc);
}

namespace noexport {

constinit inline struct {
  // invariant : if count > 0, then argv != nullptr
  int argc = 0;
  const arg* argv = nullptr;
} args = {};

}  // namespace noexport

// returns empty string if 'cli::parse' not called yet or argv == 0
// otherwise returns argv[0]
// note: if you change 'argv' in 'main' you will observe it from this function, but why you rly dont needed it
inline std::string_view program_name() noexcept {
  const auto& args = noexport::args;
  return args.argc != 0 ? args.argv[0] : std::string_view{};
}

// returns empty range, if 'cli::parse' not called yet
// program args do not include first argument, which is program_name
inline args_t program_args() noexcept {
  const auto& args = noexport::args;
  return args_range(args.argc, args.argv);
}

template <typename...>
struct typelist {};

namespace noexport {
struct null_option {};
}  // namespace noexport

errc from_cli(std::string_view raw_arg, std::string_view& s) noexcept {
  s = raw_arg;
  return errc::ok;
}
errc from_cli(std::string_view raw_arg, std::int_least64_t& s) noexcept {
  auto [_, ec] = std::from_chars(raw_arg.data(), raw_arg.data() + raw_arg.size(), s);
  if (ec != std::errc{})
    return errc::not_a_number;
  return errc::ok;
}
errc from_cli(std::string_view raw_arg, bool& b) noexcept {
  int i = string_switch<int>(raw_arg)
              .case_("on", true)
              .case_("1", true)
              .case_("ON", true)
              .case_("YES", true)
              .case_("yes", true)
              .case_("true", true)
              .case_("no", false)
              .case_("NO", false)
              .case_("off", false)
              .case_("OFF", false)
              .case_("0", false)
              .case_("false", false)
              .default_(2);
  if (i == 2)
    return errc::invalid_argument;
  b = i;
  return errc::ok;
}

#define DD_CLI_STR
#define DD_CLI_STRdefault(...) "default: " #__VA_ARGS__ ", "

#define OPTION(type, NAME, description_, ...)           \
  struct NAME##_o {                                     \
    using value_type = type;                            \
    static consteval ::std::string_view name() {        \
      return #NAME;                                     \
    }                                                   \
    static consteval ::std::string_view description() { \
      return DD_CLI_STR##__VA_ARGS__ description_;      \
    }                                                   \
    static constexpr auto& get(auto& x) {               \
      return x.NAME;                                    \
    }                                                   \
  };
#define TAG(NAME, description_)                         \
  struct NAME##_o {                                     \
    using value_type = void;                            \
    static consteval ::std::string_view name() {        \
      return #NAME;                                     \
    }                                                   \
    static consteval ::std::string_view description() { \
      return description_;                              \
    }                                                   \
    static constexpr auto& get(auto& x) {               \
      return x.NAME;                                    \
    }                                                   \
  };
#define ENUM(NAME, description_, ...)                                                                        \
  struct NAME##_o {                                                                                          \
    static consteval ::std::string_view name() {                                                             \
      return #NAME;                                                                                          \
    }                                                                                                        \
    static consteval ::std::string_view description() {                                                      \
      return "one of: [" #__VA_ARGS__ "] " description_;                                                     \
    }                                                                                                        \
    static constexpr auto values = ::std::to_array({__VA_ARGS__});                                           \
    using value_type = std::conditional_t<std::is_integral_v<decltype((__VA_ARGS__))>, ::std::int_least64_t, \
                                          ::std::string_view>;                                               \
    static constexpr auto& get(auto& x) {                                                                    \
      return x.NAME;                                                                                         \
    }                                                                                                        \
  };

#include __FILE__

namespace noexport {

using all_options = typelist<::cli::noexport::null_option
#define OPTION(type, name, ...) , name##_o
#define ENUM(name, ...) , name##_o
#include __FILE__
                             >;

}  // namespace noexport

// passes empty option description object to 'foo'
constexpr void for_each_option(auto foo) {
  [&]<typename... Options>(typelist<::cli::noexport::null_option, Options...>) {
    (foo(Options{}), ...);
  }(noexport::all_options{});
}

// passes all options as empty option description objects to 'foo'
constexpr decltype(auto) apply_to_options(auto foo) {
  return [&]<typename... Options>(typelist<::cli::noexport::null_option, Options...>) -> decltype(auto) {
    return foo(Options{}...);
  }(noexport::all_options{});
}

struct options {
#define DD_CLI
#define DD_CLIdefault(...) = __VA_ARGS__
#define TAG(name, description) bool name = false;
#define BOOLEAN(name, description, ...) bool name DD_CLI##__VA_ARGS__;
#define STRING(name, description, ...) ::std::string_view name DD_CLI##__VA_ARGS__;
#define ENUM(name, description, ...) name##_o ::value_type name = name##_o ::values[0];
#define INTEGER(name, description, ...) ::std::int_least64_t name DD_CLI##__VA_ARGS__;

#include __FILE__

#undef DD_CLI
#undef DD_CLIdefault

  constexpr auto print_to(auto out) {
    for_each_option([&](auto opts) {
      out(opts.name()), out(" = ");
      constexpr bool int_like = requires { opts.get(*this) == 42; };
      int_like ? void() : (void)out('\"');
      out(opts.get(*this));
      int_like ? void() : (void)out('\"');
      out('\n');
    });
    return std::move(out);
  }
};

consteval bool has_option(std::string_view name) {
  return apply_to_options([&](auto... opts) { return ((opts.name() == name) || ...); });
}

// accepts function which acceps std::string_view to out
template <typename Out>
inline Out print_help_message_to(Out out) noexcept {
  auto option_arg_str = []<typename O>(O) {
    if (::std::is_same_v<typename O::value_type, std::string_view>)
      return "<string>";
    else if (::std::is_same_v<typename O::value_type, bool>)
      return "<bool>";
    else if (::std::is_integral_v<typename O::value_type>)
      return "<int>";
    else
      return "";
  };
  auto option_string_len = [&](auto o) -> size_t {
    return sizeof("--") + o.name().size() + ::std::strlen(option_arg_str(o));
  };
  ::std::size_t largest_help_string =
      apply_to_options([&](auto... opts) { return ::std::max({size_t(0), option_string_len(opts)...}); });
  for_each_option([&](auto o) {
    out(" --"), out(o.name()), out(' '), out(option_arg_str(o));
    const int whitespace_count = 2 + largest_help_string - option_string_len(o);
    for (int i = 0; i < whitespace_count; ++i)
      out(' ');
    out(o.description()), out('\n');
  });
#define ALIAS(a, b) out(" -"), out(#a), out(" is an alias to "), out(#b), out('\n');
#define OPTION(...)
#include __FILE__
  return ::std::move(out);
}

#undef DD_CLI_STR
#undef DD_CLI_STRdefault

// if option if to save program name, then this function not thread safe in theory, but rly.. dont call it
// multithread))
// TODO default handler, TODO merge-parsing with options&
constexpr options parse(args_t args, error_code& ec) noexcept {
  options o;
  auto set_error_on_pos = [&](auto it, errc what) {
    ec.set_error(what, context{args, (size_t)std::distance(args.begin(), it)});
  };
  auto try_parse = [&](auto it, auto& option_value) -> errc {
    if (it == args.end())
      return errc::argument_missing;
    return from_cli(std::string_view(*it), option_value);
  };
  for (auto it = args.begin(); it != args.end(); ++it) {
    std::string_view s = *it;
    if (s.starts_with("--")) {
      s.remove_prefix(2);
    } else if (s.starts_with('-') && s.size() > 1) {
      s.remove_prefix(1);
      constexpr auto aliases = std::to_array({
#define ALIAS(a, b) std::pair<std::string_view, std::string_view>{#a, #b},
#define OPTION(...)
#include __FILE__
      });
      if (auto it2 =
              std::find_if(std::begin(aliases), std::end(aliases), [&](auto& x) { return x.first == s; });
          it2 != std::end(aliases)) {
        s = it2->second;
      } else {
        set_error_on_pos(it, errc::unknown_option);
        return o;
      }
    }
#define TAG(name, ...)                \
  if (s == std::string_view(#name)) { \
    o.name = true;                    \
    continue;                         \
  }
#define STRING(name, ...)                                    \
  if (s == std::string_view(#name)) {                        \
    if (errc ec = try_parse(++it, o.name); ec != errc::ok) { \
      set_error_on_pos(it, ec);                              \
      return o;                                              \
    }                                                        \
    continue;                                                \
  }
#define ENUM(name, ...)                                                 \
  if (s == std::string_view(#name)) {                                   \
    if (errc ec = try_parse(++it, o.name); ec != errc::ok) {            \
      set_error_on_pos(it, ec);                                         \
      return o;                                                         \
    }                                                                   \
    constexpr auto& values = name##_o ::values;                         \
    if (std::find(begin(values), end(values), o.name) == end(values)) { \
      set_error_on_pos(it, errc::impossible_enum_value);                \
      return o;                                                         \
    }                                                                   \
    continue;                                                           \
  }
#define INTEGER(name, ...)                                   \
  if (s == std::string_view(#name)) {                        \
    if (errc ec = try_parse(++it, o.name); ec != errc::ok) { \
      set_error_on_pos(it, ec);                              \
      return o;                                              \
    }                                                        \
    continue;                                                \
  }
#define BOOLEAN(...) STRING(__VA_ARGS__)
#include __FILE__

    // if 'continue' not reached
    set_error_on_pos(it, errc::unknown_option);
    return o;
  }  // parse loop end
  return o;
}

inline options parse(int argc, char* argv[], error_code& ec) noexcept {
  assert(argc >= 0);
  ::cli::noexport::args = {argc, argv};
  options o = parse(args_range(argc, argv), ec);
  if (o.help) {
    print_help_message_to([](auto s) { ::std::cout << s; });
    ::std::flush(std::cout);
    ::std::exit(0);
  }
  return o;
}

// parses args, dumps error and terminates program if error occured
inline options parse_or_exit(int argc, char* argv[]) {
  error_code ec;
  options o = parse(argc, argv, ec);
  if (ec) {
    ::std::cerr << "an error occurred: ";
    ec.print_to([](auto s) { std::cerr << s; });
    ::std::flush(std::cerr);
    ::std::exit(1);
  }
  return o;
}

}  // namespace cli

#undef program_options_file

#else  // DAVID_DINAMIT_CLI_INTERFACE (start of self include part)

#ifndef OPTION
#define OPTION(...)
#endif

#ifndef BOOLEAN
#define BOOLEAN(...) OPTION(bool, __VA_ARGS__)
#endif

#ifndef STRING
#define STRING(...) OPTION(::std::string_view, __VA_ARGS__)
#endif

#ifndef ENUM
#define ENUM(...)
#endif

#ifndef TAG
#define TAG(...) OPTION(void, __VA_ARGS__)
#endif

#ifndef INTEGER
#define INTEGER(...) OPTION(::std::int_least64_t, __VA_ARGS__)
#endif

#ifndef PATH
#define PATH(...) OPTION(::std::filesystem::path, __VA_ARGS__)
#endif

#ifndef ALIAS
#define ALIAS(a, b)
#endif

// file with description of program options, format: TODO link
#ifndef program_options_file
#define program_options_file "program_options.def"
#endif

#include program_options_file
TAG(help, "list of all options")

#undef BOOLEAN
#undef STRING
#undef TAG
#undef ENUM
#undef PATH
#undef OPTION
#undef INTEGER
#undef ALIAS

#endif  // DAVID_DINAMIT_CLI_INTERFACE (end of self include part)

#pragma once

#include <vector>
#include <string_view>

#include "clinok/utils.hpp"
#include "clinok/type_descriptor.hpp"

namespace clinok {

template <typename T>
concept CLI_like = requires {
  typename T::all_options;

  typename T::options;
  typename T::presented_options;

  T::aliases;
  T::allow_additional_args;
};

template <typename T>
concept option_like = std::is_empty_v<T> && requires {
  typename T::logic_type;  // may be `void` for tag options
  typename T::cpp_type;    // type stored in options struct
  // name from options file
  { T::name() };
  // description from options file
  { T::description() };
  // is has value for `options` struct if option not passed in arguments
  { T::has_default() } -> std::same_as<bool>;
  // T::get(auto& x)  returns field associated with option

  // required only if has_default() == true.
  // Passed as default(<...>) in options file
  //
  // T::default_args() -> clinok::args_t
};

template <typename O>
struct logic_type : std::type_identity<typename O::logic_type> {};

template <typename O>
using logic_type_t = typename logic_type<O>::type;

template <typename O>
using cpp_type_t = typename O::cpp_type;

template <typename O>
consteval bool is_tag_option() {
  return std::is_void_v<logic_type_t<O>>;
}

// may be specialied for user-declared types
// placeholder used in generated `help` to print expected input, <int> or smth like
template <typename O>
constexpr inline std::string_view placeholder_of = type_descriptor<logic_type_t<O>>::placeholder();

// may be specialied for user-declared types
// `name_of` is name for option parsing, e.g. it may be useful to rename option
// declared in options file as `my_opt` into `my-opt`
template <typename O>
constexpr inline std::string_view name_of = O::name();

// not exist by default, may be overloaded for concrete option
template <typename O>
constexpr auto possible_values_description(O) {
  static_assert(option_like<O>);
  using D = type_descriptor<logic_type_t<O>>;
  if constexpr (requires { D::possible_values_description(); }) {
    return D::possible_values_description();
  } else {
    return;  // void
  }
}

template <typename O>
constexpr bool has_possible_values_description(O) {
  return !std::is_void_v<decltype(possible_values_description(O{}))>;
}

template <typename O>
constexpr cpp_type_t<O> default_value_for() {
  if constexpr (is_tag_option<O>()) {
    return false;
  } else {
    // error if no default present
    // supports array options too (more than 1 arg as default value)
    args_t args(O::default_args().begin(), O::default_args().end());
    cpp_type_t<O> value;
    errc ec = errc::ok;
    auto it = parse_option(O{}, args.begin(), args.end(), value, ec);
    (void)it;
    assert(it == args.end() && ec == errc::ok);  // default value must be parsable
    return value;
  }
}

// may be adl-specifizlied for concrete O (ption)
template <typename O>
constexpr args_t::iterator parse_option(O, args_t::iterator b, args_t::iterator e, cpp_type_t<O>& out,
                                        errc& ec) {
  return type_descriptor<logic_type_t<O>>::parse_option(b, e, out, ec);
}

// passes empty option description object to 'foo'
template <CLI_like CLI>
constexpr void for_each_option(auto foo) {
  [&]<typename... Options>(clinok::typelist<Options...>) {
    (foo(Options{}), ...);
  }(typename CLI::all_options{});
}

// passes all options as empty option description objects to 'foo'
template <CLI_like CLI>
constexpr decltype(auto) apply_to_options(auto foo) {
  return [&]<typename... Options>(clinok::typelist<Options...>) -> decltype(auto) {
    return foo(Options{}...);
  }(typename CLI::all_options{});
}

// returns false if no such option
template <CLI_like CLI>
constexpr bool visit_option(std::string_view name, auto foo) {
  return apply_to_options<CLI>([&](auto... os) {
    auto visit_one = [&]<typename O>(O o) -> bool {
      if (name != name_of<O>) {
        return false;
      }
      foo(o);
      return true;
    };
    return (visit_one(os) || ...);
  });
}

template <CLI_like CLI>
constexpr bool has_option(std::string_view name) {
  return visit_option<CLI>(name, [](auto) {});
}

// accepts function which acceps std::string_view to out
template <CLI_like CLI, typename Out>
inline Out print_help_message_to(Out out) noexcept {
  out('\n');
  auto option_string_len = [&]<typename O>(O o) -> size_t {
    return sizeof("--") + name_of<O>.size() + placeholder_of<O>.size();
  };
  std::size_t largest_help_string =
      apply_to_options<CLI>([&](auto... opts) { return std::max({size_t(0), option_string_len(opts)...}); });
  for_each_option<CLI>([&]<typename O>(O o) {
    out(" --"), out(name_of<O>), out(' '), out(placeholder_of<O>);
    const int whitespace_count = 2 + largest_help_string - option_string_len(o);
    for (int i = 0; i < whitespace_count; ++i)
      out(' ');

    if constexpr (O::has_default() && !is_tag_option<O>()) {
      out("default: ");

      auto args = O::default_args();
      if (args.size() == 1) {
        out('"');
        out(std::string_view(args.front()));
        out('"');
      } else {
        out(noexport::join_comma(O::default_args(), [](arg a) {
          std::string s;
          s += '"';
          s += std::string_view(a);
          s += '"';
          return s;
        }));
      }
      out(", ");
    }
    out(O::description());
    if constexpr (has_possible_values_description(O{})) {
      out(". Possible values: ");
      out(possible_values_description(O{}));
    }
    out('\n');
  });

  for (auto [a, b] : CLI::aliases) {
    out(" -"), out(a), out(" is an alias to "), out(b), out('\n');
  }

  return std::move(out);
}

template <CLI_like CLI>
[[nodiscard]] constexpr bool has_alias(std::string_view name) {
  auto it = std::ranges::find_if(CLI::aliases, [&](auto& x) { return x.first == name; });
  using std::end;
  return it != end(CLI::aliases);
}

// returns resolved option name or empty string if no such alias
template <CLI_like CLI>
[[nodiscard]] constexpr std::string_view resolve_alias(std::string_view alias) {
  using std::end;
  auto it = std::ranges::find_if(CLI::aliases, [alias](auto& x) { return x.first == alias; });
  if (it != end(CLI::aliases)) {
    if (has_option<CLI>(it->second))
      return it->second;
    else {
      if (has_alias<CLI>(it->second))
        return resolve_alias<CLI>(it->second);
      else
        return "";
    }
  }
  return "";
}

template <typename CLI>
constexpr bool validate_aliases() {
  // array [alias -> resolved, ...]
  using std::begin;
  using std::end;
  std::vector<std::pair<std::string_view, std::string_view>> aliases(begin(CLI::aliases), end(CLI::aliases));
  // assume all not empty
  for (auto& [a, r] : aliases) {
    if (a.empty())
      throw +"bad alias, empty string";
    if (r.empty())
      throw +"bad alias, resolves to empty";
    if (a == r)
      throw +"bad alias, resolves to itself";
  }

  for (auto& [a, r] : aliases) {
    if (has_option<CLI>(std::string_view(a)))
      throw +"bad alias, same name as option";
  }

  for (auto& [a, r] : aliases) {
    std::string_view resolved = resolve_alias<CLI>(a);
    if (resolved.empty())
      throw +"bad alias, option do not exist";
  }
  if (aliases.size() < 2)
    return true;
  // sort by alias names AND values to effectively find not unique aliases
  std::ranges::sort(aliases, [](auto&& l, auto&& r) {
    return l.first == r.first ? l.second < r.second : l.first < r.first;
  });
  auto it = std::ranges::adjacent_find(
      aliases, [](auto&& f, auto&& s) { return f.first == s.first && f.second != s.second; });

  if (it != end(aliases))
    throw +"two aliases with same name resolved to different options";
  return true;
}

template <CLI_like CLI, typename Out>
constexpr Out print_err_to(const error_code& err, Out out) {
  if (err.what == errc::ok)
    return std::move(out);
  if (err.what == errc::option_missing) {
    out("option name is missing, \"");
    out(err.ctx.typed);
    out("\" used instead");
    return std::move(out);
  }
  if (err.what == errc::required_option_not_present) {
    out("required option \"");
    // its not from arguments, 'parse' should set it to correct missing option
    out(err.ctx.resolved_name);
    out("\" is missing\n");
    return std::move(out);
  }
  out(e2str(err.what));
  if (err.what != errc::unknown_option && err.what != errc::disallowed_free_arg)  // for better error message
    out(" when parsing \"");
  else
    out(" \"");
  out(err.ctx.typed);
  out("\"");
  if (err.ctx.typed.starts_with("-") && !err.ctx.typed.starts_with("--")) {
    out(" resolved as \"--");
    out(err.ctx.resolved_name);
    out("\"");
  }
  switch (err.what) {
    case errc::invalid_argument:
      for_each_option<CLI>([&]<typename O>(O o) {
        if constexpr (has_possible_values_description(O{})) {
          if (name_of<O> == err.ctx.resolved_name) {
            out(". Possible values are: ");
            out(possible_values_description(o));
          }
        }
      });
      break;
    case errc::unknown_option: {
      std::vector<std::string> optionnames;
      for_each_option<CLI>(
          [&]<typename O>(O o) { optionnames.push_back(std::string("--").append(name_of<O>)); });

      for (auto [a, _] : CLI::aliases) {
        optionnames.push_back(std::string("-").append(a));
      }

      auto [str, diff] = clinok::best_match_str(err.ctx.typed, optionnames);
      if (diff < 5) {  // heuristic about misspelling
        out(" you probably meant \"");
        out(str);
        if (str.starts_with("--"))
          out("\" option");
        else
          out("\" alias");
      }
      break;
    }
    default:
      break;
  }
  out('\n');
  return std::move(out);
}

template <typename CLI>
void print_err(const error_code& err, std::ostream& out = std::cerr) {
  print_err_to<CLI>(err, [&](auto&& x) { out << x; });
}

// assumes first arg as program name
// increments correspoding field in `presented` for each option, so caller may know which options presented
// Note: previous value of `presented`  will be forgotten
template <CLI_like CLI>
constexpr typename CLI::options parse(args_t args, typename CLI::presented_options& presented,
                                      error_code& ec) noexcept {
  static_assert(validate_aliases<CLI>());
  assert(!args.empty());

  typename CLI::options opts;
  presented = {};

  auto set_error = [&](std::string_view typed, errc what, std::string_view resolved) {
    ec.set_error(what, context{std::string(typed), std::string(resolved)});
  };
  std::string_view s;
  std::string_view typed;
  errc er = errc::ok;

  // skip program name
  for (auto it = args.begin() + 1; it != args.end();) {
    typed = s = *it;
    ++it;
    if (s == "-" || s == "--") {
      set_error(typed, errc::option_missing, "");
      return opts;
    }

    if (s.starts_with("--")) {
      s.remove_prefix(2);
    } else if (s.starts_with('-')) {
      s.remove_prefix(1);
      std::string_view resolved = resolve_alias<CLI>(s);
      if (resolved.empty()) [[unlikely]] {
        set_error(typed, errc::unknown_option, s);
        return opts;
      }
      s = resolved;
    } else {
      if constexpr (!CLI::allow_additional_args) {
        set_error(typed, errc::disallowed_free_arg, s);
        return opts;
      } else {
        // prevent compile time error
        [&](auto&& o) {
          if constexpr (CLI::allow_additional_args)
            o.additional_args.push_back(typed);
        }(opts);
        continue;
      }
    }

    bool processed = visit_option<CLI>(s, [&](auto o) {
      ++o.get(presented);
      it = parse_option(o, it, args.end(), o.get(opts), er);
    });

    if (er != clinok::errc::ok) {
      set_error(typed, er, s);
      return opts;
    }

    if (!processed) {
      set_error(typed, errc::unknown_option, s);
      return opts;
    }
  }  // parse loop end

  for_each_option<CLI>([&]<typename O>(O o) {
    // is required and not present in arg list
    if (o.get(presented) == 0) {
      if constexpr (!O::has_default()) {
        set_error(name_of<O>, errc::required_option_not_present, name_of<O>);
      } else {
        o.get(opts) = default_value_for<O>();
      }
    }
  });
  return opts;
}

// assumes first arg as program name
template <CLI_like CLI>
constexpr typename CLI::options parse(args_t args, error_code& ec) noexcept {
  typename CLI::presented_options presented;
  return parse<CLI>(args, presented, ec);
}

// assumes first arg as program name
template <CLI_like CLI>
inline typename CLI::options parse(int argc, char* argv[], error_code& ec) noexcept {
  assert(argc >= 0);
  typename CLI::options o = parse<CLI>(args_range(argc, argv), ec);
  if (o.help) {
    print_help_message_to<CLI>([](auto s) { std::cout << s; });
    std::flush(std::cout);
    std::exit(0);
  } else if (ec) {
    // skip program name
    bool help_found = false;
    for (std::string_view a : args_range(argc - 1, argv + 1)) {
      if (a == "--help") {
        help_found = true;
        break;
      }
      if (a.starts_with('-') && !a.starts_with("--"))
        a.remove_prefix(1);
      if (has_alias<CLI>(a) && resolve_alias<CLI>(a) == "help") {
        help_found = true;
        break;
      }
    }
    if (help_found) {
      print_help_message_to<CLI>([](auto s) { std::cout << s; });
      std::flush(std::cout);
      std::exit(EXIT_FAILURE);
    }
  }
  return o;
}

// assumes first arg as program name
// parses args, dumps error and terminates program if error occured
template <typename CLI>
inline typename CLI::options parse_or_exit(int argc, char* argv[]) {
  error_code ec;
  typename CLI::options o = parse<CLI>(argc, argv, ec);
  if (ec) {
    std::cerr << '\n';
    print_err<CLI>(ec);
    std::endl(std::cerr);
    std::exit(1);
  }
  return o;
}

template <CLI_like CLI>
inline typename CLI::options default_options() {
  typename CLI::options opts;
  for_each_option<CLI>([&]<typename O>(O o) {
    if constexpr (O::has_default())
      o.get(opts) = default_value_for<O>();
  });
  // else zero initialized
  return opts;
}

}  // namespace clinok

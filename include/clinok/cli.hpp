#pragma once

#include <vector>

#include "clinok/descriptor_field.hpp"

namespace clinok {

template <typename T>
concept CLI_like = requires {
  typename T::all_options;

  typename T::options;
  typename T::presented_options;

  T::aliases;
  T::allow_additional_args;
};

// passes empty option description object to 'foo'
template <CLI_like CLI>
constexpr void for_each_option(auto foo) {
  [&]<typename... Options>(clinok::typelist<Options...>) {
    (foo(descriptor_field<Options>{}), ...);
  }(typename CLI::all_options{});
}

// passes empty option description object to 'foo'
template <CLI_like CLI>
constexpr bool on_all_options(auto foo) {
  return [&]<typename... Options>(clinok::typelist<Options...>) {
    return (foo(descriptor_field<Options>{}) || ...);
  }(typename CLI::all_options{});
}

// passes all options as empty option description objects to 'foo'
template <CLI_like CLI>
constexpr decltype(auto) apply_to_options(auto foo) {
  return [&]<typename... Options>(clinok::typelist<Options...>) -> decltype(auto) {
    return foo(descriptor_field<Options>{}...);
  }(typename CLI::all_options{});
}

template <CLI_like CLI>
constexpr bool visit_option(std::string_view name, auto foo) {
  return apply_to_options<CLI>([&](auto... os) {
    auto visit_one = [&](auto o) -> bool {
      if (name != o.name()) {
        return false;
      }
      foo(o);
      return true;
    };
    return (visit_one(os) || ...);
  });
}

template <CLI_like CLI>
consteval bool has_option(std::string_view name) {
  return apply_to_options<CLI>([&](auto... opts) { return ((opts.name() == name) || ...); });
}

// accepts function which acceps std::string_view to out
template <CLI_like CLI, typename Out>
inline Out print_help_message_to(Out out) noexcept {
  out("\n");
  auto option_string_len = [&]<typename O>(O o) -> size_t {
    return sizeof("--") + O::name().size() + o.placeholder().size();
  };
  std::size_t largest_help_string =
      apply_to_options<CLI>([&](auto... opts) { return std::max({size_t(0), option_string_len(opts)...}); });
  for_each_option<CLI>([&]<typename O>(O o) {
    out(" --"), out(O::name()), out(' '), out(o.placeholder());
    const int whitespace_count = 2 + largest_help_string - option_string_len(o);
    for (int i = 0; i < whitespace_count; ++i)
      out(' ');
    if (O::has_implicit_default() && !is_required_option_v<O>) {
      out("Default: ");
      out(O::default_value_desc());
      out(". ");
    }
    out(O::description());
    if (O::has_possible_values()) {
      out(". Possible values: ");
      out(O::possible_values_desc());
    }
    out('\n');
  });

  for (auto [a, b] : CLI::aliases) {
    out(" -"), out(a), out(" is an alias to "), out(b), out('\n');
  }

  return std::move(out);
}

// assumes first arg as program name
template <CLI_like CLI>
constexpr typename CLI::options parse(args_t args, error_code& ec) noexcept {
  typename CLI::options opts;
  typename CLI::presented_options p_opts;

  auto set_error = [&](std::string_view typed, errc what, std::string_view resolved) {
    ec.set_error(what, context{std::string(typed), std::string(resolved)});
  };
  for (auto it = args.begin(); it != args.end();) {
    std::string_view typed = *it;
    std::string_view s = *it;

    if (s == "-" || s == "--") {
      set_error(typed, errc::option_missing, "");
      return opts;
    }

    if (s.starts_with("--")) {
      s.remove_prefix(2);
    } else if (s.starts_with('-')) {
      s.remove_prefix(1);
      if (auto it2 = std::ranges::find_if(CLI::aliases, [&](auto& x) { return x.first == s; });
          it2 != std::end(CLI::aliases)) {
        s = it2->second;
      } else {
        set_error(typed, errc::unknown_option, s);
        return opts;
      }
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
        ++it;
        continue;
      }
    }

    errc er = errc::ok;

    bool processed = visit_option<CLI>(s, [&](auto o) {
      bool& presented = o.get(p_opts);
      if (presented) {
        // TODO
      }
      presented = true;

      auto& out = o.get(opts);

      it = o.parse(++it, args.end(), out, er);
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
    if (!o.get(p_opts)) {
      if (is_required_option_v<O> || !O::has_default()) {
        set_error(o.name(), errc::required_option_not_present, o.name());
      } else {
        auto& out = o.get(opts);
        O::set_default_value(out);
      }
    }
  });
  return opts;
}

template <CLI_like CLI>
// assumes first arg as program name
inline typename CLI::options parse(int argc, char* argv[], error_code& ec) noexcept {
  assert(argc >= 0);
  typename CLI::options o = parse<CLI>(args_range(argc, argv), ec);
  if (o.help) {
    print_help_message_to<CLI>([](auto s) { std::cout << s; });
    std::flush(std::cout);
    std::exit(0);
  }
  return o;
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
  out(errc2str(err.what));
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
      for_each_option<CLI>([&](auto o) {
        if (o.name() == err.ctx.resolved_name && o.has_possible_values()) {
          out(". Possible values are: ");
          out(o.possible_values_desc());
        }
      });
      break;
    case errc::unknown_option: {
      std::vector<std::string> optionnames;
      for_each_option<CLI>([&](auto o) { optionnames.push_back(std::string("--").append(o.name())); });

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
  out("\n");
  return std::move(out);
}

template <typename CLI>
void print_err(const error_code& err, std::ostream& out = std::cerr) {
  print_err_to<CLI>(err, [&](auto&& x) { out << x; });
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
}  // namespace clinok

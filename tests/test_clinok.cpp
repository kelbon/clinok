#include <string>
#include <vector>

static int calcualte_default_myint() {
  return 17;
}

#define program_options_file "../tests/program_options.def"
#define CLINOK_NAMESPACE_NAME cli1
#include <clinok/cli_interface.hpp>

#define program_options_file "../tests/program2_options.def"
#define CLINOK_NAMESPACE_NAME cli2

#include <clinok/cli_interface.hpp>

#include "../examples/point.hpp"

#define program_options_file "../tests/program3_options.def"
#define CLINOK_NAMESPACE_NAME cli3

#include <clinok/cli_interface.hpp>

#include <iostream>
#include <sstream>
#include <tuple>
#include <algorithm>

void on_error(int line) {
  std::cout << "ERROR ON LINE " << line;
  std::exit(line);
}

static std::pair<std::string_view, std::string_view> diff(std::string_view a, std::string_view b) {
  auto [it1, it2] = std::ranges::mismatch(a, b);
  return {std::string_view(it1, a.end()), std::string_view(it2, b.end())};
}

#define error_if(...) \
  if ((__VA_ARGS__))  \
  on_error(__LINE__)

#define assert_eq(a, b)                                                                                      \
  if (a != b) {                                                                                              \
    std::cout << "EXPECTED \"" << a << "\" BUT ACTUAL IS \"" << b << "\"" << std::endl;                      \
    std::cout << "DIFF: \"" << diff(a, b).first << "\"\n AND \n\"" << diff(a, b).second << '"' << std::endl; \
    error_if(true);                                                                                          \
  }

bool operator==(const cli1::options& lhs, const cli1::options& rhs) {
  auto t = [](const cli1::options& o) {
    return std::tie(o.help, o.mytag, o.works, o.hello_world, o.myname, o.ABC2, o.myint, o.myint2, o.color);
  };

  return t(lhs) == t(rhs);
}

bool operator==(const cli2::options& lhs, const cli2::options& rhs) {
  auto t = [](const cli2::options& o) {
    return std::tie(o.help, o.mytag, o.works, o.hello_world, o.myname, o.ABC2, o.additional_args);
  };
  return t(lhs) == t(rhs);
}

bool operator==(const cli3::options& lhs, const cli3::options& rhs) {
  auto t = [](const cli3::options& o) {
    return std::tie(o.help, o.log_level, o.timeout, o.user, o.location);
  };
  return t(lhs) == t(rhs);
}

void use(auto&&...) {
}

namespace clinok {

template <>
constexpr std::string_view placeholder_of<cli3::timeout_o> = "<seconds>";

}  // namespace clinok

void test_trim_ws() {
  using clinok::noexport::trim_ws;

  static_assert(trim_ws("") == "");
  static_assert(trim_ws("   ") == "");
  static_assert(trim_ws(" aa") == "aa");
  static_assert(trim_ws("abc") == "abc");
  static_assert(trim_ws("abc   ") == "abc");
  static_assert(trim_ws("   abc   ") == "abc");
}

void test_split_by_comma() {
  using clinok::noexport::split_str_by_comma;

  std::vector<std::string_view> vec(100);
  std::string_view* in = vec.data();
  auto* out = split_str_by_comma("", in);
  error_if(out != in);
  out = split_str_by_comma("hello", in);
  error_if(out - in != 1);
  error_if(*in != "hello");
  std::fill(vec.begin(), vec.end(), "");
  out = split_str_by_comma("hello , world", in);
  error_if(out - in != 2);
  error_if(in[0] != "hello" || in[1] != "world");
  out = split_str_by_comma("  hello,world, abc", in);
  error_if(out - in != 3);
  error_if(in[0] != "hello" || in[1] != "world" || in[2] != "abc");
  std::fill(vec.begin(), vec.end(), "");
  try {
    out = split_str_by_comma("  hello,world, abc,", in);
  } catch (const char* p) {
    error_if(std::string_view(p) != "invalid syntax: trailing comma");
  }
  try {
    out = split_str_by_comma("  hello,world, abc,  ", in);
  } catch (const char* p) {
    error_if(std::string_view(p) != "invalid syntax: trailing comma");
  }
  try {
    out = split_str_by_comma(" ,  ", in);
  } catch (const char* p) {
    error_if(std::string_view(p) != "invalid syntax: trailing comma");
  }
  try {
    out = split_str_by_comma(",", in);
  } catch (const char* p) {
    error_if(std::string_view(p) != "invalid syntax: trailing comma");
  }
}

void test_levenshtein_distance() {
  struct testcase {
    std::string a;
    std::string b;
    double expected;
  };
  // clang-format off
  testcase tests[] = {
      {"", "", 0},
      {"cat", "cat", 0},
      {"kitten", "sitting", 5},
      {"ca", "abc", 3},
      {"abcd", "bacd", 1},
      {"abcd", "badc", 2},
      {"12345", "12354", 1},
      {"hello", "hlelo", 1},
      {"hello", "helo", 1},
      {"book", "back", 4},
  };
  // clang-format on
  for (const auto& test : tests) {
    double result = clinok::damerau_levenshtein_distance(test.a, test.b);
    error_if(result != test.expected);
  }
  // replace with qwerty rules
  double d1 = clinok::damerau_levenshtein_distance("heao", "heap");
  double d2 = clinok::damerau_levenshtein_distance("heaa", "heap");
  error_if(d1 != 0.5 || d2 != 2);  // 'p' closer to 'o' then to 'a' on qwerty keyboard
}

void test_parse1(std::vector<const char*> argv, clinok::errc expected_err, std::string expected_msg) {
  clinok::error_code ec;
  cli1::options o = cli1::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  if (expected_err != clinok::errc::ok)
    error_if(!ec);
  assert_eq(e2str(expected_err), e2str(ec.what));
  std::stringstream str;
  cli1::print_err_to(ec, [&](auto x) { str << x; });
  auto s = str.str();
  assert_eq(expected_msg, s);
}

void test_parse1_value(std::vector<const char*> argv, cli1::options expected) {
  clinok::error_code ec;
  cli1::options o = cli1::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  error_if(ec);
  error_if(o != expected);
}

void test_parse2(std::vector<const char*> argv, clinok::errc expected_err, std::string expected_msg) {
  clinok::error_code ec;
  cli2::options o = cli2::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  if (expected_err != clinok::errc::ok)
    error_if(!ec);
  assert_eq(e2str(expected_err), e2str(ec.what));
  std::stringstream str;
  cli2::print_err_to(ec, [&](auto x) { str << x; });
  auto s = str.str();
  assert_eq(expected_msg, s);
}

void test_parse2_value(std::vector<const char*> argv, cli2::options expected) {
  clinok::error_code ec;
  cli2::options o = cli2::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  error_if(ec);

  error_if(o != expected);
}

void test_parse3(std::vector<const char*> argv, clinok::errc expected_err, std::string expected_msg) {
  clinok::error_code ec;
  cli3::options o = cli3::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  if (expected_err != clinok::errc::ok)
    error_if(!ec);

  assert_eq(e2str(expected_err), e2str(ec.what));
  std::stringstream str;
  cli3::print_err_to(ec, [&](auto x) { str << x; });
  auto s = str.str();
  assert_eq(expected_msg, s);
}

void test_parse3_value(std::vector<const char*> argv, cli3::options expected) {
  clinok::error_code ec;
  cli3::options o = cli3::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  error_if(ec);

  error_if(o != expected);
}

void test_select_subprogram(std::vector<const char*> vecargs, std::string_view progname,
                            std::initializer_list<std::string_view> subprogram_names,
                            std::string expected_msg, int expected_index) {
  int argc = vecargs.size();
  char** argv = (char**)vecargs.data();
  std::stringstream ssm;
  int i = clinok::select_subprogram(argc, argv, progname, subprogram_names, ssm);
  auto msg = ssm.str();
  assert_eq(expected_msg, msg);
  error_if(i != expected_index);
}

template <clinok::CLI_like CLI>
std::string get_help() {
  std::string result;
  clinok::print_help_message_to<CLI>([&](auto x) {
    std::stringstream ss;
    ss << x;
    result.append(ss.str());
  });
  return result;
}

// clang-format off
constexpr std::string_view expected_help1 = R"(
 --mytag                 something
 --works <bool>          default: "false", it works
 --hello_world <string>  default: "hello, man", because, why not
 --myname <string>       default: "", fdsfdsfsddsfsdffdsfsireriteowireowroiewr
 --ABC2 <string>         default: "why", VERYmda

 --myint <int>           default: 17, myint description
 --myint2 <int>          myint2 without default
 --color <string>        description. Possible values: [red, green, blue, yellow]
 --help                  list of all options
 -h is an alias to help
 -w is an alias to works
 -hh is an alias to myint
 -i is an alias to myint2
 -c is an alias to color
)";

constexpr std::string_view expected_help2 = R"(
 --mytag                 something
 --works <bool>          default: "false", it works
 --hello_world <string>  default: "hello, man", because, why not
 --myname <string>       fdsfdsfsddsfsdffdsfsireriteowireowroiewri
                      feworuiwerioewroieroiwr324032423044023eowur98
                      weurfiowir3295r823ri02v,3rvm23v,
                      rwrceri9234rioeeosdfosdiodfiosdf
 --ABC2 <string>         default: "why", VERYLONG
DESC FDISF ODF OFDOISE 

 --help                  list of all options
)";

constexpr std::string_view expected_help3 = R"(
 --log-level <string>  default: "trace", Verbosity level. Possible values: [trace, debug, info, warn, error]
 --timeout <seconds>   default: "10", Request timeout in seconds
 --user <string>       User name
 --location <x> <y>    default: ["0", "0"], Custom type: point x,y
 --help                list of all options
 -l is an alias to log-level
 -t is an alias to timeout
)";

// clang-format on

using alias = std::pair<std::string_view, std::string_view>;
struct A1 {
  using all_options = clinok::typelist<>;
  using options = void;
  using presented_options = void;

  static constexpr bool allow_additional_args = false;

  static constexpr alias aliases[] = {{"a", "b"}};
};

namespace my {

struct pseudooptionA {};
struct pseudooptionB {};
struct pseudooptionC {};

}  // namespace my

namespace clinok {

template <>
constexpr std::string_view name_of<my::pseudooptionA> = "a";

template <>
constexpr std::string_view name_of<my::pseudooptionB> = "b";

template <>
constexpr std::string_view name_of<my::pseudooptionC> = "c";

}  // namespace clinok

struct A2 : A1 {
  using all_options = clinok::typelist<my::pseudooptionA>;
};

struct A3 : A1 {
  using all_options = clinok::typelist<my::pseudooptionB>;
};

struct A4 : A1 {
  static constexpr alias aliases[] = {{"", "b"}};
};

struct A5 : A1 {
  static constexpr alias aliases[] = {{"a", ""}};
};
struct A6 : A1 {
  using all_options = clinok::typelist<my::pseudooptionA, my::pseudooptionB, my::pseudooptionC>;
  static constexpr alias aliases[] = {{"d", "b"}, {"abc", "b"}, {"fffdd", "a"}, {"d", "c"}};
};

struct A7 : A1 {
  static constexpr alias aliases[] = {{"a", "a"}};
};

struct A8 : A1 {
  using all_options = clinok::typelist<my::pseudooptionA, my::pseudooptionB, my::pseudooptionC>;
  static constexpr alias aliases[] = {{"d", "b"}, {"abc", "b"},   {"fffdd", "a"},
                                      {"d", "b"}, {"fffdd", "a"}, {"d", "a"}};
};

struct A9R : A1 {
  using all_options = clinok::typelist<my::pseudooptionA, my::pseudooptionB, my::pseudooptionC>;
  static constexpr alias aliases[] = {
      {"d", "b"}, {"abc", "b"}, {"fffdd", "a"}, {"d", "b"}, {"fffdd", "a"}, {"dd", "d"},
  };
};

void test_resolve_aliases() {
  std::string_view s = clinok::resolve_alias<A9R>("alala");
  assert_eq("", s);
  s = clinok::resolve_alias<A9R>("d");
  assert_eq("b", s);
  s = clinok::resolve_alias<A9R>("dd");
  // dd -> d -> b
  assert_eq("b", s);
  s = clinok::resolve_alias<A9R>("fffdd");
  assert_eq("a", s);
  s = clinok::resolve_alias<A9R>("abc");
  assert_eq("b", s);
}

void test_validate_aliases() {
  try {
    clinok::validate_aliases<A1>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("bad alias, option do not exist"), p);
  }
  try {
    clinok::validate_aliases<A2>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("bad alias, same name as option"), p);
  }
  try {
    clinok::validate_aliases<A3>();
  } catch (...) {
    error_if(true);
  }
  try {
    clinok::validate_aliases<A4>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("bad alias, empty string"), p);
  }
  try {
    clinok::validate_aliases<A5>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("bad alias, resolves to empty"), p);
  }
  try {
    clinok::validate_aliases<A6>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("two aliases with same name resolved to different options"), p);
  }
  try {
    clinok::validate_aliases<A7>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("bad alias, resolves to itself"), p);
  }
  try {
    clinok::validate_aliases<A8>();
    error_if(true);
  } catch (const char* p) {
    assert_eq(std::string_view("two aliases with same name resolved to different options"), p);
  }
  try {
    clinok::validate_aliases<A9R>();
  } catch (...) {
    error_if(true);
  }
}

int main() {
  test_resolve_aliases();
  test_validate_aliases();
  test_trim_ws();
  test_split_by_comma();
  test_levenshtein_distance();
  test_parse1(
      {
          "program_name_placeholder",
          "--a",
      },
      clinok::errc::unknown_option, "unknown option \"--a\" you probably meant \"-w\" alias\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--ab",
      },
      clinok::errc::unknown_option, "unknown option \"--ab\" you probably meant \"--ABC2\" option\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--myint",
      },
      clinok::errc::argument_missing, "argument missing when parsing \"--myint\"\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--color",
          "0",
      },
      clinok::errc::invalid_argument,
      "invalid argument when parsing \"--color\". Possible values are: [red, green, blue, yellow]\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--myint",
          "1",
      },
      clinok::errc::required_option_not_present, "required option \"color\" is missing\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--color",
          "blue",
      },
      clinok::errc::required_option_not_present, "required option \"myint2\" is missing\n");
  test_parse1(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "--color",
          "blue",
      },
      clinok::errc::ok, "");
  // with alias
  test_parse1(
      {
          "program_name_placeholder",
          "-i",
          "1",
          "-c",
          "yellow",
      },
      clinok::errc::ok, "");

  // with alias 'hh' to 'abc'
  test_parse1(
      {
          "program_name_placeholder",
          "-hh",
      },
      clinok::errc::argument_missing, "argument missing when parsing \"-hh\" resolved as \"--myint\"\n");
  test_parse1(
      {
          "program_name_placeholder",
          "-c",
          "0",
      },
      clinok::errc::invalid_argument,
      "invalid argument when parsing \"-c\" resolved as \"--color\". Possible values are: [red, green, blue, "
      "yellow]\n");

  {
    cli1::options expected = clinok::default_options<cli1::cli_t>();
    expected.myint = 1;
    expected.myint2 = 1;
    expected.color = cli1::color_e::red;
    test_parse1_value(
        {
            "program_name_placeholder",
            "--myint2",
            "1",
            "-hh",
            "1",
            "-c",
            "red",
        },
        expected);
  }

  {
    cli1::options expected = clinok::default_options<cli1::cli_t>();
    expected.myint2 = 1;
    expected.color = cli1::color_e::red;
    test_parse1_value(
        {
            "program_name_placeholder",
            "--myint2",
            "1",
            "--color",
            "red",
        },
        expected);
  }
  {
    cli1::options expected = clinok::default_options<cli1::cli_t>();
    expected.myint2 = 1;
    expected.color = cli1::color_e::blue;
    test_parse1_value(
        {
            "program_name_placeholder",
            "--myint2",
            "1",
            "-c",
            "blue",
        },
        expected);
  }

  test_parse1(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "--color",
          "black",
      },
      clinok::errc::invalid_argument,
      "invalid argument when parsing \"--color\". Possible values are: [red, green, blue, yellow]\n");

  test_parse1(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-c",
          "black",
      },
      clinok::errc::invalid_argument,
      "invalid argument when parsing \"-c\" resolved as \"--color\". Possible values are: [red, green, "
      "blue, yellow]\n");
  test_parse1(
      {
          "program_name_placeholder",
          "myint2",
          "1",
          "-c",
          "black",
      },
      clinok::errc::disallowed_free_arg, "disallowed free arg \"myint2\"\n");
  {
    cli2::options expected = clinok::default_options<cli2::cli_t>();
    expected.myname = "name";
    test_parse2_value(
        {
            "program_name_placeholder",
            "--myname",
            "name",
        },
        expected);
  }
  {
    cli2::options expected = clinok::default_options<cli2::cli_t>();
    expected.myname = "name";
    expected.additional_args = {"arg1", "arg2"};
    test_parse2_value(
        {
            "program_name_placeholder",
            "--myname",
            "name",
            "arg1",
            "arg2",
        },
        expected);
  }
  {
    cli2::options expected = clinok::default_options<cli2::cli_t>();
    expected.myname = "name";
    expected.additional_args = {"arg1", "arg2", "arg3"};
    test_parse2_value(
        {
            "program_name_placeholder",
            "arg1",
            "--myname",
            "name",
            "arg2",
            "arg3",
        },
        expected);
  }
  {
    cli2::options expected = clinok::default_options<cli2::cli_t>();
    expected.myname = "name";
    expected.additional_args = {"arg1", "arg2"};
    test_parse2_value(
        {
            "program_name_placeholder",
            "arg1",
            "arg2",
            "--myname",
            "name",
        },
        expected);
  }
  test_parse2(
      {
          "program_name_placeholder",
          "-",
          "--myname",
          "name",
      },
      clinok::errc::option_missing, "option name is missing, \"-\" used instead");
  test_parse2(
      {
          "program_name_placeholder",
          "--",
          "--myname",
          "name",
      },
      clinok::errc::option_missing, "option name is missing, \"--\" used instead");
  test_parse2(
      {
          "program_name_placeholder",
          "--mynamr",
          "name",
      },
      clinok::errc::unknown_option, "unknown option \"--mynamr\" you probably meant \"--myname\" option\n");

  test_parse3({"program_name_placeholder", "-l", "info"}, clinok::errc::required_option_not_present,
              "required option \"user\" is missing\n");

  test_parse3({"program_name_placeholder", "--timeout"}, clinok::errc::argument_missing,
              "argument missing when parsing \"--timeout\"\n");

  test_parse3({"program_name_placeholder", "--timeout", "5"}, clinok::errc::required_option_not_present,
              "required option \"user\" is missing\n");

  {
    cli3::options expected = clinok::default_options<cli3::cli_t>();
    expected.log_level = cli3::log_level_e::info;
    expected.user = "alice";
    expected.location = Point{.x = 10, .y = 20};
    test_parse3_value(
        {"program_name_placeholder", "--user", "alice", "--location", "10", "20", "--log-level", "info"},
        expected);
  }
  std::string help1 = get_help<cli1::cli_t>(), help2 = get_help<cli2::cli_t>(),
              help3 = get_help<cli3::cli_t>();
  assert_eq(expected_help1, help1);
  assert_eq(expected_help2, help2);
  assert_eq(expected_help3, help3);

  static_assert(!cli1::cli_t::allow_additional_args && cli2::cli_t::allow_additional_args);
  cli1::options o1;
  // should compile
  use(o1.mytag, o1.works, o1.hello_world, o1.myname, o1.ABC2, o1.myint, o1.myint2, o1.color);
  // default values
  error_if(o1.myint != 0 || o1.myint2 != 0 || o1.ABC2 != "" || o1.myname != "" || o1.hello_world != "" ||
           o1.works != false || o1.mytag != false || o1.color != cli1::color_e{});
  cli2::options o2;
  use(o2.mytag, o2.works, o2.hello_world, o2.myname, o2.ABC2);

  test_select_subprogram({"git", "status", "abc"}, "git", {"status", "branch"}, "", 0);
  test_select_subprogram({"git", "status", "abc"}, "git", {"branch", "status"}, "", 1);
  test_select_subprogram({"git", "status", "abc"}, "git", {"status"}, "", 0);
  test_select_subprogram({"git", "statu", "abc"}, "git", {"branch", "status"}, R"(Usage: git <subprogram>
valid subprograms list:
branch
status
)",
                         -2);

  cli3::options o3;
  use(o3.location, o3.log_level, o3.timeout, o3.user);

  return 0;
}

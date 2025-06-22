#define program_options_file "../tests/program_options.def"
#define CLINOK_NAMESPACE_NAME cli1
#include <clinok/cli_interface.hpp>

#define program_options_file "../tests/program2_options.def"
#define CLINOK_NAMESPACE_NAME cli2

#include <clinok/cli_interface.hpp>

#include <iostream>
#include <sstream>

#include <clinok/utils.hpp>

#define error_if(...)                          \
  if ((__VA_ARGS__)) {                         \
    std::cout << "ERROR ON LINE " << __LINE__; \
    std::exit(__LINE__);                       \
  }

void use(auto&&...) {
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

void test_parse(std::vector<const char*> argv, clinok::errc expected_err, std::string expected_msg) {
  clinok::error_code ec;
  cli1::options o = cli1::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  if (expected_err != clinok::errc::ok)
    error_if(!ec);
  error_if(ec.what != expected_err);
  std::stringstream str;
  cli1::print_err_to(ec, [&](auto x) { str << x; });
  auto s = str.str();
  error_if(s != expected_msg);
}

int main() {
  auto n = cli1::color_o::names;
  test_levenshtein_distance();
  test_parse(
      {
          "program_name_placeholder",
          "--a",
      },
      clinok::errc::unknown_option, "unknown option --a you probably meant the -w alias\n");
  test_parse(
      {
          "program_name_placeholder",
          "--ab",
      },
      clinok::errc::unknown_option, "unknown option --ab you probably meant the --abc option\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
      },
      clinok::errc::argument_missing, "argument missing when parsing --abc\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "0",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing --abc. Possible values are: 1 2 3 4 5 \n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
      },
      clinok::errc::required_option_not_present, "required option myint2 is missing\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
          "--myint2",
          "1",
      },
      clinok::errc::ok, "");
  // with alias
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
          "-i",
          "1",
      },
      clinok::errc::ok, "");

  // with alias 'hh' to 'abc'
  test_parse(
      {
          "program_name_placeholder",
          "-hh",
      },
      clinok::errc::argument_missing, "argument missing when parsing -hh resolved as abc\n");
  test_parse(
      {
          "program_name_placeholder",
          "-hh",
          "0",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing -hh resolved as abc. Possible values are: 1 2 3 4 5 \n");
  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-hh",
          "1",
      },
      clinok::errc::ok, "");

  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "--color",
          "red",
      },
      clinok::errc::ok, "");

  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-c",
          "blue",
      },
      clinok::errc::ok, "");

  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "--color",
          "black",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing --color. Possible values are: red green blue yellow \n");

  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-c",
          "black",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing -c resolved as color. Possible values are: red green blue yellow "
      "\n");

  cli1::options o1;
  // should compile
  use(o1.mytag, o1.works, o1.hello_world, o1.myname, o1.ABC2, o1.abc, o1.str_enum, o1.myint, o1.myint2,
      o1.color);
  // default values
  error_if(o1.myint != 17 || o1.myint2 != 0 || o1.str_enum != "hello" || o1.abc != 1 || o1.ABC2 != "why" ||
           o1.myname != "" || o1.hello_world != "hello, man" || o1.works != false || o1.mytag != false ||
           o1.color != cli1::color_e::red);
  cli2::options o2;
  use(o2.mytag, o2.works, o2.hello_world, o2.myname, o2.ABC2, o2.str_enum);
}

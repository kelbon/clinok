#define program_options_file "../tests/program_options.def"
#define CLINOK_NAMESPACE_NAME cli1
#include <clinok/cli_interface.hpp>

#define program_options_file "../tests/program2_options.def"
#define CLINOK_NAMESPACE_NAME cli2

#include <clinok/cli_interface.hpp>

#include <iostream>
#include <sstream>

#define error_if(...)                          \
  if ((__VA_ARGS__)) {                         \
    std::cout << "ERROR ON LINE " << __LINE__; \
    std::exit(__LINE__);                       \
  }

void use(auto&&...) {
}

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

void test_parse2(std::vector<const char*> argv, clinok::errc expected_err, std::string expected_msg,
                 std::vector<std::string_view> expected_free_args) {
  clinok::error_code ec;
  cli2::options o = cli2::parse(clinok::args_range(argv.size(), (char**)argv.data()), ec);
  if (expected_err != clinok::errc::ok)
    error_if(!ec);
  error_if(ec.what != expected_err);
  std::stringstream str;
  cli2::print_err_to(ec, [&](auto x) { str << x; });
  auto s = str.str();
  error_if(s != expected_msg);
  error_if(o.additional_args != expected_free_args);
}

void test_select_subprogram(std::vector<const char*> vecargs, std::string_view progname,
                            std::initializer_list<std::string_view> subprogram_names,
                            std::string expected_msg, int expected_index) {
  int argc = vecargs.size();
  char** argv = (char**)vecargs.data();
  std::stringstream ssm;
  int i = clinok::select_subprogram(argc, argv, progname, subprogram_names, ssm);
  auto msg = ssm.str();
  error_if(msg != expected_msg);
  error_if(i != expected_index);
}

int main() {
  test_trim_ws();
  test_split_by_comma();
  test_levenshtein_distance();
  test_parse(
      {
          "program_name_placeholder",
          "--a",
      },
      clinok::errc::unknown_option, "unknown option \"--a\" you probably meant \"-w\" alias\n");
  test_parse(
      {
          "program_name_placeholder",
          "--ab",
      },
      clinok::errc::unknown_option, "unknown option \"--ab\" you probably meant \"--abc\" option\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
      },
      clinok::errc::argument_missing, "argument missing when parsing \"--abc\"\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "0",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing \"--abc\". Possible values are: 1 2 3 4 5 \n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
      },
      clinok::errc::required_option_not_present, "required option \"color\" is missing\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
          "--color",
          "blue",
      },
      clinok::errc::required_option_not_present, "required option \"myint2\" is missing\n");
  test_parse(
      {
          "program_name_placeholder",
          "--abc",
          "1",
          "--myint2",
          "1",
          "--color",
          "blue",
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
          "-c",
          "yellow",
      },
      clinok::errc::ok, "");

  // with alias 'hh' to 'abc'
  test_parse(
      {
          "program_name_placeholder",
          "-hh",
      },
      clinok::errc::argument_missing, "argument missing when parsing \"-hh\" resolved as \"--abc\"\n");
  test_parse(
      {
          "program_name_placeholder",
          "-hh",
          "0",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing \"-hh\" resolved as \"--abc\". Possible values are: 1 2 3 4 5 \n");
  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-hh",
          "1",
          "-c",
          "red",
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
      "impossible enum value when parsing \"--color\". Possible values are: red green blue yellow \n");

  test_parse(
      {
          "program_name_placeholder",
          "--myint2",
          "1",
          "-c",
          "black",
      },
      clinok::errc::impossible_enum_value,
      "impossible enum value when parsing \"-c\" resolved as \"--color\". Possible values are: red green "
      "blue yellow "
      "\n");
  test_parse(
      {
          "program_name_placeholder",
          "myint2",
          "1",
          "-c",
          "black",
      },
      clinok::errc::disallowed_free_arg, "disallowed free arg \"myint2\"\n");
  test_parse2(
      {
          "program_name_placeholder",
          "--myname",
          "name",
      },
      clinok::errc::ok, "", {});
  test_parse2(
      {
          "program_name_placeholder",
          "--myname",
          "name",
          "arg1",
          "arg2",
      },
      clinok::errc::ok, "", {"arg1", "arg2"});
  test_parse2(
      {
          "program_name_placeholder",
          "arg1",
          "--myname",
          "name",
          "arg2",
          "arg3",
      },
      clinok::errc::ok, "", {"arg1", "arg2", "arg3"});
  test_parse2(
      {
          "program_name_placeholder",
          "arg1",
          "arg2",
          "--myname",
          "name",
      },
      clinok::errc::ok, "", {"arg1", "arg2"});
  test_parse2(
      {
          "program_name_placeholder",
          "-",
          "--myname",
          "name",
      },
      clinok::errc::option_missing, "option name is missing, \"-\" used instead", {});
  test_parse2(
      {
          "program_name_placeholder",
          "--",
          "--myname",
          "name",
      },
      clinok::errc::option_missing, "option name is missing, \"--\" used instead", {});
  test_parse2(
      {
          "program_name_placeholder",
          "--mynamr",
          "name",
      },
      clinok::errc::unknown_option, "unknown option \"--mynamr\" you probably meant \"--myname\" option\n",
      {});
  static_assert(cli1::is_required_option<cli1::color_o>::value);
  static_assert(cli1::is_required_option<cli1::myint2_o>::value);
  static_assert(!cli1::is_required_option<cli1::ABC2_o>::value);
  static_assert(!cli1::allow_additional_args && cli2::allow_additional_args);
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

  test_select_subprogram({"git", "status", "abc"}, "git", {"status", "branch"}, "", 0);
  test_select_subprogram({"git", "status", "abc"}, "git", {"branch", "status"}, "", 1);
  test_select_subprogram({"git", "status", "abc"}, "git", {"status"}, "", 0);
  test_select_subprogram({"git", "statu", "abc"}, "git", {"branch", "status"}, R"(Usage: git <subprogram>
valid subprograms list:
branch
status
)",
                         -2);
  return 0;
}

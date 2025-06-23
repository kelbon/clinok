
#include <clinok/utils.hpp>

#include <vector>
#include <cstring>
#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <cmath>

namespace clinok {

static double qwerty_cost(char a, char b) {
  // Приводим символы к нижнему регистру
  a = std::tolower(a);
  b = std::tolower(b);

  if (a == b)
    return 0.0;

  // coordinates of keys on qwerty keyboard
  static const std::unordered_map<char, std::pair<int, int>> qwerty_coords = {
      // digits
      {'1', {0, 0}},
      {'2', {0, 1}},
      {'3', {0, 2}},
      {'4', {0, 3}},
      {'5', {0, 4}},
      {'6', {0, 5}},
      {'7', {0, 6}},
      {'8', {0, 7}},
      {'9', {0, 8}},
      {'0', {0, 9}},

      // qwertyuiop
      {'q', {1, 0}},
      {'w', {1, 1}},
      {'e', {1, 2}},
      {'r', {1, 3}},
      {'t', {1, 4}},
      {'y', {1, 5}},
      {'u', {1, 6}},
      {'i', {1, 7}},
      {'o', {1, 8}},
      {'p', {1, 9}},

      // asdfghjkl
      {'a', {2, 0}},
      {'s', {2, 1}},
      {'d', {2, 2}},
      {'f', {2, 3}},
      {'g', {2, 4}},
      {'h', {2, 5}},
      {'j', {2, 6}},
      {'k', {2, 7}},
      {'l', {2, 8}},

      // zxcvbnm
      {'z', {3, 0}},
      {'x', {3, 1}},
      {'c', {3, 2}},
      {'v', {3, 3}},
      {'b', {3, 4}},
      {'n', {3, 5}},
      {'m', {3, 6}},
  };

  auto it_a = qwerty_coords.find(a);
  auto it_b = qwerty_coords.find(b);

  if (it_a == qwerty_coords.end() || it_b == qwerty_coords.end())
    return 1.0;

  const auto& [row_a, col_a] = it_a->second;
  const auto& [row_b, col_b] = it_b->second;

  double row_diff = row_a - row_b;
  double col_diff = col_a - col_b;
  double distance = std::sqrt(row_diff * row_diff + col_diff * col_diff);
  return distance / 2.0;
}

double levenshtein_distance(std::string_view a, std::string_view b) {
  const size_t m = a.size();
  const size_t n = b.size();

  if (m == 0)
    return n;
  if (n == 0)
    return m;

  std::vector<double> prev_row(n + 1);
  std::vector<double> curr_row(n + 1);

  for (size_t i = 0; i <= n; ++i) {
    prev_row[i] = i;
  }

  for (size_t i = 1; i <= m; ++i) {
    curr_row[0] = i;

    for (size_t j = 1; j <= n; ++j) {
      double cost = (a[i - 1] == b[j - 1]) ? 0 : 1;

      curr_row[j] =
          std::min({prev_row[j] + 1, curr_row[j - 1] + 1, prev_row[j - 1] + qwerty_cost(a[i - 1], b[j - 1])});
    }

    prev_row.swap(curr_row);
  }

  return prev_row[n];
}

double damerau_levenshtein_distance(std::string_view a, std::string_view b) {
  const size_t m = a.size();
  const size_t n = b.size();

  if (m == 0)
    return n;
  if (n == 0)
    return m;

  std::vector<double> prev_prev_row(n + 1);
  std::vector<double> prev_row(n + 1);
  std::vector<double> curr_row(n + 1);

  for (size_t i = 0; i <= n; ++i) {
    prev_row[i] = i;
  }

  for (size_t i = 1; i <= m; ++i) {
    curr_row[0] = i;

    for (size_t j = 1; j <= n; ++j) {
      double cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      curr_row[j] =
          std::min({prev_row[j] + 1, curr_row[j - 1] + 1, prev_row[j - 1] + qwerty_cost(a[i - 1], b[j - 1])});

      if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
        curr_row[j] = std::min(curr_row[j], prev_prev_row[j - 2] + 1);
      }
    }
    auto tmp = std::move(prev_prev_row);
    memset(tmp.data(), 0, tmp.size());
    prev_prev_row = std::move(prev_row);
    prev_row = std::move(curr_row);
    curr_row = std::move(tmp);
  }

  return prev_row[n];
}

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
  if (raw_arg == "on" || raw_arg == "ON" || raw_arg == "1" || raw_arg == "yes" || raw_arg == "YES" ||
      raw_arg == "true") {
    b = true;
    return errc::ok;
  } else if (raw_arg == "off" || raw_arg == "OFF" || raw_arg == "0" || raw_arg == "no" || raw_arg == "NO" ||
             raw_arg == "false") {
    b = false;
    return errc::ok;
  }
  return errc::invalid_argument;
}

std::string_view errc2str(errc e) noexcept {
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
    case errc::required_option_not_present:
      return "required option not present";
    case errc::disallowed_free_arg:
      return "disallowed free arg";
    case errc::option_missing:
      return "option missing";
    case errc::ok:
      return "ok";
  }
  return "";
}

}  // namespace clinok

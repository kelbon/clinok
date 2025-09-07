#pragma once

#include <iostream>

#include "clinok/type_descriptor.hpp"

struct Point {
  int x{};
  int y{};

  bool operator==(const Point&) const = default;
};

inline std::ostream& operator<<(std::ostream& out, const Point& p) {
  return out << p.x << " " << p.y;
}

namespace clinok {

template <>
struct type_descriptor<Point> {
  constexpr static std::string_view placeholder() {
    return "<x> <y>";
  }

  static clinok::args_t::iterator parse_option(clinok::args_t::iterator it, clinok::args_t::iterator end,
                                               Point& out, clinok::errc& er) {
    it = type_descriptor<int>::parse_option(it, end, out.x, er);
    if (er != errc::ok) {
      return it;
    }
    it = type_descriptor<int>::parse_option(it, end, out.y, er);

    return it;
  }
};

}  // namespace clinok

#pragma once

#include <iostream>

#include "clinok/descriptor_type.hpp"

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
struct descriptor_type<Point> : default_descriptor_type {
  constexpr static std::string_view placeholder() {
    return "<x> <y>";
  }

  static clinok::args_t::iterator parse(clinok::args_t::iterator it, clinok::args_t::iterator end, Point& out,
                                        clinok::errc& er) {
    it = descriptor_type<int>::parse(it, end, out.x, er);
    if (er != errc::ok) {
      return it;
    }
    it = descriptor_type<int>::parse(it, end, out.y, er);

    return it;
  }
};

}  // namespace clinok

#pragma once

#include <string_view>

#include "clinok/descriptor_type.hpp"

namespace clinok {

template <typename O>
struct default_descriptor_field {
  using T = typename O::value_type;
  using D = descriptor_type<T>;

  constexpr static std::string_view placeholder() {
    return D::placeholder();
  }

  static args_t::iterator parse(args_t::iterator it, args_t::iterator end, auto& out, errc& er) {
    return D::parse(it, end, out, er);
  }

  consteval static std::string_view name() {
    return O::name();
  }

  consteval static std::string_view description() {
    return O::description();
  }

  constexpr static auto& get(auto& x) {
    return O::get(x);
  }

  constexpr static bool has_default() noexcept {
    return O::has_default() || D::has_default();
  }

  constexpr static bool has_implicit_default() noexcept {
    return !O::has_default() && D::has_default();
  }

  constexpr static auto default_value_desc() {
    return D::default_value_desc();
  }

  static void set_default_value(auto& out) {
    if (O::has_default()) {
      return;
    }
    D::set_default_value(out);
  }

  constexpr static bool has_possible_values() {
    return D::has_possible_values();
  }

  constexpr static auto possible_values_desc() {
    return D::possible_values_desc();
  }
};

template <typename O>
struct descriptor_field : default_descriptor_field<O> {};

template <typename Option>
struct is_required_option : std::bool_constant<!descriptor_field<Option>::has_default()> {};

template <typename Option>
struct is_required_option<descriptor_field<Option>> : is_required_option<Option> {};

template <typename Option>
inline constexpr bool is_required_option_v = is_required_option<Option>::value;

}  // namespace clinok

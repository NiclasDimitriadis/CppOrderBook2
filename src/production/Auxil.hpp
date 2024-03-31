#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "param_pack.hpp"

namespace Auxil {
// constexpr-function to compute the smallest power of 2 larger than its
// argument
template <typename uint_>
  requires std::unsigned_integral<uint_>
constexpr uint_ pow_two_ceil(uint_ arg) {
  if (std::has_single_bit(arg))
    return arg;
  else
    return pow_two_ceil<uint_>(arg + 1);
};

template <typename I>
  requires std::unsigned_integral<I>
constexpr I ceil_(I i1, I i2) {
  if (i1 % i2 == 0)
    return i1;
  else
    return ceil_(static_cast<I>(i1 + 1), i2);
};

template <typename I>
  requires std::unsigned_integral<I>
constexpr I divisible_(I i1, I i2) {
  if (i2 % i1 == 0)
    return i1;
  else
    return divisible_(static_cast<I>(i1 + 1), i2);
};

template <typename I>
  requires std::unsigned_integral<I>
constexpr I divisible_or_ceil(I i1, I i2) {
  if (i1 < i2)
    return divisible_(i1, i2);
  else
    return ceil_(i1, i2);
};

template <typename entryType>
constexpr size_t cache_optimal_alignment(size_t cacheline,
                                         bool exclusive_cacheline) {
  return std::max(Auxil::divisible_or_ceil(sizeof(entryType), cacheline),
                  cacheline * exclusive_cacheline);
};

// takes an unsigned integer type as its argument, returns true if argument is <
// 0, false otherwise
template <typename Sint>
  requires std::signed_integral<Sint>
bool is_negative(Sint sint) {
  static constexpr auto shift_by = sizeof(Sint) * 8 - 1;
  if constexpr (std::endian::native == std::endian::little)
    return (std::make_unsigned_t<Sint>)sint >> shift_by == 1;
  else
    return (std::make_unsigned_t<Sint>)sint << shift_by == 1;
}

// concept to enforce a socket wrapper can be called essentially like a POSIX
// socket, via a recv-method
template <typename socketType>
concept ReadableAsSocket =
    requires(socketType socket, void *buffer_ptr, std::uint32_t n_bytes) {
      { socket.recv(buffer_ptr, n_bytes) } -> std::same_as<std::int32_t>;
    };

// generates element-wise additions
template <size_t... index, typename Tuple>
void add_to_tuple_iterate(Tuple &t1, const Tuple &t2,
                          std::index_sequence<index...>) {
  ([&]() { std::get<index>(t1) += std::get<index>(t2); }(), ...);
};

// adds elements of second tuple to respective elements in first tuple
template <typename... Types>
void add_to_tuple(std::tuple<Types...> &t1, const std::tuple<Types...> &t2) {
  add_to_tuple_iterate(t1, t2, std::make_index_sequence<sizeof...(Types)>{});
};

// function template with overloads for all strint-to-arithmetic conversions in
// standard library overloads for integer-types overload for regular int and
// uint as no conversion function for regular sized uint exists
template <typename Arith>
  requires std::integral<Arith> && (sizeof(Arith) <= 4)
Arith str_to_arith(const std::string &arg) {
  return std::stoi(arg);
};

template <typename Arith>
  requires std::signed_integral<Arith> && (sizeof(Arith) == 8)
Arith str_to_arith(const std::string &arg) {
  return std::stol(arg);
};

template <typename Arith>
  requires std::signed_integral<Arith> && (sizeof(Arith) > 8)
Arith str_to_arith(const std::string &arg) {
  return std::stoll(arg);
};

// overloads for unsigned-integer types
template <typename Arith>
  requires std::unsigned_integral<Arith> && (sizeof(Arith) == 8)
Arith str_to_arith(const std::string &arg) {
  return std::stoul(arg);
};

template <typename Arith>
  requires std::unsigned_integral<Arith> && (sizeof(Arith) > 8)
Arith str_to_arith(const std::string &arg) {
  return std::stoull(arg);
};

// overloads for floating point types
template <typename Arith>
  requires std::floating_point<Arith> && (sizeof(Arith) <= 4)
Arith str_to_arith(const std::string &arg) {
  return std::stof(arg);
};

template <typename Arith>
  requires std::floating_point<Arith> && (sizeof(Arith) == 8)
Arith str_to_arith(const std::string &arg) {
  return std::stod(arg);
};

template <typename Arith>
  requires std::floating_point<Arith> && (sizeof(Arith) > 8)
Arith str_to_arith(const std::string &arg) {
  return std::stold(arg);
};
}; // namespace Auxil

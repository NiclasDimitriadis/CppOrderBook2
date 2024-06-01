#pragma once

#include <concepts>
#include <type_traits>

namespace helpers{
template <typename T>
concept constexpr_bool_value = requires() {
  { T::value } -> std::same_as<const bool &>;
};

template<typename T1, typename T2>
requires constexpr_bool_value<T1> && constexpr_bool_value<T2>
struct And{
  static constexpr bool value = T1::value && T2::value;
};

template<typename T1, typename T2>
requires constexpr_bool_value<T1> && constexpr_bool_value<T2>
struct Or{
  static constexpr bool value = T1::value || T2::value;
};

template<typename T1, typename T2>
requires constexpr_bool_value<T1> && constexpr_bool_value<T2>
struct Xor{
  static constexpr bool value = T1::value != T2::value;
};

// determines whether some type is a specialization of a certain class template with type template parameters
template<template <typename...> class, typename>
struct specializes_class_template : std::false_type{};

template<template <typename...> class class_template, typename... Ts>
struct specializes_class_template<class_template, class_template<Ts...>> : std::true_type{};

template<template <typename...> class class_template, typename type>
constexpr inline bool specializes_class_template_v = specializes_class_template<class_template, std::remove_cvref_t<type>>::value;

// determines whether some type is a specialization of a certain class template with a non-type template parameter pack
template<template <auto...> class, typename>
struct specializes_class_template_nt : std::false_type{};

template<template <auto...> class class_template, auto... vals>
struct specializes_class_template_nt<class_template, class_template<vals...>> : std::true_type{};

template<template <auto...> class class_template, typename type>
constexpr inline bool specializes_class_template_nt_v = specializes_class_template_nt<class_template, std::remove_cvref_t<type>>::value;

// determines whether some type is a specialization of a certain class template with a type paramater and a pack of non-type paramaters of the type specified by the type paramater (e.g. std::integer_sequence)
template<template <typename, auto ...> class, typename, typename = void>
struct specializes_class_template_tnt : std::false_type{};

template<template <typename T, T...> class class_template, typename Arg, Arg... vals>
struct specializes_class_template_tnt<class_template, class_template<Arg, vals...>> : std::true_type{};

template<template <typename T, T...> class class_template, typename type>
constexpr inline bool specializes_class_template_tnt_v = specializes_class_template_tnt<class_template, std::remove_cvref_t<type>>::value;

};

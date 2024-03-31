#pragma once

#include "helpers.hpp"
#include "param_pack.hpp"
#include "non_rep_combinations.hpp"

// monoid definition found at:
// https://www.boost.org/doc/libs/1_77_0/libs/graph/doc/Monoid.html

namespace monoidal_class_template {

template<template <typename...> class, typename>
struct closure_class_template{
  static_assert(false);
};

// check the closure-property necessary for a class template to be monoidal

template <template <typename...> class Templ, typename A, typename B>
struct closure_class_template <Templ, param_pack::type_pack_t<A,B>>{
  static constexpr bool value =
      helpers::specializes_class_template_v<Templ, Templ<Templ<A>, Templ<B>>>;
};

template <template <typename...> class Class_Templ, size_t order, typename... Ts>
struct check_closure_property: check_closure_property<Class_Templ, order, param_pack::type_pack_t<Ts...>>{};

template <template <typename...> class Class_Templ, size_t order, typename Type_Pack>
requires param_pack::type_pack_convertible_v<Type_Pack>
&& (param_pack::generate_type_pack_t<Type_Pack>::size >= 2)
&& (order > 0)
struct check_closure_property<Class_Templ,order,Type_Pack> {
private:
  // template for checking closure property w.r.t. one pair of monoid spcializations
  template<typename type_pack> using closure_pair = closure_class_template<Class_Templ, type_pack>;

  // convert Type_Pack paramter into param_pack::type_pack_t if it isn't already
  using type_pack = param_pack::generate_type_pack_t<Type_Pack>;
  // apply monoid to all non-rep combinations of length order
  using monoid_applied = type_pack::template apply_templ_to_nr_combs_t<Class_Templ, order>;
  // generate indices for all non repeating type pairs from Type_Pack
  using pair_combination_indices =
          param_pack::generate_non_type_pack_t<non_rep_combinations::non_rep_index_combinations_t<
          type_pack::size, 2>>;
  // generate type pairs according to indices
  using pair_combinations = monoid_applied::template split_t<pair_combination_indices,2>;
  // apply closure check to all pairs
  using combinations_checks = pair_combinations::template functor_map_t<closure_pair>;

public:
  // sum up check results of all combinations
  static constexpr bool value = combinations_checks::template fold_t<helpers::And, std::true_type>::value;
};

template <template <typename... Ts> class Class_Templ, size_t order, typename... Ts>
constexpr inline bool check_closure_property_v =
    check_closure_property<Class_Templ, order, Ts...>::value;

// check for assiociativity
// to that end, we need to instroduce a comparator type that determines whether
// to template specializations are equivalent even when they're not reconized as
// the same type by the compiler
template <template <typename...> class,
          template <typename, typename> class, typename>
struct associative_class_template{
  static_assert(false);
};

template <template <typename...> class Class_Templ,
          template <typename, typename> class Comparator, typename A,
          typename B, typename C>
requires helpers::specializes_class_template_v<Class_Templ, A>
&& helpers::specializes_class_template_v<Class_Templ, B>
&& helpers::specializes_class_template_v<Class_Templ, C>
&& helpers::constexpr_bool_value<Comparator<
      Class_Templ<Class_Templ<A,B>,C>,
      Class_Templ<A, Class_Templ<B,C>>>>
struct associative_class_template<Class_Templ, Comparator, param_pack::type_pack_t<A,B,C>> {
  static constexpr bool value = Comparator<
      Class_Templ<Class_Templ<A,B>,C>,
      Class_Templ<A, Class_Templ<B,C>>>::value;
};


template <template <typename...> class Class_Templ,
          template <typename, typename> class Comparator, size_t order, typename... Ts>
struct check_associativity_property: check_associativity_property<Class_Templ, Comparator, order, param_pack::type_pack_t<Ts...>>{};

template <template <typename...> class Class_Templ,
          template <typename, typename> class Comparator, size_t order, typename Type_Pack>
          requires param_pack::type_pack_convertible_v<Type_Pack>
&& (param_pack::generate_type_pack_t<Type_Pack>::size >= 3)
&& (order > 0)
struct check_associativity_property<Class_Templ, Comparator, order, Type_Pack> {
private:
  template<typename type_pack>
  using associative_triple = associative_class_template<Class_Templ, Comparator, type_pack>;

  // convert Type_Pack paramter into param_pack::type_pack_t if it isn't already
  using type_pack = param_pack::generate_type_pack_t<Type_Pack>;
  // apply monoid to all non-rep combinations of length order
  using monoid_applied = type_pack::template apply_templ_to_nr_combs_t<Class_Templ, order>;
  // generate indices for all non repeating type triples from Type_Pack
  using combination_indices = param_pack::generate_non_type_pack_t<non_rep_combinations::non_rep_index_combinations_t<
          monoid_applied::size, 3>>;
  // generate type triples according to indices
  using combinations = monoid_applied::template split_t<combination_indices,3>;
  // apply closure check to all triples
  using combinations_checks = combinations::template functor_map_t<associative_triple>;

public:
  // sum up check results of all combinations
  static constexpr bool value = combinations_checks::template fold_t<helpers::And, std::true_type>::value;
};

template <template <typename... > class Class_Templ,
          template <typename, typename> class Comparator, size_t order, typename... Ts>
constexpr inline bool check_associativity_property_v =
    check_associativity_property<Class_Templ, Comparator, order, Ts...>::value;

// use omission of type template-parameter as identity element
template <template <typename... > class Class_Templ,
          template <typename, typename> class Comparator, typename A>
requires helpers::specializes_class_template_v<Class_Templ, A>
&& helpers::constexpr_bool_value<Comparator<A, Class_Templ<Class_Templ<A>>>>
struct has_identity {
  static constexpr bool value =
      Comparator<A, Class_Templ<Class_Templ<A>>>::value;
};


template <template <typename...> class Class_Templ,
          template <typename, typename> class Comparator, size_t order, typename... Ts>
struct check_identity_element_property: check_identity_element_property<Class_Templ, Comparator, order, param_pack::type_pack_t<Ts...>>{};

template <template <typename...> class Class_Templ,
          template <typename, typename> class Comparator, size_t order, typename Type_Pack>
requires param_pack::type_pack_convertible_v<Type_Pack>
&& (param_pack::generate_type_pack_t<Type_Pack>::size >= 1)
&& (order > 0)
struct check_identity_element_property<Class_Templ, Comparator, order, Type_Pack> {
private:
  template<typename A>
  using identity_type = has_identity<Class_Templ, Comparator, A>;

  // convert Type_Pack paramter into param_pack::type_pack_t if it isn't already
  using type_pack = param_pack::generate_type_pack_t<Type_Pack>;
  using monoid_applied = type_pack::template apply_templ_to_nr_combs_t<Class_Templ, order>;
  // apply identity check to all types
  using checks = monoid_applied::template functor_map_t<identity_type>;

public:
  // sum up check results of all combinations
  static constexpr bool value = checks::template fold_t<helpers::And, std::true_type>::value;
};


template <template <typename... > class Class_Templ,
          template <typename M1, typename M2> class Comparator, size_t order, typename... Ts>
constexpr inline bool check_identity_element_property_v =
    check_identity_element_property<Class_Templ, Comparator, order, Ts...>::value;

template <template <typename... > class Class_Templ,
          template <typename MA, typename MB> class Comparator, size_t order, typename... Ts>
struct monoidal_class_template {
  static constexpr bool value =
      check_closure_property_v<Class_Templ, order, Ts...> &&
      check_associativity_property_v<Class_Templ, Comparator, order, Ts...> &&
      check_identity_element_property_v<Class_Templ, Comparator, order, Ts...>;
};

template <template <typename... > class Class_Templ,
          template <typename MA, typename MB> class Comparator, size_t order, typename... Ts>
constexpr inline bool monoidal_class_template_v = monoidal_class_template<Class_Templ, Comparator, order, Ts...>::value;
}; // namespace monoidal_class_template

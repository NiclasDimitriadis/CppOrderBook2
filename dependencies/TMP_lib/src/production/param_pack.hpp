#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "helpers.hpp"
#include "non_rep_combinations.hpp"

namespace param_pack {

// extracts the i-th type from a type-parameter pack
template <size_t i, typename... Ts>
  requires(i < sizeof...(Ts))
struct t_pack_index {
private:
  template <size_t at_index, size_t target_index, typename T, typename... Ts_>
  struct pack_index_logic {
    using type = pack_index_logic<at_index + 1, target_index, Ts_...>::type;
  };

  template <size_t target_index, typename T, typename... Ts_>
  struct pack_index_logic<target_index, target_index, T, Ts_...> {
    using type = T;
  };

public:
  using type = pack_index_logic<0, i, Ts...>::type;
};

template <size_t i, typename... Ts>
using pack_index_t = t_pack_index<i, Ts...>::type;

// extracts i-th element from non-type parameter pack
template <size_t i, auto... vs>
requires(i < sizeof...(vs))
struct nt_pack_index {

private:
  template <size_t at_index, size_t target_index, auto v1, auto... vs_>
  struct nt_pack_index_logic {
    static constexpr auto value =
        nt_pack_index_logic<at_index + 1, target_index, vs_...>::value;
  };

  template <size_t target_index, auto v1, auto... vs_>
  struct nt_pack_index_logic<target_index, target_index, v1, vs_...> {
    static constexpr auto value = v1;
  };

public:
  static constexpr auto value = nt_pack_index_logic<0, i, vs...>::value;
};

template <size_t i, auto... vs>
constexpr inline auto pack_index_v = nt_pack_index<i, vs...>::value;


// determines whether all elements is a non-type parameter pack are of the same type
template<auto... is>
struct single_type_nt_pack{
  static_assert(false);
};

template<auto i1, auto i2, auto... is>
struct single_type_nt_pack<i1,i2,is...>{
  static constexpr bool value = std::is_same_v<decltype(i1), decltype(i2)> && single_type_nt_pack<i2, is...>::value;
};

template<auto i>
struct single_type_nt_pack<i>{
  static constexpr bool value = true;
};

template<auto... Is>
constexpr inline bool single_type_nt_pack_v = single_type_nt_pack<Is...>::value;


// deduces the type of a non-type parameter pack
template <auto... is>
  requires(sizeof...(is) > 0) && single_type_nt_pack_v<is...>
struct pack_type {
  using type = decltype(pack_index_v<0,is...>);
};

template <auto... is>
using pack_type_t = typename pack_type<is...>::type;


template<typename T, T... vals>
struct non_type_pack{
private:
    template<typename, auto...>
    struct append_logic{
      static_assert(false);
    };

    template<T... app_vals>
    struct append_logic<non_type_pack<T, app_vals...>>{
      using type = non_type_pack<T, vals..., app_vals...>;
    };

    template<size_t n, auto...>
    requires (n <= sizeof...(vals))
    struct truncate_front_logic{
      static_assert(false);
    };

    template<size_t n, T fst, T... tail>
    requires (sizeof...(tail) != sizeof...(vals) - n - 1)
    struct truncate_front_logic<n, fst, tail...>{
      using type = truncate_front_logic<n, tail...>::type;
    };

    template<T... trunc_vals>
    struct truncate_front_logic<sizeof...(vals), trunc_vals...>{
      using type = non_type_pack<T>;
    };

   template<T... tail>
    struct truncate_front_logic<sizeof...(vals) - sizeof...(tail), tail...>{
      using type = non_type_pack<T, tail...>;
    };

    template<size_t n, auto...>
    requires (n <= sizeof...(vals))
    struct truncate_back_logic{
      static_assert(false);
    };

    template<size_t n, T fst, T... tail>
    struct truncate_back_logic<n, fst, tail...>{
        using type = non_type_pack<T, fst>::template append_t<typename truncate_back_logic<n, tail...>::type>;
    };

    template<T fst, T... tail>
    struct truncate_back_logic<sizeof...(tail), fst, tail...>{
        using type = non_type_pack<T, fst>;
    };

    template<size_t index, auto... vals_>
    struct reverse_logic{
      using type = non_type_pack<T, pack_index_v<index, vals_...>>::template append_t<typename reverse_logic<index - 1, vals_...>::type>;
    };

    template<size_t index, auto... vals_>
    requires (sizeof...(vals_) == 0)
    struct reverse_logic<index, vals_...>{
      using type = non_type_pack<T>;
    };

    template <auto... vals_>
    struct reverse_logic<0, vals_...>{
      using type = non_type_pack<T, pack_index_v<0, vals_...>>;
    };

    template<auto...>
    struct functor_map_logic{
      static_assert(false);
    };

    template<auto callable, T fst, T... tail>
    struct functor_map_logic<callable, fst, tail...>{
      private:
      using F_Type = decltype(callable(std::declval<T>()));
      public:
      using type =
       non_type_pack<F_Type, callable(fst)>::template append_t<typename functor_map_logic<callable, tail...>::type>;
    };

    template<auto callable>
    struct functor_map_logic<callable>{
      using type = non_type_pack<decltype(callable(std::declval<T>()))>;
    };

    template<auto callable>
    requires std::is_invocable_v<decltype(callable), T> && (!std::is_same_v<decltype(callable(std::declval<T>())), void>)
    struct functor_map{
      using type = functor_map_logic<callable, vals...>::type;
    };

    template<template<auto...> class, auto...>
    struct monadic_bind_logic{
      static_assert(false);
    };

    template<template<T...> class Class_Template, T fst, T... tail>
    requires helpers::specializes_class_template_tnt_v<non_type_pack, Class_Template<fst>>
    struct monadic_bind_logic<Class_Template, fst, tail...>{
      using type = Class_Template<fst>::template append_t<typename monadic_bind_logic<Class_Template, tail...>::type>;
    };

    template<template<T...> class Class_Template, T last>
    requires helpers::specializes_class_template_tnt_v<non_type_pack, Class_Template<last>>
    struct monadic_bind_logic<Class_Template, last>{
      using type = Class_Template<last>;
    };

    template<template<T...> class Class_Template>
    requires (sizeof...(vals) > 0)
    struct monadic_bind{
      using type = monadic_bind_logic<Class_Template, vals...>::type;
    };

    template<auto fold_by, T first, T second, T... tail>
    requires std::is_invocable_r_v<T,decltype(fold_by),T,T>
    struct fold_logic{
      static constexpr T value = fold_logic<fold_by, fold_by(first, second), tail...>::value;
    };

    template<auto fold_by, T second_to_last, T last>
    struct fold_logic<fold_by, second_to_last, last>{
      static constexpr T value = fold_by(second_to_last, last);
    };

public:
    using type = T;
    static constexpr size_t size = sizeof...(vals);

    // appends another non_type_pack
    template<typename Append_Pack>
    using append_t = append_logic<Append_Pack>::type;

    // removes first n entries
    template<size_t n>
    using truncate_front_t = truncate_front_logic<n, vals...>::type;

    // removes last n entries
    template<size_t n>
    using truncate_back_t = truncate_back_logic<n, vals...>::type;

    // yields first n elements
    template<size_t n>
    using head_t = truncate_back_t<sizeof...(vals) - n>;

    // yields last n elements
    template<size_t n>
    using tail_t = truncate_front_t<sizeof...(vals) - n>;

    // extract i-th entry in parameter pack
    template<size_t i>
    static constexpr T index_v = pack_index_v<i, vals...>;

    // uses vals as non-type arguments to another template
    template<template <auto...> class Class_Template>
    using specialize_template_t = Class_Template<vals...>;

    // uses T as type argument and vals as non-type arguments to another template
    template<template <typename U, U...> class Class_Template>
    using specialize_type_param_template_t = Class_Template<T, vals...>;

    // yields non_type_pack with entries in reverse order
    using reverse_t = reverse_logic<sizeof...(vals) - 1, vals...>::type;

    // applies a callable object to every singe entry in vals in a functorial way
    template<auto callable>
    using functor_map_t = functor_map<callable>::type;

    // applies a class template to all entries in vals in a monadic way
    template<template<T...> class Class_Template>
    using monadic_bind_t = monadic_bind<Class_Template>::type;

    // folds vals... into a single value
    template<auto fold_by, T init>
    static constexpr T fold_v = fold_logic<fold_by, init, vals...>::value;

    using drop_const_t = non_type_pack<std::remove_const_t<type>, vals...>;
};

template<auto... vals>
using non_type_pack_t = non_type_pack<pack_type_t<vals...>, vals...>;

template<typename>
struct generate_non_type_pack{};

template<template<auto...> class from_template, auto... vals_>
struct generate_non_type_pack<from_template<vals_...>>{
    using type = param_pack::non_type_pack_t<vals_...>;
};

template<template<typename, auto...> class from_template, typename T, T... vals_>
struct generate_non_type_pack<from_template<T, vals_...>>{
    using type = param_pack::non_type_pack_t<vals_...>;
};

template<typename T>
using generate_non_type_pack_t = generate_non_type_pack<T>::type;

// metafunction to determine whether a type can be used to generate a non_type_pack_t via generate_non_type_pack_t
template<typename, typename = void>
struct non_type_pack_convertible: std::false_type{};

template<typename T>
struct non_type_pack_convertible<T, std::void_t<generate_non_type_pack_t<T>>>: std::true_type{};

template<typename T>
constexpr inline bool non_type_pack_convertible_v = non_type_pack_convertible<T>::value;


template<typename... Ts>
struct type_pack_t{
private:
  template<typename>
  struct append_logic{
    static_assert(false);
  };

  template<typename... Us>
  struct append_logic<type_pack_t<Us...>>{
    using type = type_pack_t<Ts..., Us...>;
  };

  template<size_t n, typename... Us>
  requires (n <= sizeof...(Us))
  struct truncate_front_logic{
    static_assert(false);
  };

  // empty pack type if n equals the number of type contained
  template<typename... Us>
  struct truncate_front_logic<sizeof...(Ts), Us...>{
    using type = type_pack_t<>;
  };

  // regular recursion step
  template<size_t n, typename U, typename... Us>
  requires(sizeof...(Us) != sizeof...(Ts) - n - 1)
  struct truncate_front_logic<n, U, Us...>{
    using type = truncate_front_logic<n, Us...>::type;
  };

  // stop recursion when n first entries have been removed
  template<typename... Us>
  struct truncate_front_logic<sizeof...(Ts) - sizeof...(Us), Us...>{
    using type = type_pack_t<Us...>;
  };

  template<size_t n, typename...>
  requires (n <= sizeof...(Ts))
  struct truncate_back_logic{
    static_assert(false);
  };

  template<size_t n, typename U, typename... Us>
  struct truncate_back_logic<n, U, Us...>{
    using type = type_pack_t<U>::template append_t<typename truncate_back_logic<n, Us...>::type>;
  };

  template<typename U, typename... Us>
  struct truncate_back_logic<sizeof...(Us), U, Us...>{
    using type = type_pack_t<U>;
  };

  template<size_t index, typename... Us>
  struct reverse_logic{
    using type = type_pack_t<pack_index_t<index, Us...>>::template append_t<typename reverse_logic<index - 1, Us...>::type>;
  };

  template<typename... Us>
  struct reverse_logic<0, Us...>{
    using type = type_pack_t<pack_index_t<0, Us...>>;
  };

  template<template<typename...> class, typename...>
  struct functor_map_logic{
    static_assert(false);
  };

  template<template<typename...> class Class_Template, typename Fst, typename... Tail>
  struct functor_map_logic<Class_Template, Fst, Tail...>{
    using type = type_pack_t<Class_Template<Fst>>::template append_t<typename functor_map_logic<Class_Template, Tail...>::type>;
  };

  template<template<typename...> class Class_Template, typename Last>
  struct functor_map_logic<Class_Template, Last>{
    using type = type_pack_t<Class_Template<Last>>;
  };

  template<template<typename...> class Class_Template>
  requires (sizeof...(Ts) > 0)
  struct functor_map{
    using type = functor_map_logic<Class_Template, Ts...>::type;
  };

  template<template<typename...> class, typename...>
  struct monadic_bind_logic{
    static_assert(false);
  };

  template<template<typename...> class Class_Template, typename Fst, typename... Tail>
  requires helpers::specializes_class_template_v<type_pack_t, Class_Template<Fst>>
  struct monadic_bind_logic<Class_Template, Fst, Tail...>{
    using type = Class_Template<Fst>::template append_t<typename monadic_bind_logic<Class_Template, Tail...>::type>;
  };

  template<template<typename...> class Class_Template, typename Last>
  requires helpers::specializes_class_template_v<type_pack_t, Class_Template<Last>>
  struct monadic_bind_logic<Class_Template, Last>{
    using type = Class_Template<Last>;
  };

  template<template<typename...> class Class_Template>
  requires(sizeof...(Ts) > 0)
  struct monadic_bind{
    using type = monadic_bind_logic<Class_Template, Ts...>::type;
  };

  template<size_t fst, size_t... tail>
  struct subset{
    using type = type_pack_t<pack_index_t<fst,Ts...>>::template append_t<typename subset<tail...>::type>;
  };

  template<size_t last>
  struct subset<last>{
    using type = type_pack_t<pack_index_t<last,Ts...>>;
  };

  template<size_t... is>
  using subset_t = subset<is...>::type;

  template<typename>
  struct subset_from_pack{
    static_assert(false);
  };

  template<size_t... is>
  struct subset_from_pack<non_type_pack_t<is...>>{
    using type = subset_t<is...>;
  };

  template<typename NT_Pack>
  using subset_from_pack_t = subset_from_pack<NT_Pack>::type;

  template<typename T_, size_t>
  struct split_logic{
    static_assert(false);
  };

  template<size_t len, typename Uint, size_t... is>
  requires (sizeof...(is) % len == 0) && (sizeof...(is) > len)
  && std::unsigned_integral<Uint>
  struct split_logic<non_type_pack<Uint, is...>, len>{
  private:
    using arg_pack = non_type_pack_t<is...>;
    using subset_pack = arg_pack::template head_t<len>;
    using pass_down_pack = arg_pack::template truncate_front_t<len>;
  public:
    using type = type_pack_t<subset_from_pack_t<subset_pack>>::template append_t<typename split_logic<pass_down_pack, len>::type>;
  };

  template<size_t len, typename Uint, size_t... is>
  requires (sizeof...(is) == len)
  && std::unsigned_integral<Uint>
  struct split_logic<non_type_pack<Uint, is...>, len>{
  private:
    using subset_pack = non_type_pack_t<is...>;
  public:
    using type = type_pack_t<subset_from_pack_t<subset_pack>>;
  };

  template<template<typename...> class F, typename First, typename Second, typename... Tail>
    requires helpers::specializes_class_template_v<F, F<First, Second>>
    struct fold_logic{
      using type = fold_logic<F, F<First, Second>, Tail...>::type;
    };

    template<template<typename...> class F, typename Second_To_Last, typename Last>
    requires helpers::specializes_class_template_v<F, F<Second_To_Last, Last>>
    struct fold_logic<F, Second_To_Last, Last>{
      using type = F<Second_To_Last, Last>;
    };

    template<template <typename...> class templ, size_t order>
    requires (order > 0)
    struct apply_templ_to_nr_combs{
    private:
    template<typename T>
    using feed_template = T::template specialize_template_t<templ>;

    using combinations = generate_non_type_pack_t<non_rep_combinations::non_rep_index_combinations_t<sizeof...(Ts), order>>;
    using split_up = type_pack_t<Ts...>::template split_logic<combinations, order>::type;
    public:
      using type = split_up::template functor_map_t<feed_template>;
    };

public:
  static constexpr size_t size = sizeof...(Ts);

  // extract i-th entry in parameter pack
  template<size_t i>
  using index_t = pack_index_t<i, Ts...>;

  // append another tpye_pack_t
  template<typename Append_Pack>
  using append_t = append_logic<Append_Pack>::type;

  // removes first n entries
  template<size_t n>
  using truncate_front_t = truncate_front_logic<n, Ts...>::type;

  // removes last n entries
  template<size_t n>
  using truncate_back_t = truncate_back_logic<n, Ts...>::type;

  // yields first n elements
  template<size_t n>
  using head_t = truncate_back_t<sizeof...(Ts) - n>;

  // yields last n elements
  template<size_t n>
  using tail_t = truncate_front_t<sizeof...(Ts) - n>;

  // use Ts as type-arguments to specialize other type template
  template<template <typename...> class Class_Template>
  using specialize_template_t = Class_Template<Ts...>;

  // reverses order of Ts...
  using reverse_t = reverse_logic<sizeof...(Ts) - 1, Ts...>::type;

  // apply a class template to Ts in a functorial way
  template<template <typename...> class Class_Template>
  using functor_map_t = functor_map<Class_Template>::type;

  // apply a class template to Ts in a monadic way
  template<template <typename...> class Class_Template>
  using monadic_bind_t = monadic_bind<Class_Template>::type;

  // generates as nested type pack of subsets of Ts... of length len from indices specified in a non type pack of type size_t
  template<typename NT_Pack, size_t len>
  using split_t = split_logic<NT_Pack, len>::type;

  // generates a type by folding Ts... via a class template
  template<template<typename...> class F, typename Init>
  using fold_t = fold_logic<F, Init, Ts...>::type;

  // splits types into non-repeating groups of a certain size and uses each to specialize a template
  template<template <typename...> class Templ, size_t order>
  using apply_templ_to_nr_combs_t = apply_templ_to_nr_combs<Templ, order>::type;
};

template<typename>
struct generate_type_pack{};

template<template<typename...> class Class_Templ, typename... Ts>
struct generate_type_pack<Class_Templ<Ts...>>{
    using type = type_pack_t<Ts...>;
};

template<typename T>
using generate_type_pack_t = generate_type_pack<T>::type;

// metafunction to determine whether a type can be used to generate a type_pack_t via generate_type_pack_t
template<typename, typename = void>
struct type_pack_convertible: std::false_type{};

template<typename T>
struct type_pack_convertible<T, std::void_t<generate_type_pack_t<T>>>: std::true_type{};

template<typename T>
constexpr inline bool type_pack_convertible_v = type_pack_convertible<T>::value;

};

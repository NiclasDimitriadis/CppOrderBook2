#pragma once

#include "helpers.hpp"
#include "monoidal_class_template.hpp"
#include "non_rep_combinations.hpp"
#include "param_pack.hpp"

namespace type_pack_check{
    template<template<typename...> class Type_Checker, template<typename, typename> class Aggregator, size_t order, typename... Ts>
    struct checker_aggregator_check: checker_aggregator_check<Type_Checker, Aggregator, order, param_pack::type_pack_t<Ts...>>{};


    template<template<typename...> class Type_Handler, template<typename, typename> class Aggregator, size_t order, typename Type_Pack>
    requires param_pack::type_pack_convertible_v<Type_Pack>
    struct checker_aggregator_check<Type_Handler, Aggregator, order, Type_Pack>{
    private:
        using type_pack = param_pack::generate_type_pack_t<Type_Pack>;
        // apply Monoid_Templ to all non repeating combiantions of types of length order
        using checks_applied = type_pack::template apply_templ_to_nr_combs_t<Type_Handler, order>;
        // clip first element to use as init when folding
        using fst = checks_applied::template index_t<0>;
        using tail = checks_applied::template truncate_front_t<1>;
    public:
        using type = tail::template fold_t<Aggregator, fst>;
    };


    template<template<typename...> class Type_Handler, template<typename, typename> class Aggregator, size_t order, typename... Ts>
    using checker_aggregator_check_t = checker_aggregator_check<Type_Handler, Aggregator, order, Ts...>::type;


    template<template<typename...> class Monoid_Templ, size_t order, typename... Ts>
    struct monoid_check: monoid_check<Monoid_Templ, order, param_pack::type_pack_t<Ts...>>{};

    template<template<typename...> class Monoid_Templ, size_t order, typename Type_Pack>
    requires param_pack::type_pack_convertible_v<Type_Pack>
    && monoidal_class_template::check_closure_property_v<Monoid_Templ, order,
    param_pack::generate_type_pack_t<Type_Pack>>
    struct monoid_check<Monoid_Templ, order, Type_Pack>{
    private:
        using type_pack = param_pack::generate_type_pack_t<Type_Pack>;
        // apply Monoid_Templ to all non repeating combiantions of types of length order
        using checks_applied = type_pack::template apply_templ_to_nr_combs_t<Monoid_Templ, order>;
        // clip first element to use as init when folding
        using fst = checks_applied::template index_t<0>;
        using tail = checks_applied::template truncate_front_t<1>;
    public:
        using type = tail::template fold_t<Monoid_Templ, fst>;
    };

    template<template<typename...> class Monoid_Templ, size_t order, typename... Ts>
    using monoid_check_t = monoid_check<Monoid_Templ,order,Ts...>::type;

}

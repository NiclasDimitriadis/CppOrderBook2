#include "Test_Includes.hpp"

template<int I>
struct INT_{
    static constexpr int value = I;
};

using ONE = INT_<1>;
using TWO = INT_<2>;
using THREE = INT_<3>;
using FOUR = INT_<4>;
using FIVE = INT_<5>;

template<typename T>
struct EXTRACT_VAL{
    static constexpr auto value = T::value;
};

template<typename T1, typename T2>
struct MAX_AGGR{
    static constexpr auto value = std::max(T1::value, T2::value);
};

template<typename T1, typename T2>
struct EXTRACT_NE{
    static constexpr bool value = T1::value != T2::value;
};

template<typename T1, typename T2>
struct AND{
    static constexpr bool value = T1::value && T2::value;
};

using max_val_functor_fold = type_pack_check::checker_aggregator_check_t<EXTRACT_VAL,MAX_AGGR,1,ONE,TWO,THREE,FOUR,FIVE>;
static_assert(max_val_functor_fold::value == 5);

using no_equal_vals = type_pack_check::checker_aggregator_check_t<EXTRACT_NE,AND,2,ONE,TWO,THREE,FOUR,FIVE>;
static_assert(no_equal_vals::value == true);

template<typename...>
struct MAX_MONOID{
    static_assert(false);
};

template<typename T>
struct MAX_MONOID<T>{
    static constexpr auto value = T::value;
};

template<typename T1, typename T2>
struct MAX_MONOID<T1, T2>{
    static constexpr auto value = std::max(T1::value, T2::value);
};

template<typename...>
struct NOT_EQUAL_MONOID{
    static_assert(false);
};

template<typename T1, typename T2>
requires (!helpers::specializes_class_template_v<NOT_EQUAL_MONOID,T1>)
&& (!helpers::specializes_class_template_v<NOT_EQUAL_MONOID,T2>)
struct NOT_EQUAL_MONOID<T1,T2>{
    static constexpr bool value = T1::value != T2::value;
};

template<typename T1, typename T2>
requires helpers::specializes_class_template_v<NOT_EQUAL_MONOID,T1>
&& helpers::specializes_class_template_v<NOT_EQUAL_MONOID,T2>
struct NOT_EQUAL_MONOID<T1,T2>{
    static constexpr bool value = T1::value && T2::value;
};


using max_val_monoid = type_pack_check::monoid_check_t<MAX_MONOID,1,ONE,TWO,THREE,FOUR,FIVE>;
static_assert(max_val_monoid::value == 5);

using not_equal_monoid = type_pack_check::monoid_check_t<NOT_EQUAL_MONOID,2,ONE,TWO,THREE,FOUR,FIVE>;
static_assert(not_equal_monoid::value == true);

int main(){
    std::cout << "All test cases for type_pack_check.hpp compiled successfully" << "\n";
};

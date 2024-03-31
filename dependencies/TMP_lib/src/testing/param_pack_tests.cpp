
#include "Test_Includes.hpp"


constexpr int a = 1;
constexpr int b = 2;
constexpr int c = 3;
constexpr int d = 4;
constexpr int e = 5;
constexpr int f = 6;


static_assert(std::is_same_v<const int, param_pack::pack_type_t<a,b,c>>);

static_assert(std::is_same_v<param_pack::pack_index_t<2,int,bool,float,double,char>,float>);

static_assert(param_pack::pack_index_v<2,1,2,3,4,5,6> == 3);


using test_NTP = param_pack::non_type_pack<int,0,1,2,3,4>;

static_assert(test_NTP::size == 5);
static_assert(std::is_same_v<test_NTP::type,int>);

static_assert(test_NTP::index_v<2> == 2);
static_assert(test_NTP::index_v<2> != 3);

using test_NTP2 = test_NTP::append_t<param_pack::non_type_pack<int,5,6,7,8,9>>;

static_assert(test_NTP2::size == 10);
static_assert(test_NTP2::index_v<8> == 8);
static_assert(test_NTP2::index_v<8> != 9);

using test_NTP3 = test_NTP2::truncate_front_t<3>;
static_assert(test_NTP3::size == 7);
static_assert(test_NTP3::index_v<0> == 3);

using test_NTP4 = test_NTP2::truncate_back_t<3>;
static_assert(test_NTP4::size == 7);
static_assert(test_NTP4::index_v<6> == 6);

static_assert(std::is_same_v<test_NTP::head_t<2>, test_NTP::truncate_back_t<3>>);
static_assert(std::is_same_v<test_NTP::head_t<3>, test_NTP::truncate_back_t<2>>);

using test_NTP5 = test_NTP::functor_map_t<[](int i){return i * 10;}>;
static_assert(test_NTP5::size == 5);
static_assert(test_NTP5::index_v<4> == 40);

template<int i>
struct stretch{
    using type = param_pack::non_type_pack<int,i,i,i>;
};

template<int i>
using stretch_t = stretch<i>::type;

using test_NTP6 = test_NTP::monadic_bind_t<stretch_t>;
static_assert(test_NTP6::size == 15);
static_assert(test_NTP6::index_v<2> == 0);
static_assert(test_NTP6::index_v<3> == 1);
static_assert(test_NTP6::index_v<4> == 1);
static_assert(test_NTP6::index_v<5> == 1);
static_assert(test_NTP6::index_v<6> == 2);

static_assert(test_NTP::fold_v<[](int i1, int i2){return i1 + i2;},0> == 10);

template<typename...>
struct t_template{};

using t_type = t_template<int, float, bool>;

template<auto...>
struct nt_template{};

using nt_type = nt_template<int(1),int(2),int(3),int(4)>;

template<typename T, T... Ts>
struct tnt_template{};

using tnt_type = tnt_template<int, int(1),int(2),int(3),int(4)>;

using test_NTP7 = param_pack::non_type_pack<const int,1,2,3,4>;
static_assert(std::is_same_v<test_NTP7, param_pack::generate_non_type_pack_t<nt_type>>);
static_assert(std::is_same_v<test_NTP7, param_pack::generate_non_type_pack_t<tnt_type>>);

static_assert(param_pack::non_type_pack_convertible_v<nt_type>);
static_assert(param_pack::non_type_pack_convertible_v<tnt_type>);
static_assert(!param_pack::non_type_pack_convertible_v<float>);

template<auto...>
struct sample_templ{};


using Test_Type = sample_templ<int(0),int(1),int(2)>;
using Test_Type2 = std::integer_sequence<int,0,1,2>;


static_assert(std::is_same_v<param_pack::generate_non_type_pack_t<Test_Type>,param_pack::generate_non_type_pack_t<Test_Type2>>);

using test_TP = param_pack::type_pack_t<int, float, bool, size_t>;
using test_TP2 = param_pack::type_pack_t<double, char>;

static_assert(std::is_same_v<bool, test_TP::index_t<2>>);

static_assert(test_TP::size == 4);

using test_TP3 = test_TP::append_t<test_TP2>;
static_assert(test_TP3::size == 6);

using test_TP4 = test_TP3::truncate_front_t<2>;
static_assert(test_TP4::size == 4);
static_assert(std::is_same_v<test_TP4::index_t<0>,bool>);

using test_TP5 = test_TP3::truncate_back_t<2>;
static_assert(test_TP5::size == 4);
static_assert(std::is_same_v<test_TP5::index_t<3>, size_t>);

template<typename T>
using to_size_t = size_t;

static_assert(std::is_same_v<test_TP3::head_t<2>, test_TP3::truncate_back_t<4>>);
static_assert(std::is_same_v<test_TP3::tail_t<4>, test_TP3::truncate_front_t<2>>);

using test_TP6 = test_TP3::functor_map_t<to_size_t>;
static_assert(std::is_same_v<test_TP6::index_t<0>, size_t>);
static_assert(std::is_same_v<test_TP6::index_t<3>, size_t>);

template<typename T>
using add_const_and_ref = param_pack::type_pack_t<T, const T, T&>;

using test_TP7 = test_TP3::monadic_bind_t<add_const_and_ref>;
static_assert(test_TP7::size == 18);
static_assert(std::is_same_v<test_TP7::index_t<1>, const int>);
static_assert(std::is_same_v<test_TP7::index_t<2>, int&>);
static_assert(std::is_same_v<test_TP7::index_t<10>, const size_t>);

using test_Var = std::variant<int, float, bool, size_t>;
using test_TP8 = param_pack::generate_type_pack_t<test_Var>;
static_assert(std::is_same_v<test_TP, test_TP8>);

static_assert(param_pack::type_pack_convertible_v<test_Var>);
static_assert(!param_pack::type_pack_convertible_v<float>);

using split_NTP = param_pack::non_type_pack<size_t,0,0,1,1,2,2>;

using test_TP9 = test_TP3::split_t<split_NTP,2>;
static_assert(test_TP9::size == 3);
static_assert(std::is_same_v<test_TP9::index_t<0>,param_pack::type_pack_t<int,int>>);
static_assert(std::is_same_v<test_TP9::index_t<1>,param_pack::type_pack_t<float,float>>);
static_assert(std::is_same_v<test_TP9::index_t<2>,param_pack::type_pack_t<bool,bool>>);


template<size_t i>
struct has_value{
    static constexpr size_t value = i;
};


template<typename T1, typename T2>
struct max_value{
    static constexpr size_t value = std::max(T1::value, T2::value);
};

using test_TP10 = param_pack::type_pack_t<has_value<1>,has_value<2>,has_value<100>,has_value<3>>;
using fold_result = test_TP10::fold_t<max_value, has_value<0>>;
static_assert(fold_result::value == 100);


int main() {
  std::cout << "All test cases for param_pack.hpp compiled successfully" << "\n";
}

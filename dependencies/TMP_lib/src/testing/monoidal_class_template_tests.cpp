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

template<typename...>
struct SUM_{
    static_assert(false);
};

template<typename INT>
struct SUM_<INT>{
    static constexpr int value = INT::value;
};

template<typename S1, typename S2>
requires helpers::specializes_class_template_v<SUM_,S1>
&& helpers::specializes_class_template_v<SUM_,S2>
struct SUM_<S1, S2>{
    static constexpr int value = S1::value + S2::value;
};


template<typename S1, typename S2>
requires helpers::specializes_class_template_v<SUM_,S1>
&& helpers::specializes_class_template_v<SUM_,S2>
struct SUM_COMP_{
    static constexpr bool value = S1::value == S2::value;
};


    static_assert(monoidal_class_template::check_closure_property_v<SUM_,1,ONE,TWO,THREE,FOUR,FIVE>);
    static_assert(monoidal_class_template::check_closure_property_v<SUM_,1,ONE,TWO>);

    static_assert(monoidal_class_template::check_associativity_property_v<SUM_,SUM_COMP_,1,ONE,TWO,THREE,FOUR,FIVE>);

    static_assert(monoidal_class_template::check_identity_element_property_v<SUM_,SUM_COMP_,1,ONE,TWO,THREE,FOUR,FIVE>);

    static_assert(monoidal_class_template::monoidal_class_template_v<SUM_,SUM_COMP_,1,ONE,TWO,THREE,FOUR,FIVE>);


int main() {
  std::cout << "All test cases for monoidal_class_template.hpp compiled successfully" << "\n";
}

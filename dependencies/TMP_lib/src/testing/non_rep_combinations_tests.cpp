#include "Test_Includes.hpp"


using TestSeq1 = std::integer_sequence<size_t,
1,2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,2,3,2,4,2,5,2,6,2,7,2,8,2,9,3,4,3,5,3,6,3,7,3,8,3,9,4,5,4,6,4,7,4,8,4,9,5,6,5,7,5,8,5,9,6,7,6,8,6,9,7,8,7,9,8,9
>;
using TestSeq2 = std::integer_sequence<size_t,
1,2,3,1,2,4,1,2,5,2,3,4,2,3,5,3,4,5>;
using TestSeq3 = std::integer_sequence<size_t,
0,1,2,0,1,3,0,1,4,1,2,3,1,2,4,2,3,4>;
using TestSeq4 = std::integer_sequence<size_t,1,2,3,4,5>;


static_assert(std::is_same_v<non_rep_combinations::non_rep_combinations_t<9,2>, TestSeq1>);
static_assert(std::is_same_v<non_rep_combinations::non_rep_combinations_t<5,3>, TestSeq2>);
static_assert(std::is_same_v<non_rep_combinations::non_rep_combinations_t<5,1>, TestSeq4>);
static_assert(std::is_same_v<non_rep_combinations::non_rep_combinations_t<5,5>, TestSeq4>);



    static_assert(std::is_same_v<non_rep_combinations::non_rep_index_combinations_t<5,3>, TestSeq3>);



int main() {
  std::cout << "All test cases for non_rep_combinations.hpp compiled successfully" << "\n";
}

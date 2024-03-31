#include "Test_Includes.hpp"

template<typename...>
struct t_template{};

using t_type = t_template<int, float, bool>;

template<auto...>
struct nt_template{};

using nt_type = nt_template<int(1),int(2),int(3),int(4)>;

template<typename T, T... Ts>
struct tnt_template{};

using tnt_type = tnt_template<int, int(1),int(2),int(3),int(4)>;

struct bool_val{
  static constexpr bool value = true;
};

struct no_bool_val{
  static constexpr int i = 5;
};
static_assert(helpers::constexpr_bool_value<bool_val>);
static_assert(!helpers::constexpr_bool_value<no_bool_val>);

static_assert(helpers::specializes_class_template_v<t_template, t_type>);
static_assert(helpers::specializes_class_template_nt_v<nt_template, nt_type>);
static_assert(helpers::specializes_class_template_tnt_v<tnt_template, tnt_type>);

static_assert(!helpers::specializes_class_template_v<t_template, nt_type>);
static_assert(!helpers::specializes_class_template_nt_v<nt_template, t_type>);
static_assert(!helpers::specializes_class_template_tnt_v<tnt_template, nt_type>);

int main() {
  std::cout << "All test cases for helpers.hpp compiled successfully" << "\n";
}

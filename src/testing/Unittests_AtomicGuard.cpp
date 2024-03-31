#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing Guards::AtomicFlagGuard") {
  std::atomic_flag test_flag{false};
  Guards::AtomicFlagGuard test_guard = Guards::AtomicFlagGuard(&test_flag);

  SUBCASE("binds to flag") {
    CHECK(&test_flag == test_guard.return_flag_ptr());
  }

  SUBCASE("locks correctly") {
    CHECK(test_guard.is_locked() == false);
    test_guard.lock();
    CHECK(test_flag.test() == true);
    CHECK(test_guard.is_locked() == true);
  }

  SUBCASE("unlocks correctly") {
    test_guard.lock();
    test_guard.unlock();
    CHECK(test_guard.is_locked() == false);
    CHECK(test_flag.test() == false);
  }

  SUBCASE("rebinds correctly") {
    auto other_flag = std::atomic_flag{false};
    test_guard.lock();
    test_guard.rebind(&other_flag);
    CHECK(test_flag.test() == false);
    CHECK(&other_flag == test_guard.return_flag_ptr());
  }

  SUBCASE("move assigns correctly") {
    test_guard.lock();
    auto other_guard = Guards::AtomicFlagGuard(nullptr);
    other_guard = std::move(test_guard);
    CHECK(&test_flag == other_guard.return_flag_ptr());
    CHECK(other_guard.is_locked() == true);
    CHECK(nullptr == test_guard.return_flag_ptr());
  }

  SUBCASE("move constructs correctly") {
    auto other_guard = Guards::AtomicFlagGuard(std::move(test_guard));
    CHECK(&test_flag == other_guard.return_flag_ptr());
    CHECK(other_guard.is_locked() == false);
    CHECK(nullptr == test_guard.return_flag_ptr());
  }
}

int main() {
  doctest::Context context;
  context.run();
}

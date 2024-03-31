#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing OrderBookBucket::OrderBookBucket") {
  using testBucketClass = OrderBookBucket::OrderBookBucket<8, std::int32_t>;
  testBucketClass testBucket{};
  CHECK(testBucket.get_volume() == 0);
  testBucket.add_liquidity(1000);
  CHECK(testBucket.get_volume() == 1000);
  CHECK(testBucket.consume_liquidity(2000) == 1000);
  CHECK(testBucket.get_volume() == 0);
  testBucket.add_liquidity(1000);
  testBucket.add_liquidity(-500);
  CHECK(testBucket.get_volume() == 500);
  testBucket.add_liquidity(-1000);
  CHECK(testBucket.get_volume() == -500);
  CHECK(testBucket.consume_liquidity(1000) == 0);
  CHECK(testBucket.consume_liquidity(-1000) == -500);
  CHECK(testBucket.get_volume() == 0);
}

int main() {
  doctest::Context context;
  context.run();
}

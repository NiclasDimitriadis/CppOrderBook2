#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing FileToTuples::file_to_tuples") {
  using LineTuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
  auto msg_tuples =
      FileToTuples::file_to_tuples<LineTuple>("../testMsgCsv.csv");
  CHECK(std::get<0>(msg_tuples[0]) == 0);
  CHECK(std::get<0>(msg_tuples[1]) == 1);
  CHECK(std::get<0>(msg_tuples[2]) == 2);
  CHECK(std::get<1>(msg_tuples[1]) == 22);
  CHECK(std::get<2>(msg_tuples[2]) == 333);
}

int main() {
  doctest::Context context;
  context.run();
}

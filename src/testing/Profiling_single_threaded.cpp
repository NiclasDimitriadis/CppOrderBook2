#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <variant>
#include <vector>

#include "FIXMockSocket.hpp"
#include "FIXMsgClasses.hpp"
#include "FIXSocketHandler.hpp"
#include "FileToTuples.hpp"
#include "OrderBook.hpp"

using line_tuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
using MsgClassVar =
    std::variant<FIXMsgClasses::AddLimitOrder,
                 FIXMsgClasses::WithdrawLimitOrder, FIXMsgClasses::MarketOrder>;
static constexpr std::uint32_t two_pow_20 = 1048576;
using OrderBookClass =
    OrderBook::OrderBook<MsgClassVar, std::int64_t, 6000, false>;
using SockHandler =
    FIXSocketHandler::FIXSocketHandler<MsgClassVar,
                                       FIXMockSocket::FIXMockSocket>;

int main(int argc, char *argv[]) {
  // process path argumement, set up objects
  std::string csv_path(argv[1]);
  auto tuple_vec = FileToTuples::file_to_tuples<line_tuple>(csv_path);
  auto mock_sock = FIXMockSocket::FIXMockSocket(tuple_vec, nullptr);
  auto s_handler = SockHandler(&mock_sock);
  auto book = OrderBookClass(7000);
  const int n_msgs = tuple_vec.size();
  std::optional<MsgClassVar> read_ret;

  for (int i = 0; i < n_msgs; ++i) {
    read_ret = s_handler.read_next_message();
    book.process_order(read_ret.value());
  }

  // use current time as seed for random generator, generate random index
  std::srand(std::time(nullptr));
  int random_price = 7000 + (std::rand() % 6000);
  // query volume at random index to prohibit compiler from optimizing away the
  // program logic
  std::cout << "volume at randomly generated price " << random_price << ": "
            << book.volume_at_price(random_price) << "\n";
}

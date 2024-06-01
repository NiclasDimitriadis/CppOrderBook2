#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing FIXSocketHandler::FIXSocketHandler") {
  using MsgClassVariant = std::variant<FIXMsgClasses::AddLimitOrder,
                                       FIXMsgClasses::WithdrawLimitOrder,
                                       FIXMsgClasses::MarketOrder>;
  using SockHandlerClass =
      FIXSocketHandler::FIXSocketHandler<MsgClassVariant,
                                         FIXMockSocket::FIXMockSocket>;
  using LineTuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
  auto msg_tuples =
      FileToTuples::file_to_tuples<LineTuple>("../testMsgCsv.csv");
  auto mock_socket = FIXMockSocket::FIXMockSocket(msg_tuples);
  auto mock_socketPtr = &mock_socket;
  auto sock_handler = SockHandlerClass(mock_socketPtr);
  auto recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  MsgClassVariant recv_content = recv_res.value();
  CHECK(recv_content.index() == 0);
  CHECK(std::get<0>(recv_content).get_order_volume() == -11);
  CHECK(std::get<0>(recv_content).get_order_price() == 111);

  recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  recv_content = recv_res.value();
  CHECK(recv_content.index() == 1);
  CHECK(std::get<1>(recv_content).get_order_volume() == 22);
  CHECK(std::get<1>(recv_content).get_order_price() == 222);

  recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  recv_content = recv_res.value();
  CHECK(recv_content.index() == 2);
  CHECK(std::get<2>(recv_content).get_order_volume() == 33);

  recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  recv_content = recv_res.value();
  CHECK(recv_content.index() == 0);
  CHECK(std::get<0>(recv_content).get_order_volume() == 1000);
  CHECK(std::get<0>(recv_content).get_order_price() == 20);

  recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  recv_content = recv_res.value();
  CHECK(recv_content.index() == 1);
  CHECK(std::get<1>(recv_content).get_order_volume() == -2000);
  CHECK(std::get<1>(recv_content).get_order_price() == 50);

  recv_res = sock_handler.read_next_message();
  CHECK(!recv_res.has_value());

  recv_res = sock_handler.read_next_message();
  CHECK(recv_res.has_value());
  recv_content = recv_res.value();
  CHECK(recv_content.index() == 2);
  CHECK(std::get<2>(recv_content).get_order_volume() == 3000);
}

int main() {
  doctest::Context context;
  context.run();
}

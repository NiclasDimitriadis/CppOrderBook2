#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing OrderBook::orderBook") {
  using MsgClassVariant = std::variant<FIXMsgClasses::AddLimitOrder,
                                       FIXMsgClasses::WithdrawLimitOrder,
                                       FIXMsgClasses::MarketOrder>;
  using TestOrderBookClass =
      OrderBook::OrderBook<MsgClassVariant, std::int64_t, 1000, false>;

  SUBCASE("testing for correct behavior of best_bid_ask") {
    auto test_order_book = TestOrderBookClass{0};
    auto best_bid_ask = test_order_book.best_bid_ask();
    CHECK(!std::get<0>(best_bid_ask).has_value());
    CHECK(!std::get<1>(best_bid_ask).has_value());
    MsgClassVariant test_msg_class_var;
    test_msg_class_var.emplace<0>(1000, 101);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(-1000, 99);
    test_order_book.process_order(test_msg_class_var);
    best_bid_ask = test_order_book.best_bid_ask();
    CHECK(std::get<0>(best_bid_ask).value() == 99);
    CHECK(std::get<1>(best_bid_ask).value() == 101);
  }

  SUBCASE(
      "testing regular and out of bounds limit order adding and withdrawel") {
    auto test_order_book = TestOrderBookClass{5};
    MsgClassVariant test_msg_class_var;
    // adding limit order
    test_msg_class_var.emplace<0>(1000, 100);
    auto order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 0);
    CHECK(std::get<1>(order_resp) == 0);
    CHECK(std::get<2>(order_resp) == 0);
    CHECK(std::get<3>(order_resp) == 0);
    CHECK(test_order_book.volume_at_price(100) == 1000);
    // withdrawing limit order
    test_msg_class_var.emplace<1>(1000, 100);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 0);
    CHECK(std::get<1>(order_resp) == 1000);
    CHECK(std::get<2>(order_resp) == 0);
    CHECK(std::get<3>(order_resp) == 0);
    CHECK(test_order_book.volume_at_price(100) == 0);
    // repeat with negative volume
    // adding limit order
    test_msg_class_var.emplace<0>(-1000, 100);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 0);
    CHECK(std::get<1>(order_resp) == 0);
    CHECK(std::get<2>(order_resp) == 0);
    CHECK(std::get<3>(order_resp) == 0);
    CHECK(test_order_book.volume_at_price(100) == -1000);
    // withdrawing limit order
    test_msg_class_var.emplace<1>(-1000, 100);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 0);
    CHECK(std::get<1>(order_resp) == -1000);
    CHECK(std::get<2>(order_resp) == 0);
    CHECK(std::get<3>(order_resp) == 0);
    CHECK(test_order_book.volume_at_price(100) == 0);
    // try adding order out of bounds
    test_msg_class_var.emplace<0>(1000, 2000);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 0);
    CHECK(std::get<1>(order_resp) == 0);
    CHECK(std::get<2>(order_resp) == 0);
    CHECK(std::get<3>(order_resp) == 1);
  }

  SUBCASE("testing immediate limit order matching") {
    auto test_order_book = TestOrderBookClass{0};
    MsgClassVariant test_msg_class_var;
    test_msg_class_var.emplace<0>(-500, 100);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(-500, 101);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(2000, 98);
    auto order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 100);
    CHECK(std::get<1>(order_resp) == 1000);
    CHECK(std::get<2>(order_resp) == 101 * 500 + 100 * 500);
    CHECK(test_order_book.volume_at_price(101) == 0);
    CHECK(test_order_book.volume_at_price(100) == 0);
    CHECK(test_order_book.volume_at_price(98) == 1000);
    // repeat with opposite signs
    test_msg_class_var.emplace<0>(500, 100);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(500, 101);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(-2000, 103);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 101);
    CHECK(std::get<1>(order_resp) == -2000);
    CHECK(std::get<2>(order_resp) == 98 * -1000 + 101 * -500 + 100 * -500);
    CHECK(test_order_book.volume_at_price(98) == 0);
    CHECK(test_order_book.volume_at_price(101) == 0);
    CHECK(test_order_book.volume_at_price(100) == 0);
    CHECK(test_order_book.volume_at_price(103) == 0);
  }

  SUBCASE(
      "testing correct execution of market order with sufficient liquidity") {
    auto test_order_book = TestOrderBookClass{0};
    MsgClassVariant test_msg_class_var;
    for (int i = 0; i < 10; ++i) {
      test_msg_class_var.emplace<0>(1000, 101 + i);
      test_order_book.process_order(test_msg_class_var);
      test_msg_class_var.emplace<0>(-1000, 99 - i);
      test_order_book.process_order(test_msg_class_var);
    };
    auto best_bid_ask = test_order_book.best_bid_ask();
    CHECK(std::get<0>(best_bid_ask).value() == 99);
    CHECK(std::get<1>(best_bid_ask).value() == 101);
    // fill negative volume/demand
    test_msg_class_var.emplace<2>(-3500);
    auto order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 104);
    CHECK(std::get<1>(order_resp) == -3500);
    CHECK(std::get<2>(order_resp) ==
          (-1000 * 101 - 1000 * 102 - 1000 * 103 - 500 * 104));
    CHECK(test_order_book.volume_at_price(101) == 0);
    CHECK(test_order_book.volume_at_price(104) == 500);
    best_bid_ask = test_order_book.best_bid_ask();
    CHECK(std::get<1>(best_bid_ask).value() == 104);
    // fill positive volume/ supply
    test_msg_class_var.emplace<2>(3500);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 96);
    CHECK(std::get<1>(order_resp) == 3500);
    CHECK(std::get<2>(order_resp) ==
          (1000 * 99 + 1000 * 98 + 1000 * 97 + 500 * 96));
    CHECK(test_order_book.volume_at_price(99) == 0);
    CHECK(test_order_book.volume_at_price(96) == -500);
    best_bid_ask = test_order_book.best_bid_ask();
    CHECK(std::get<0>(best_bid_ask).value() == 96);
  }

  SUBCASE(
      "testing correct execution of market order with insufficient liquidity") {
    auto test_order_book = TestOrderBookClass{0};
    MsgClassVariant test_msg_class_var;
    for (int i = 0; i < 3; ++i) {
      test_msg_class_var.emplace<0>(1000, 101 + i);
      test_order_book.process_order(test_msg_class_var);
      test_msg_class_var.emplace<0>(-1000, 99 - i);
      test_order_book.process_order(test_msg_class_var);
    };
    auto best_bid_ask = test_order_book.best_bid_ask();
    CHECK(std::get<0>(best_bid_ask).value() == 99);
    CHECK(std::get<1>(best_bid_ask).value() == 101);
    // fill negative volume/demand
    test_msg_class_var.emplace<2>(-3500);
    auto order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 103);
    CHECK(std::get<1>(order_resp) == -3000);
    CHECK(std::get<2>(order_resp) == (-1000 * 101 - 1000 * 102 - 1000 * 103));
    CHECK(test_order_book.volume_at_price(101) == 0);
    CHECK(test_order_book.volume_at_price(103) == 0);
    best_bid_ask = test_order_book.best_bid_ask();
    CHECK(!std::get<1>(best_bid_ask).has_value());
    // fill positive volume/ supply
    test_msg_class_var.emplace<2>(3500);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<0>(order_resp) == 97);
    CHECK(std::get<1>(order_resp) == 3000);
    CHECK(std::get<2>(order_resp) == (1000 * 99 + 1000 * 98 + 1000 * 97));
    CHECK(test_order_book.volume_at_price(99) == 0);
    CHECK(test_order_book.volume_at_price(97) == 00);
    best_bid_ask = test_order_book.best_bid_ask();
    CHECK(!std::get<0>(best_bid_ask).has_value());
  }

  SUBCASE("testing for correct behavior when shifting the order book") {
    auto test_order_book = TestOrderBookClass{100};
    MsgClassVariant test_msg_class_var;
    CHECK(!test_order_book.shift_book(-200));
    test_msg_class_var.emplace<0>(-10, 500);
    test_order_book.process_order(test_msg_class_var);
    test_msg_class_var.emplace<0>(-10, 1400);
    auto order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<3>(order_resp) == 1);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(test_order_book.shift_book(300));
    CHECK(test_order_book.volume_at_price(500) == -10);
    CHECK(test_order_book.shift_book(-300));
    CHECK(test_order_book.volume_at_price(500) == -10);
    test_order_book.shift_book(300);
    test_msg_class_var.emplace<0>(-10, 1400);
    order_resp = test_order_book.process_order(test_msg_class_var);
    CHECK(std::get<3>(order_resp) == 0);
    test_msg_class_var.emplace<0>(-10, 400);
    test_order_book.process_order(test_msg_class_var);
    CHECK(test_order_book.volume_at_price(400) == -10);
    CHECK(test_order_book.volume_at_price(1400) == -10);
    CHECK(!test_order_book.shift_book(-1));
    CHECK(!test_order_book.shift_book(1));
  }

  SUBCASE("testing concurrent writes to orderbook") {
    auto test_order_book = TestOrderBookClass(0);
    MsgClassVariant test_msg_class_var_0;
    test_msg_class_var_0.emplace<0>(-1, 100);
    MsgClassVariant test_msg_class_var_1;
    test_msg_class_var_1.emplace<0>(-1, 100);
    std::atomic<bool> start_signal{false};
    std::thread thread_0([&]() {
      while (!start_signal.load(std::memory_order_acquire)) {
      };
      for (int i = 0; i < 10000000; ++i) {
        test_order_book.process_order(test_msg_class_var_0);
      }
    });
    std::thread thread_1([&]() {
      while (!start_signal.load(std::memory_order_acquire)) {
      };
      for (int i = 0; i < 10000000; ++i) {
        test_order_book.process_order(test_msg_class_var_1);
      }
    });
    start_signal.store(true);
    thread_0.join();
    thread_1.join();
    CHECK(test_order_book.volume_at_price(100) == -20000000);
  }

  SUBCASE("brute-force-testing order book invariants using 1 million randomly "
          "generated erratic yet valid messages") {
    using SockHandler =
        FIXSocketHandler::FIXSocketHandler<MsgClassVariant,
                                           FIXMockSocket::FIXMockSocket>;
    using line_tuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
    auto test_order_book1 = TestOrderBookClass{1000};
    auto test_order_book2 = TestOrderBookClass{1000};
    auto tuple_vec =
        FileToTuples::file_to_tuples<line_tuple>("../RandTestDataErratic.csv");
    auto mock_socket = FIXMockSocket::FIXMockSocket(tuple_vec);
    auto sock_handler = SockHandler(&mock_socket);
    std::optional<MsgClassVariant> read_ret;
    std::string error_code;
    for (int i = 0; i < 1000000; ++i) {
      read_ret = sock_handler.read_next_message();
      test_order_book1.process_order(read_ret.value());
      error_code += test_order_book1.__invariants_check(i);
      test_order_book2.process_order(read_ret.value());
      if (!error_code.empty()) {
        break;
      };
    }
    CHECK(error_code.empty());
  }
}

int main() {
  doctest::Context context;
  context.run();
}

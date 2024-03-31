#define DOCTEST_CONFIG_IMPLEMENT

#include "Unittest_Includes.hpp"

TEST_CASE("testing FIXMockSocket::FIXMockSocket") {
  using LineTuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
  auto msg_vec = FileToTuples::file_to_tuples<LineTuple>("../testMsgCsv.csv");
  std::atomic_flag empty_buffer{false};
  auto mock_socket = FIXMockSocket::FIXMockSocket(msg_vec, &empty_buffer);

  SUBCASE("testing reading new limit order message from mock socket") {
    std::uint8_t dest_buffer[20];
    std::span<std::uint8_t, 20> dbs(dest_buffer, 20);
    auto recv_ret = mock_socket.recv(dest_buffer, 6);
    CHECK(recv_ret == 6);
    CHECK(*reinterpret_cast<std::uint32_t *>(&dest_buffer[0]) == 17);
    CHECK(*reinterpret_cast<std::uint16_t *>(&dest_buffer[4]) == 0xEB50);
    recv_ret = mock_socket.recv(&dest_buffer[6], 11);
    CHECK(recv_ret == 11);
    CHECK(*reinterpret_cast<std::int32_t *>(&dest_buffer[6]) == -11);
    CHECK(*reinterpret_cast<std::uint32_t *>(&dest_buffer[10]) == 111);
    std::uint8_t byte_sum{0};
    for (int i = 0; i < 14; ++i) {
      byte_sum += dest_buffer[i];
    };
    std::uint8_t fst_digit = (byte_sum / 100);
    std::uint8_t scnd_digit = (byte_sum - (fst_digit * 100)) / 10;
    std::uint8_t thrd_digit = byte_sum - (fst_digit * 100 + scnd_digit * 10);
    CHECK(dest_buffer[14] == fst_digit + 48);
    CHECK(dest_buffer[15] == scnd_digit + 48);
    CHECK(dest_buffer[16] == thrd_digit + 48);
  }

  SUBCASE("testing reading withdraw limit order message from mock socket") {
    std::uint8_t dest_buffer[20];
    // read first message to remove it from socket
    auto recv_ret = mock_socket.recv(dest_buffer, 17);
    recv_ret = mock_socket.recv(dest_buffer, 6);
    CHECK(recv_ret == 6);
    CHECK(*reinterpret_cast<std::uint32_t *>(&dest_buffer[0]) == 18);
    CHECK(*reinterpret_cast<std::uint16_t *>(&dest_buffer[4]) == 0xEB50);
    recv_ret = mock_socket.recv(&dest_buffer[6], 12);
    CHECK(recv_ret == 12);
    CHECK(*reinterpret_cast<std::int32_t *>(&dest_buffer[6]) == 22);
    CHECK(*reinterpret_cast<std::uint32_t *>(&dest_buffer[10]) == 222);
    CHECK(*reinterpret_cast<std::uint8_t *>(&dest_buffer[14]) == 0xFF);
    std::uint8_t byte_sum{0};
    for (int i = 0; i < 15; ++i) {
      byte_sum += dest_buffer[i];
    };
    std::uint8_t fst_digit = (byte_sum / 100);
    std::uint8_t scnd_digit = (byte_sum - (fst_digit * 100)) / 10;
    std::uint8_t thrd_digit = byte_sum - (fst_digit * 100 + scnd_digit * 10);
    CHECK(dest_buffer[15] == fst_digit + 48);
    CHECK(dest_buffer[16] == scnd_digit + 48);
    CHECK(dest_buffer[17] == thrd_digit + 48);
  }

  SUBCASE("testing reading market order message from mock socket") {
    std::uint8_t dest_buffer[20];
    // read first two messages to remove them from socket
    auto recv_ret = mock_socket.recv(dest_buffer, 17);
    recv_ret = mock_socket.recv(dest_buffer, 18);
    recv_ret = mock_socket.recv(dest_buffer, 6);
    CHECK(recv_ret == 6);
    CHECK(*reinterpret_cast<std::uint32_t *>(&dest_buffer[0]) == 13);
    CHECK(*reinterpret_cast<std::uint16_t *>(&dest_buffer[4]) == 0xEB50);
    recv_ret = mock_socket.recv(&dest_buffer[6], 7);
    CHECK(recv_ret == 7);
    CHECK(*reinterpret_cast<std::int32_t *>(&dest_buffer[6]) == 33);
    std::uint8_t byte_sum{0};
    for (int i = 0; i < 10; ++i) {
      byte_sum += dest_buffer[i];
    };
    std::uint8_t fst_digit = (byte_sum / 100);
    std::uint8_t scnd_digit = (byte_sum - (fst_digit * 100)) / 10;
    std::uint8_t thrd_digit = byte_sum - (fst_digit * 100 + scnd_digit * 10);
    CHECK(dest_buffer[10] == fst_digit + 48);
    CHECK(dest_buffer[11] == scnd_digit + 48);
    CHECK(dest_buffer[12] == thrd_digit + 48);
  }

  SUBCASE("testing correct setting of flag indicating empty buffer") {
    std::uint8_t dest_buffer[100];
    mock_socket.recv(&dest_buffer[0], 100);
    CHECK(!empty_buffer.test());
    mock_socket.recv(&dest_buffer[0], 56);
    CHECK(empty_buffer.test());
  }
}

int main() {
  doctest::Context context;
  context.run();
}

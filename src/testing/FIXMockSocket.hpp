#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <numeric>
#include <span>
#include <tuple>
#include <vector>

namespace FIXMockSocket {
struct FIXMockSocket {
private:
  // calculates and inserts FIX checksum
  void insert_checksum(std::uint32_t, std::uint32_t) noexcept;
  // functions to insert byte patterns for specific messages into memory, to be
  // called only during construction
  void insert_new_limit_order(
      const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &,
      std::uint32_t);
  void insert_withdraw_limit_order(
      const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &,
      std::uint32_t);
  void insert_market_order(
      const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &,
      std::uint32_t);
  void insert_oversized_msg(
      const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &,
      std::uint32_t);
  std::uint32_t mem_size;
  std::uint8_t *mem_ptr;
  std::span<uint8_t> mem_span;
  // iterates over vector, retreives message date from tuple, calls respective
  // insert function to be called only during construction
  void populate_memory(
      const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
          &) noexcept;
  // iterates over vector, determines numer of bytes needed to store all
  // messages to be called only during construction
  std::uint32_t determine_mem_size(
      const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
          &);
  // bookkeeping on number of messages already read and index for next read
  std::uint32_t read_index = 0;
  // flag to set when end of the buffer is reached
  std::atomic_flag *end_of_buffer;

public:
  // construct mock socket from vector of tuples containing:
  // unit8_t: message type(0: NewLimitOrder, 1 WithdrawLimitOrder, 2
  // MarketOrder) int32_t order volume uint32_t order price
  explicit FIXMockSocket(
      const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
          &,
      std::atomic_flag *);
  ~FIXMockSocket();
  std::int32_t recv(void *, std::uint32_t) noexcept;
};
}; // namespace FIXMockSocket

namespace FIXMockSocket {
void FIXMockSocket::insert_checksum(std::uint32_t msg_index,
                                    std::uint32_t checksum_index) noexcept {
  auto accum_span =
      this->mem_span.subspan(msg_index, checksum_index - msg_index);
  const std::uint8_t byte_sum =
      std::accumulate(accum_span.begin(), accum_span.end(), 0);
  this->mem_span[checksum_index] = (byte_sum / 100) + 48;
  this->mem_span[checksum_index + 1] = ((byte_sum % 100) / 10) + 48;
  this->mem_span[checksum_index + 2] = (byte_sum % 10) + 48;
};

void FIXMockSocket::insert_new_limit_order(
    const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &order_tuple,
    std::uint32_t begin_index) {
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index]) = 17;
  *reinterpret_cast<std::uint16_t *>(&this->mem_span[begin_index + 4]) = 0xEB50;
  *reinterpret_cast<std::int32_t *>(&this->mem_span[begin_index + 6]) =
      std::get<1>(order_tuple);
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index + 10]) =
      std::get<2>(order_tuple);
  this->insert_checksum(begin_index, begin_index + 14);
};

void FIXMockSocket::insert_withdraw_limit_order(
    const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &order_tuple,
    std::uint32_t begin_index) {
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index]) = 18;
  *reinterpret_cast<std::uint16_t *>(&this->mem_span[begin_index + 4]) = 0xEB50;
  *reinterpret_cast<std::int32_t *>(&this->mem_span[begin_index + 6]) =
      std::get<1>(order_tuple);
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index + 10]) =
      std::get<2>(order_tuple);
  mem_span[begin_index + 14] = 0xFF;
  this->insert_checksum(begin_index, begin_index + 15);
};

void FIXMockSocket::insert_market_order(
    const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &order_tuple,
    std::uint32_t begin_index) {
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index]) = 13;
  *reinterpret_cast<std::uint16_t *>(&this->mem_span[begin_index + 4]) = 0xEB50;
  *reinterpret_cast<std::int32_t *>(&this->mem_span[begin_index + 6]) =
      std::get<1>(order_tuple);
  this->insert_checksum(begin_index, begin_index + 10);
};

void FIXMockSocket::insert_oversized_msg(
    const std::tuple<std::uint8_t, std::int32_t, std::uint32_t> &order_tuple,
    std::uint32_t begin_index) {
  *reinterpret_cast<std::uint32_t *>(&this->mem_span[begin_index]) = 40;
  *reinterpret_cast<std::uint16_t *>(&this->mem_span[begin_index + 4]) = 0xEB50;
  // set 8 bits to make sure checksum will invalidate message even if all bits
  // were zeroed beforehand
  *reinterpret_cast<std::uint8_t *>(&this->mem_span[begin_index + 6]) = 0xFF;
};

std::uint32_t FIXMockSocket::determine_mem_size(
    const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
        &msg_vec) {
  std::uint32_t ret{0};
  for (auto msg : msg_vec) {
    ret += 17 * (std::get<0>(msg) == 0) + 18 * (std::get<0>(msg) == 1) +
           13 * (std::get<0>(msg) == 2) + 20 * (std::get<0>(msg) == 3) +
           40 * (std::get<0>(msg) == 4);
  };
  return ret;
};

void FIXMockSocket::populate_memory(
    const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
        &msg_vec) noexcept {
  std::uint32_t index{0};
  std::uint8_t msg_type_index{0};
  for (auto msg : msg_vec) {
    msg_type_index = std::get<0>(msg);
    switch (msg_type_index) {
    case 0:
      this->insert_new_limit_order(msg, index);
      index += 17;
      break;
    case 1:
      this->insert_withdraw_limit_order(msg, index);
      index += 18;
      break;
    case 2:
      this->insert_market_order(msg, index);
      index += 13;
      break;
    // 20 arbitrary bytes to simulate faulty order
    case 3:
      index += 20;
      break;
    case 4:
      // for checking whether unknown msg types/insignificant messages with
      // valid header can be correctly discarded
      this->insert_oversized_msg(msg, index);
      index += 40;
      break;
    };
  };
};

FIXMockSocket::FIXMockSocket(
    const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint32_t>>
        &msg_vec,
    std::atomic_flag *flag_ptr = nullptr)
    : end_of_buffer(flag_ptr) {
  this->mem_size = this->determine_mem_size(msg_vec);
  this->mem_ptr = new std::uint8_t[this->mem_size];
  this->mem_span = std::span<std::uint8_t>{this->mem_ptr, this->mem_size};
  this->populate_memory(msg_vec);
};

FIXMockSocket::~FIXMockSocket() { delete[] this->mem_ptr; };

std::int32_t FIXMockSocket::recv(void *dest, std::uint32_t len) noexcept {
  if ((this->read_index + len) > this->mem_size) {
    std::cout << "FIXMockSocket: out of bounds read during call of recv(). "
                 "terminating."
              << "\n";
    std::terminate();
  };
  std::memcpy(dest, &this->mem_span[this->read_index], len);
  this->read_index += len;
  if (this->read_index == this->mem_size && this->end_of_buffer != nullptr) {
    this->end_of_buffer->test_and_set(std::memory_order_release);
  };
  return len;
};
} // namespace FIXMockSocket

// class for handling a single socket, retrieving FIX SBE messages that come
// with Simple Open Framing Header, constructing an object representing one of
// multiple types of messages in a std::variant and emplacing it in some data
// structure to be consumed elsewhere
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <span>
#include <variant>

#include "Auxil.hpp"
#include "FIXMsgTypeChecks.hpp"

namespace FIXSocketHandler {
template <typename MsgClassPack, typename SocketType_>
  requires Auxil::ReadableAsSocket<SocketType_> &&
           MsgTypeChecks::MsgConsistencyCheck_v<
               typename param_pack::generate_type_pack_t<MsgClassPack>> &&
           MsgTypeChecks::AlternativesDefaultConstructible<
               typename param_pack::generate_type_pack_t<MsgClassPack>>

struct FIXSocketHandler {
private:
  using MsgClassPack_ = param_pack::generate_type_pack_t<MsgClassPack>;
  using MsgClassVariant_ =
      MsgClassPack_::template specialize_template_t<std::variant>;
  // static data about byte layout of FIX messages
  static constexpr std::uint32_t delimit_offset =
      std::variant_alternative_t<0, MsgClassVariant_>::delimiter_offset;
  static constexpr std::uint16_t delimit_value =
      std::variant_alternative_t<0, MsgClassVariant_>::delimiter_value;
  static constexpr std::uint32_t length_offset =
      std::variant_alternative_t<0, MsgClassVariant_>::length_offset;
  static constexpr std::uint8_t length_len = sizeof(
      decltype(std::variant_alternative_t<0, MsgClassVariant_>::length_offset));
  static constexpr std::uint32_t header_length =
      std::variant_alternative_t<0, MsgClassVariant_>::header_length;
  static constexpr std::uint32_t buffer_size =
      MsgTypeChecks::determine_max_msg_len<MsgClassVariant_>;
  static constexpr std::uint32_t shortest_msg_size =
      MsgTypeChecks::determine_min_msg_len<MsgClassVariant_>;
  static_assert(shortest_msg_size > header_length);
  // index sequence used to generate static "loop" over message types
  static constexpr auto index_seq =
      std::make_index_sequence<std::variant_size_v<MsgClassVariant_>>();
  void *read_buffer = nullptr;
  // span used to access buffer
  std::span<std::uint8_t, buffer_size> buffer_span;
  SocketType_ *socket_ptr;
  __attribute__((flatten)) bool validate_checksum(
      const std::uint32_t,
      const std::span<std::uint8_t, buffer_size> &) const noexcept;
  // finds delimiter of next message if it isn't found in the expected place
  __attribute__((noinline)) int scan_for_delimiter() noexcept;
  // safely discards excess bytes if an incoming message is longer than any of
  // the message types passed to buffer (and hence longer than buffer) returns
  // size of buffer for caller to assign to message-length variable so all
  // remaining bytes can be read within regular program logic
  __attribute__((noinline)) std::uint32_t
  discard_long_msg(const std::uint32_t) noexcept;

  // member function templates to generate "loop" over all message types to find
  // and construct the correct one
  template <std::size_t... Is>
  int determine_msg_type(const int, std::index_sequence<Is...>) const noexcept;

  template <std::size_t... Is>
  MsgClassVariant_ construct_variant(const int, const std::span<std::uint8_t>,
                                     std::index_sequence<Is...>) const noexcept;

public:
  explicit FIXSocketHandler(SocketType_ *);
  ~FIXSocketHandler();
  FIXSocketHandler(const FIXSocketHandler &) = delete;
  FIXSocketHandler(FIXSocketHandler &&) = delete;
  FIXSocketHandler &operator=(const FIXSocketHandler &) = delete;
  FIXSocketHandler &operator=(FIXSocketHandler &&) = delete;
  using MsgClassVariant = MsgClassVariant_;
  using SocketType = SocketType_;
  // public interface for reading FIX messages from socket
  __attribute__((flatten)) std::optional<MsgClassVariant>
  read_next_message() noexcept;
};
}; // namespace FIXSocketHandler

#define TEMPL_TYPES                                                            \
  template <typename MsgClassPack, typename SocketType_>                       \
    requires Auxil::ReadableAsSocket<SocketType_> &&                           \
             MsgTypeChecks::MsgConsistencyCheck_v<                             \
                 typename param_pack::generate_type_pack_t<MsgClassPack>> &&   \
             MsgTypeChecks::AlternativesDefaultConstructible<                  \
                 typename param_pack::generate_type_pack_t<MsgClassPack>>

#define SOCK_HANDLER                                                           \
  FIXSocketHandler::FIXSocketHandler<MsgClassPack, SocketType_>

TEMPL_TYPES
std::optional<typename SOCK_HANDLER::MsgClassVariant_>
SOCK_HANDLER::read_next_message() noexcept {
  // read length-of-header bytes into buffer
  std::int32_t bytes_read = this->socket_ptr->recv(read_buffer, header_length);

  // verifiy that header is valid by checking whether delimeter sequence is in
  // expected place
  const auto delimit_span = std::span<const std::uint8_t, 2>{
      reinterpret_cast<const std::uint8_t *>(&delimit_value), 2};

  // determine if bytes read are a valid header, find valid header if they are
  // not
  const bool valid_header = std::ranges::equal(
      this->buffer_span.subspan(delimit_offset, 2), delimit_span);
  bytes_read = valid_header ? bytes_read : scan_for_delimiter();

  // extract message length from message header
  int msg_length =
      *(std::uint32_t *)(reinterpret_cast<void *>(&buffer_span[length_offset]));
  msg_length = msg_length <= buffer_size ? msg_length
                                         : this->discard_long_msg(msg_length);

  // read rest of message into buffer
  void *recv_dest = reinterpret_cast<void *>(&this->buffer_span[bytes_read]);
  this->socket_ptr->recv(recv_dest, msg_length - bytes_read);

  // determine type of received message by checking all types in variant
  static constexpr auto variant_index_seq =
      std::make_index_sequence<std::variant_size_v<MsgClassVariant_>>();
  int msg_type_index = determine_msg_type(msg_length, variant_index_seq);

  // validate message via checksum, if invalid message is discarded by setting
  // type-index to -1, indicating cold path
  msg_type_index =
      validate_checksum(msg_length, buffer_span) ? msg_type_index : -1;

  // enqueue message to buffer
  const auto return_variant = this->construct_variant(
      msg_type_index, this->buffer_span, variant_index_seq);
  std::optional<MsgClassVariant> ret =
      msg_type_index >= 0 ? std::make_optional(return_variant) : std::nullopt;
  return ret;
};

TEMPL_TYPES
bool SOCK_HANDLER::validate_checksum(
    const std::uint32_t msg_length,
    const std::span<std::uint8_t, buffer_size> &buffer) const noexcept {
  const auto check_sum_subspan = buffer.subspan(0, msg_length - 3);
  const std::uint8_t byte_sum =
      std::accumulate(check_sum_subspan.begin(), check_sum_subspan.end(), 0);
  // convert three ASCII characters at the end of FIX msg to unsigned 8 bit int
  // to compare to checksum computed above
  const std::uint8_t msg_checksum =
      ((this->buffer_span[msg_length - 3]) - 48) * 100 +
      ((buffer_span[msg_length - 2]) - 48) * 10 +
      ((buffer_span[msg_length - 1]) - 48);
  return byte_sum == msg_checksum;
};

TEMPL_TYPES
int SOCK_HANDLER::scan_for_delimiter() noexcept {
  static constexpr int bytes_to_keep{header_length - 1};
  auto search_span = this->buffer_span.subspan(0, shortest_msg_size);
  std::shift_left(search_span.begin(), search_span.end(), 1);
  void *recv_dest = reinterpret_cast<void *>(&search_span[bytes_to_keep]);
  this->socket_ptr->recv(recv_dest, shortest_msg_size - bytes_to_keep);
  const std::uint16_t delimit_val_local = delimit_value;
  const auto delimit_val_span = std::span<const std::uint8_t, 2>{
      reinterpret_cast<const std::uint8_t *>(&delimit_val_local), 2};
  auto search_result = std::ranges::search(search_span, delimit_val_span);
  bool delimiter_found = !search_result.empty();
  while (!delimiter_found) {
    std::shift_left(search_span.begin(), search_span.end(),
                    shortest_msg_size - bytes_to_keep);
    this->socket_ptr->recv(recv_dest, shortest_msg_size - bytes_to_keep);
    search_result = std::ranges::search(search_span, delimit_val_span);
    delimiter_found = !search_result.empty();
  }
  const std::uint32_t position_of_header =
      std::distance(search_span.begin(), std::ranges::get<0>(search_result)) -
      delimit_offset;
  std::shift_left(search_span.begin(), search_span.end(), position_of_header);
  return shortest_msg_size - position_of_header;
};

TEMPL_TYPES
std::uint32_t
SOCK_HANDLER::discard_long_msg(const std::uint32_t msg_length) noexcept {
  int excess_bytes = msg_length - buffer_size;
  const int available_buffer = buffer_size - header_length;
  std::uint32_t bytes_read{0};
  while (excess_bytes > 0) {
    bytes_read = this->socket_ptr->recv(
        reinterpret_cast<void *>(&this->buffer_span[header_length]),
        std::min(available_buffer, excess_bytes));
    excess_bytes -= bytes_read;
  }
  return this->buffer_size;
};

// returns index of matching alternative, -1 if no match found
// argument var not needed for logic at runtime, but necessary to allow implicit
// deduction of variant type Variant_  and implicit template specialization
// this allows the compiler to deduce all the indices in Is
TEMPL_TYPES
template <std::size_t... Is>
int SOCK_HANDLER::determine_msg_type(
    const int compare_to, std::index_sequence<Is...>) const noexcept {
  int ret = -1;
  // lambda needs to be invoked with trailing "()" to work within fold
  // expression
  (
      [&]() {
        if (compare_to ==
            std::variant_alternative_t<Is, MsgClassVariant>::msg_length) {
          ret = Is;
        }
      }(),
      ...);
  return ret;
};

// returns variant containing message class specified by index argument, default
// constructed variant if index is not valid for variant
TEMPL_TYPES
template <std::size_t... Is>
typename SOCK_HANDLER::MsgClassVariant
SOCK_HANDLER::construct_variant(const int variant_index,
                                const std::span<std::uint8_t> argument,
                                std::index_sequence<Is...>) const noexcept {
  MsgClassVariant ret;
  (
      [&]() {
        if (Is == variant_index) {
          ret.template emplace<
              std::variant_alternative_t<Is, MsgClassVariant_>>(argument);
        }
      }(),
      ...);
  return ret;
};

TEMPL_TYPES
SOCK_HANDLER::FIXSocketHandler(SocketType *socket_ptr_arg)
    : socket_ptr{socket_ptr_arg},
      buffer_span{reinterpret_cast<std::uint8_t *>(read_buffer), buffer_size} {
  this->read_buffer = std::aligned_alloc(64, buffer_size);
  this->buffer_span = std::span<std::uint8_t, buffer_size>{
      reinterpret_cast<std::uint8_t *>(read_buffer), buffer_size};
};

TEMPL_TYPES
SOCK_HANDLER::~FIXSocketHandler() { std::free(this->read_buffer); };

#undef TEMPL_TYPES
#undef SOCK_HANDLER

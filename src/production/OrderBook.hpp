#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>

#include "Auxil.hpp"
#include "FIXMsgTypeChecks.hpp"
#include "Guards.hpp"
#include "OrderBookBucket.hpp"

namespace OrderBook {
template <typename MsgClassPack, typename EntryType_,
          std::uint32_t book_length_, bool exclusive_cacheline_>
  requires std::signed_integral<EntryType_> &&
           // apply check for required FIX message class functionality and
           // interfaces to every type contained in MsgClassVariant_
           // book length and BucketType are needed to initialize order-handling
           // function templates of message types which in turn is necessary to
           // verify that they satisfy requirements when initialized at compile
           // time
           param_pack::type_pack_convertible_v<MsgClassPack> &&
           MsgTypeChecks::has_FIX_msg_handling_interface<
               param_pack::generate_type_pack_t<MsgClassPack>,
               OrderBookBucket::OrderBookBucket<
                   Auxil::cache_optimal_alignment<
                       OrderBookBucket::OrderBookBucket<0, EntryType_>>(
                       64, exclusive_cacheline_),
                   EntryType_>,
               book_length_>

struct OrderBook {
private:
  // static members and member types
  static constexpr size_t alignment = Auxil::cache_optimal_alignment<
      OrderBookBucket::OrderBookBucket<0, EntryType_>>(64,
                                                       exclusive_cacheline_);
  using BucketType = OrderBookBucket::OrderBookBucket<alignment, EntryType_>;
  using MsgClassVariant_ = param_pack::generate_type_pack_t<
      MsgClassPack>::template specialize_template_t<std::variant>;
  using OrderClassTuple = param_pack::generate_type_pack_t<
      MsgClassPack>::template specialize_template_t<std::tuple>;
  ;
  // contains: marginal execution price, filled volume, total revenue
  // generated(price*volume), 1 if price is out of bounds of the orderbook
  using OrderResponse =
      std::tuple<std::uint32_t, EntryType_, EntryType_, std::uint8_t>;
  // combines std::optionals that keep track of best bid, lowest bid, best offer
  // and highest offer
  // non static member variables
  std::uint32_t base_price;
  // contains std::optionals that keep track of best bid, lowest bid, best offer
  // and highest offer
  std::optional<std::uint32_t> stats_array[4];
  const std::span<std::optional<std::uint32_t>, 4> stats_span;
  alignas(64) std::atomic_flag modify_lock = false;
  std::atomic<std::int64_t> version_counter = 0;
  // points to memory location of the actual orderbook entries
  void *const memory_pointer;
  const std::span<BucketType> mem_span;
  // index sequence to specialize member function templates below on number of
  // differenct messages
  static constexpr auto n_msg_types_seq =
      std::make_index_sequence<std::variant_size_v<MsgClassVariant_>>();

  template <size_t... Is>
  __attribute__((flatten)) OrderClassTuple
  make_tuple_from_variant(const MsgClassVariant_ &,
                          std::index_sequence<Is...>) const noexcept;

  template <size_t... Is>
  __attribute__((flatten)) OrderResponse
  call_order_handle_methods(const OrderClassTuple &,
                            std::index_sequence<Is...>) noexcept;

public:
  // static members and member types
  using EntryType = EntryType_;
  using MsgClassVariant = MsgClassVariant_;
  static constexpr std::uint32_t book_length = book_length_;
  static constexpr std::uint32_t byte_length = book_length * alignment;

  // 5 special members
  explicit OrderBook(std::uint32_t);
  ~OrderBook();
  OrderBook(const OrderBook &) = delete;
  OrderBook &operator=(const OrderBook &) = delete;
  OrderBook(OrderBook &&) = delete;
  OrderBook &operator=(OrderBook &&) = delete;

  // public member functions
  __attribute__((flatten))
  OrderResponse process_order(MsgClassVariant) noexcept;
  std::tuple<std::optional<std::uint32_t>, std::optional<std::uint32_t>>
  best_bid_ask() const noexcept;
  EntryType volume_at_price(const std::uint32_t) const noexcept;
  bool shift_book(const std::int32_t) noexcept;
  std::string __invariants_check(int = 0) const;
};
} // namespace OrderBook

#define ORDER_BOOK_TEMPLATE_DECLARATION                                        \
  template <typename MsgClassPack, typename EntryType_,                        \
            std::uint32_t book_length_, bool exclusive_cacheline_>             \
    requires std::signed_integral<EntryType_> &&                               \
             param_pack::type_pack_convertible_v<MsgClassPack> &&              \
             MsgTypeChecks::has_FIX_msg_handling_interface<                    \
                 param_pack::generate_type_pack_t<MsgClassPack>,               \
                 OrderBookBucket::OrderBookBucket<                             \
                     Auxil::cache_optimal_alignment<                           \
                         OrderBookBucket::OrderBookBucket<0, EntryType_>>(     \
                         64, exclusive_cacheline_),                            \
                     EntryType_>,                                              \
                 book_length_>

#define ORDER_BOOK                                                             \
  OrderBook::OrderBook<MsgClassPack, EntryType_, book_length_,                 \
                       exclusive_cacheline_>

// namespace OrderBook {

ORDER_BOOK_TEMPLATE_DECLARATION
ORDER_BOOK::OrderBook(std::uint32_t base_price_)
    : memory_pointer{std::aligned_alloc(alignment, byte_length)},
      mem_span{std::span<BucketType>{
          reinterpret_cast<BucketType *>(this->memory_pointer), book_length}},
      stats_span{stats_array}, base_price{base_price_} {
  // initialize all bucket objects
  std::ranges::fill(this->mem_span, BucketType());
};

ORDER_BOOK_TEMPLATE_DECLARATION
ORDER_BOOK::~OrderBook() {
  // free allocated memory (RAII)
  std::free(this->memory_pointer);
};

ORDER_BOOK_TEMPLATE_DECLARATION
template <size_t... Is>
typename ORDER_BOOK::OrderClassTuple
ORDER_BOOK::make_tuple_from_variant(const MsgClassVariant_ &msg_var,
                                    std::index_sequence<Is...>) const noexcept {
  // same type as OrderBook::OrderClassTuple
  OrderClassTuple ret;
  (
      [&]() {
        if (msg_var.index() == Is) {
          std::get<Is>(ret) = std::get<Is>(msg_var);
        }
      }(),
      ...);
  return ret;
};

ORDER_BOOK_TEMPLATE_DECLARATION
template <size_t... Is>
typename ORDER_BOOK::OrderResponse
ORDER_BOOK::call_order_handle_methods(const OrderClassTuple &order_tpl,
                                      std::index_sequence<Is...>) noexcept {
  OrderResponse ret_resp;
  (
      [&]() {
        OrderResponse local_resp;
        local_resp = std::get<Is>(order_tpl)
                         .template handle_order<BucketType, book_length>(
                             std::get<Is>(order_tpl), this->base_price,
                             this->mem_span, stats_span);
        Auxil::add_to_tuple(ret_resp, local_resp);
      }(),
      ...);
  return ret_resp;
};

ORDER_BOOK_TEMPLATE_DECLARATION
typename ORDER_BOOK::OrderResponse
ORDER_BOOK::process_order(MsgClassVariant order) noexcept {
  OrderResponse ret;
  // create tuple of blank orders for each type and insert the order to be
  // processed in correct spot
  auto orders = this->make_tuple_from_variant(order, n_msg_types_seq);

  // acquire write access to order book, set version counter to odd value to
  // invalidate reads while order book is manipulated
  auto guard = Guards::AtomicFlagGuard(&this->modify_lock);
  guard.lock();
  this->version_counter.fetch_add(1, std::memory_order_acquire);

  // call handler member functions for all order types, use blank orders for
  // irrelevant types, update statistics
  ret = call_order_handle_methods(orders, n_msg_types_seq);

  // release lock, set increment counter to even value
  this->version_counter.fetch_add(1, std::memory_order_release);
  guard.unlock();

  return ret;
};

ORDER_BOOK_TEMPLATE_DECLARATION
std::tuple<std::optional<std::uint32_t>, std::optional<std::uint32_t>>
ORDER_BOOK::best_bid_ask() const noexcept {
  auto ret =
      std::tuple<std::optional<std::uint32_t>, std::optional<std::uint32_t>>();
  std::int64_t init_version{0};
  std::atomic<std::int64_t> fin_version{0};
  // safe version before reading
  do {
    init_version = this->version_counter.load(std::memory_order_acquire);
    ret =
        std::tuple<std::optional<std::uint32_t>, std::optional<std::uint32_t>>{
            this->stats_span[0], this->stats_span[2]};
    fin_version.store(this->version_counter, std::memory_order_release);
  } while (init_version % 2 || init_version != fin_version);
  return ret;
};

ORDER_BOOK_TEMPLATE_DECLARATION
typename ORDER_BOOK::EntryType
ORDER_BOOK::volume_at_price(const std::uint32_t price) const noexcept {
  std::int64_t init_version;
  std::atomic<std::int64_t> fin_version;
  bool in_range;
  std::uint32_t access_at_index;
  EntryType ret;
  do {
    init_version = this->version_counter.load(std::memory_order_acquire);
    in_range =
        price >= this->base_price && price <= (this->base_price + book_length);
    access_at_index = price - this->base_price * in_range + 0 * !in_range;
    // avoid out of bounds memory access if volume is read while book is in the
    // process of shifting
    access_at_index = std::min(access_at_index, book_length);
    ret = in_range ? this->mem_span[access_at_index].get_volume() : 0;
    fin_version.store(this->version_counter, std::memory_order_release);
  } while (!init_version % 2 || init_version != fin_version);
  return ret;
};

ORDER_BOOK_TEMPLATE_DECLARATION
bool ORDER_BOOK::shift_book(const std::int32_t shift_by) noexcept {
  const bool shift_up = shift_by >= 0;
  const bool pos_base_price =
      (static_cast<std::int32_t>(this->base_price) + shift_by) >= 0;
  // acquire exclusive/write access to order book, reads still possible until
  // version_counter is incremented
  auto guard = Guards::AtomicFlagGuard(&this->modify_lock);
  guard.lock();
  // check if there are enough empty buckets to spare when book is shifted so no
  // information is lost, negative shift
  const std::int32_t max_price = this->base_price + book_length;
  const std::int32_t free_buckets_top =
      max_price - static_cast<std::int32_t>(this->stats_span[3].value_or(
                      this->stats_span[0].value_or(this->base_price)));
  const std::int32_t free_buckets_bottom =
      static_cast<std::int32_t>(this->stats_span[1].value_or(
          this->stats_span[2].value_or(max_price))) -
      this->base_price;
  const bool sufficient_distance_shift_up =
      shift_up && free_buckets_bottom >= shift_by;
  const bool sufficient_distance_shift_down =
      !shift_up && free_buckets_top >= -shift_by;
  // shift is always possible if the order book is empty and base price remains
  // >= 0
  const bool zero_volume =
      !this->stats_span[0].has_value() && !this->stats_span[2].has_value();
  const bool shift_possible = (sufficient_distance_shift_up ||
                               sufficient_distance_shift_down || zero_volume) &&
                              pos_base_price;
  // no need to perform actual shift if book is empty, just adjust base_price
  const std::int32_t mem_shift = shift_by * shift_possible * !zero_volume;
  // enter critical section for reads
  this->version_counter.fetch_add(1, std::memory_order_acquire);
  std::shift_left(this->mem_span.begin(), this->mem_span.end(),
                  shift_up * mem_shift);
  std::shift_right(this->mem_span.begin(), this->mem_span.end(),
                   (-1) * !shift_up * mem_shift);
  this->base_price += shift_by * shift_possible;
  this->version_counter.fetch_add(1, std::memory_order_release);
  return shift_possible;
};

ORDER_BOOK_TEMPLATE_DECLARATION
std::string ORDER_BOOK::__invariants_check(int n_it) const {
  auto best_bid = this->stats_span[0];
  auto lowest_bid = this->stats_span[1];
  auto best_offer = this->stats_span[2];
  auto highest_offer = this->stats_span[3];
  std::stringstream error_msgs;
  auto is_positive = [](BucketType b) { return b.get_volume() >= 0; };
  auto is_negative = [](BucketType b) { return b.get_volume() <= 0; };
  auto is_zero = [](BucketType b) { return b.get_volume() == 0; };
  auto invalid_stat = [&](std::optional<std::uint32_t> stat) {
    return stat.has_value() &&
           mem_span[stat.value() - base_price].get_volume() == 0;
  };

  if (best_bid.has_value() && lowest_bid.has_value()) {
    const std::uint32_t lb_index = lowest_bid.value() - base_price;
    const std::uint32_t bb_index = best_bid.value() - base_price;
    auto lb_subspan = mem_span.subspan(0, lb_index);
    auto bb_subspan =
        mem_span.subspan(bb_index + 1, book_length - bb_index - 1);
    const bool wrong_below = !std::ranges::all_of(lb_subspan, is_zero);
    const bool wrong_above = !std::ranges::all_of(bb_subspan, is_positive);

    if (wrong_above) {
      error_msgs << "invalid entry above best bid at index " << n_it << "\n";
    };

    if (wrong_below) {
      error_msgs << "invalid entry below lowest bid at index " << n_it << "\n";
    };
  };

  if (best_offer.has_value() && highest_offer.has_value()) {
    std::uint32_t ho_index = highest_offer.value() - base_price;
    std::uint32_t bo_index = best_offer.value() - base_price;
    auto bo_subspan = mem_span.subspan(0, bo_index);
    auto ho_subspan =
        mem_span.subspan(ho_index + 1, book_length - ho_index - 1);
    const bool wrong_above = !std::ranges::all_of(ho_subspan, is_zero);
    const bool wrong_below = !std::ranges::all_of(bo_subspan, is_negative);
    if (wrong_above) {
      error_msgs << "invalid entry above highest offer at index " << n_it
                 << "\n";
    };

    if (wrong_below) {
      error_msgs << "invalid entry below best offer at index " << n_it << "\n";
    };
  };

  if (best_bid.has_value() && best_offer.has_value() &&
      (best_bid.value() >= best_offer.value())) {
    error_msgs << "best bid >= best offer at index " << n_it << "\n";
  };

  if (best_bid.has_value() != lowest_bid.has_value()) {
    error_msgs << "best bid.has_value() != lowest bid.has_value() at index "
               << n_it << "\n";
  };

  if (best_offer.has_value() != highest_offer.has_value()) {
    error_msgs
        << "best offer.has_value() != highest offer.has_value() at index "
        << n_it << "\n";
  };

  if (best_bid.value_or(0) < lowest_bid.value_or(0)) {
    error_msgs << "best bid < lowest bid at index " << n_it << "\n";
  };

  if (best_offer.value_or(0) > highest_offer.value_or(0)) {
    error_msgs << "best offer > highest offer at index " << n_it << "\n";
  };

  if (invalid_stat(lowest_bid)) {
    error_msgs << "no liquidity at lowest bid at index " << n_it << "\n";
  };

  if (invalid_stat(best_bid)) {
    error_msgs << "no liquidity at best bid at index " << n_it << "\n";
  };

  if (invalid_stat(best_offer)) {
    error_msgs << "no liquidity at best offer at index " << n_it << "\n";
  };

  if (invalid_stat(highest_offer)) {
    error_msgs << "no liquidity at highest offer at index " << n_it << "\n";
  };

  return error_msgs.str();
};

// namespace OrderBook

#undef ORDER_BOOK_TEMPLATE_DECLARATION
#undef ORDER_BOOK

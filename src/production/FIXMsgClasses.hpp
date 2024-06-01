#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <tuple>

#include "OrderBookBucket.hpp"
#include "OrderBookHelpers.hpp"

namespace FIXMsgClasses {
struct AddLimitOrder {
private:
  std::int32_t order_volume = 0;
  std::uint32_t order_price = 0;
public:
  static constexpr std::uint32_t msg_length{17};
  static constexpr std::uint32_t length_offset{0};
  static constexpr std::uint32_t header_length{6};
  static constexpr std::uint32_t delimiter_offset{4};
  static constexpr std::uint32_t delimiter_value{0xEB50};
  static constexpr std::uint32_t volume_offset{6};
  static constexpr std::uint32_t price_offset{10};
  std::int32_t get_order_volume() const noexcept;
  std::uint32_t get_order_price() const noexcept;
  explicit AddLimitOrder(std::int32_t, std::uint32_t);
  explicit AddLimitOrder(const std::span<std::uint8_t> &);
  explicit AddLimitOrder() = default;

  using StatsOptType = std::optional<std::uint32_t>;
  using StatsSpanType = const std::span<StatsOptType, 4> &;

  template <typename BucketType>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  using OrderResponseType =
      std::tuple<std::uint32_t, typename BucketType::EntryType,
                 typename BucketType::EntryType, std::uint8_t>;

  template <typename BucketType, std::uint32_t book_length>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  __attribute__((flatten)) static OrderResponseType<BucketType>
  handle_order(const AddLimitOrder &, const std::uint32_t,
               const std::span<BucketType> &, StatsSpanType) noexcept;
};

struct WithdrawLimitOrder {
private:
  std::int32_t order_volume = 0;
  std::uint32_t order_price = 0;
public:
  static constexpr std::uint32_t msg_length{18};
  static constexpr std::uint32_t length_offset{0};
  static constexpr std::uint32_t header_length{6};
  static constexpr std::uint32_t delimiter_offset{4};
  static constexpr std::uint16_t delimiter_length{2};
  static constexpr std::uint16_t delimiter_value{0xEB50};
  static constexpr std::uint32_t volume_offset{6};
  static constexpr std::uint32_t price_offset{10};
  std::int32_t get_order_volume() const noexcept;
  std::uint32_t get_order_price() const noexcept;
  explicit WithdrawLimitOrder(std::int32_t, std::uint32_t);
  explicit WithdrawLimitOrder(const std::span<std::uint8_t> &);
  explicit WithdrawLimitOrder() = default;

  using StatsOptType = std::optional<std::uint32_t>;
  using StatsSpanType = const std::span<StatsOptType, 4> &;

  template <typename BucketType>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  using OrderResponseType =
      std::tuple<std::uint32_t, typename BucketType::EntryType,
                 typename BucketType::EntryType, std::uint8_t>;

  template <typename BucketType, std::uint32_t book_length>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  __attribute__((flatten)) static OrderResponseType<BucketType>
  handle_order(const WithdrawLimitOrder &, const std::uint32_t,
               const std::span<BucketType> &, StatsSpanType) noexcept;
};

struct MarketOrder {
private:
  std::int32_t order_volume = 0;
public:
  static constexpr std::uint32_t msg_length{13};
  static constexpr std::uint32_t length_offset{0};
  static constexpr std::uint32_t header_length{6};
  static constexpr std::uint32_t delimiter_offset{4};
  static constexpr std::uint32_t delimiter_value{0xEB50};
  static constexpr std::uint32_t volume_offset{6};
  std::int32_t get_order_volume() const noexcept;

  explicit MarketOrder(std::int32_t);
  explicit MarketOrder(const std::span<std::uint8_t> &);
  explicit MarketOrder() = default;

  using StatsOptType = std::optional<std::uint32_t>;
  using StatsSpanType = const std::span<StatsOptType, 4> &;
  template <typename BucketType>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  using OrderResponseType =
      std::tuple<std::uint32_t, typename BucketType::EntryType,
                 typename BucketType::EntryType, std::uint8_t>;

  template <typename BucketType, std::uint32_t book_length>
    requires OrderBookBucket::IsOrderBookBucket<BucketType>
  static OrderResponseType<BucketType> __attribute__((flatten))
  handle_order(const MarketOrder &, const std::uint32_t,
               const std::span<BucketType> &, StatsSpanType) noexcept;
};
}; // namespace FIXMsgClasses

namespace FIXMsgClasses {
AddLimitOrder::AddLimitOrder(const std::span<std::uint8_t> &buffer_span): order_volume{*reinterpret_cast<std::int32_t *>(&buffer_span[volume_offset])},
order_price{*reinterpret_cast<std::uint32_t *>(&buffer_span[price_offset])}{};

AddLimitOrder::AddLimitOrder(std::int32_t order_volume_,
                             std::uint32_t order_price_)
    : order_volume{order_volume_}, order_price{order_price_} {};

std::uint32_t AddLimitOrder::get_order_price() const noexcept{
  return this->order_price;
};

std::int32_t AddLimitOrder::get_order_volume() const noexcept{
  return this->order_volume;
};

template <typename BucketType, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType>
AddLimitOrder::OrderResponseType<BucketType> AddLimitOrder::handle_order(
    const AddLimitOrder &order, const std::uint32_t base_price,
    const std::span<BucketType> &mem_span, StatsSpanType stats) noexcept {
  std::uint32_t price = order.get_order_price();
  // unpack stats from tuple, use reference types to avoid repackaging after
  // updating
  auto &best_bid = stats[0];
  auto &lowest_bid = stats[1];
  auto &best_offer = stats[2];
  auto &highest_offer = stats[3];
  // check if price is out of range, if so set order volume to 0
  // and price to base price to avoid out of bounds memory access, then proceed
  // regularly
  const std::int32_t order_volume = order.get_order_volume();
  const bool price_in_range =
      order_volume == 0 ||
      OrderBookHelpers<BucketType, book_length>::price_in_range(price,
                                                                base_price);
  price = price * price_in_range + base_price * !price_in_range;
  const std::int32_t volume = order_volume * price_in_range;
  const bool dummy_order = volume == 0;
  const bool buy_order = volume < 0;

  // fill order volume immediately if possible
  const bool crossed_book =
      !dummy_order *
      ((buy_order && best_offer.has_value() &&
        (price >= best_offer.value_or(0))) ||
       (!buy_order && best_bid.has_value() && (price <= best_bid.value_or(0))));
  OrderResponseType<BucketType> ret{0, 0, 0, 0 + 1 * !price_in_range};
  const std::int32_t run_end_price =
      !crossed_book * (-1) +
      crossed_book * (buy_order * highest_offer.value_or(-1) +
                      !buy_order * lowest_bid.value_or(-1));
  ret = OrderBookHelpers<BucketType, book_length>::run_through_book(
      volume, run_end_price, base_price, mem_span, stats);

  // insert error code if price is out of range
  std::get<3>(ret) += 1 * !price_in_range;

  // enter liquidity into order book
  const typename BucketType::EntryType unfilled_vol = volume - std::get<1>(ret);
  const bool liq_added = unfilled_vol != 0;
  !dummy_order ? mem_span[price - base_price].add_liquidity(unfilled_vol)
               : void();

  // check which statistic (if any) needs to be updated
  const bool update_lowest_bid =
      liq_added && buy_order &&
      (!lowest_bid.has_value() || lowest_bid.value() > price);
  const bool update_best_bid =
      liq_added && buy_order &&
      (!best_bid.has_value() || best_bid.value() < price);
  const bool update_highest_offer =
      liq_added && !buy_order &&
      (!highest_offer.has_value() || highest_offer.value() < price);
  const bool update_best_offer =
      liq_added && !buy_order &&
      (!best_offer.has_value() || best_offer.value() > price);

  // perform update
  lowest_bid = update_lowest_bid ? lowest_bid.emplace(price) : lowest_bid;
  best_bid = update_best_bid ? best_bid.emplace(price) : best_bid;
  highest_offer =
      update_highest_offer ? highest_offer.emplace(price) : highest_offer;
  best_offer = update_best_offer ? best_offer.emplace(price) : best_offer;

  return ret;
};

WithdrawLimitOrder::WithdrawLimitOrder(
    const std::span<std::uint8_t> &buffer_span):
    order_volume{*reinterpret_cast<std::int32_t *>(&buffer_span[volume_offset])},
    order_price{*reinterpret_cast<std::uint32_t *>(&buffer_span[price_offset])}{};

WithdrawLimitOrder::WithdrawLimitOrder(std::int32_t order_volume_,
                                       std::uint32_t order_price_)
    : order_volume{order_volume_}, order_price{order_price_} {};

std::uint32_t WithdrawLimitOrder::get_order_price() const noexcept{
  return this->order_price;
};

std::int32_t WithdrawLimitOrder::get_order_volume() const noexcept{
  return this->order_volume;
};

template <typename BucketType, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType>
WithdrawLimitOrder::OrderResponseType<BucketType>
WithdrawLimitOrder::handle_order(const WithdrawLimitOrder &order,
                                 const std::uint32_t base_price,
                                 const std::span<BucketType> &mem_span,
                                 StatsSpanType stats) noexcept {
  // unpack stats from tuple, use reference types to avoid repackaging after
  // updating
  auto &best_bid = stats[0];
  auto &lowest_bid = stats[1];
  auto &best_offer = stats[2];
  auto &highest_offer = stats[3];
  std::uint32_t price = order.get_order_price();
  const std::int32_t order_volume = order.get_order_volume();
  // check if price is out of range, if so set order volume to 0 but proceed
  // set price_in_range to true for volume == 0 to avoid error code in order
  // response for default/dummy withdraw messages
  const bool has_volume = order_volume != 0;
  const bool price_in_range =
      !has_volume || OrderBookHelpers<BucketType, book_length>::price_in_range(
                         price, base_price);
  price = price * price_in_range + base_price * !price_in_range;
  const std::int32_t volume = order_volume * price_in_range;

  // perform actual withdrawel
  const std::int32_t volume_withdrawn =
      has_volume ? mem_span[price - base_price].consume_liquidity(volume) : 0;

  // upate stats if necessary
  const bool liquidity_exhausted =
      !has_volume || mem_span[price - base_price].get_volume() == 0;
  best_bid = best_bid.and_then([&](std::uint32_t arg) {
    return (price != arg || !liquidity_exhausted)
               ? std::make_optional(arg)
               : OrderBookHelpers<BucketType, book_length>::find_liquid_bucket(
                     price, lowest_bid, base_price, mem_span);
  });
  lowest_bid = lowest_bid.and_then([&](std::uint32_t arg) {
    return (price != arg || !liquidity_exhausted)
               ? std::make_optional(arg)
               : OrderBookHelpers<BucketType, book_length>::find_liquid_bucket(
                     price, best_bid, base_price, mem_span);
  });
  best_offer = best_offer.and_then([&](std::uint32_t arg) {
    return (price != arg || !liquidity_exhausted)
               ? std::make_optional(arg)
               : OrderBookHelpers<BucketType, book_length>::find_liquid_bucket(
                     price, highest_offer, base_price, mem_span);
  });
  highest_offer = highest_offer.and_then([&](std::uint32_t arg) {
    return (price != arg || !liquidity_exhausted)
               ? std::make_optional(arg)
               : OrderBookHelpers<BucketType, book_length>::find_liquid_bucket(
                     price, best_offer, base_price, mem_span);
  });
  return OrderResponseType<BucketType>{0, volume_withdrawn, 0,
                                       1 * !price_in_range};
};

MarketOrder::MarketOrder(const std::span<std::uint8_t> &buffer_span):
order_volume{*reinterpret_cast<std::int32_t *>(&buffer_span[volume_offset])}{};

MarketOrder::MarketOrder(std::int32_t order_volume_)
    : order_volume{order_volume_} {};

std::int32_t MarketOrder::get_order_volume() const noexcept{
  return this->order_volume;
};

template <typename BucketType, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType>
MarketOrder::OrderResponseType<BucketType> MarketOrder::handle_order(
    const MarketOrder &order, const std::uint32_t base_price,
    const std::span<BucketType> &mem_span, StatsSpanType stats) noexcept {
  // unpack tuple
  auto &lowest_bid = stats[1];
  auto &highest_offer = stats[3];
  const std::int32_t volume = order.get_order_volume();
  const std::int32_t run_end_price =
      -1 * (volume == 0) + (volume < 0) * (highest_offer.value_or(-1)) +
      (volume > 0) * (lowest_bid.value_or(-1));
  return OrderBookHelpers<BucketType, book_length>::run_through_book(
      volume, run_end_price, base_price, mem_span, stats);
};
}; // namespace FIXMsgClasses

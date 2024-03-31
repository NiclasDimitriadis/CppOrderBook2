#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <tuple>

#include "OrderBookBucket.hpp"

template <typename BucketType_, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType_>
struct OrderBookHelpers {

  using BucketType = BucketType_;
  using EntryType = BucketType::EntryType;
  using OrderResponse =
      std::tuple<std::uint32_t, EntryType, EntryType, std::uint8_t>;
  using StatsOptType = std::optional<std::uint32_t>;
  using StatsSpanType = const std::span<StatsOptType, 4>;

  // return std::optional with index of first non empty price bucket from
  // start_index to end_index returns optional containing the price (NOT INDEX!)
  // of liquid bucket, empty optional if search is unsuccesful returns empty
  // optional if std::nullopt is passed as end_index
  __attribute__((flatten)) static std::optional<std::uint32_t>
  find_liquid_bucket(const std::uint32_t, const std::optional<std::uint32_t>,
                     const std::uint32_t,
                     const std::span<BucketType> &) noexcept;

  // iterates over portion of the order book to withdraw liquidity/ fill volume
  // to be used for market orders and limit orders that can be filled
  // immediately
  __attribute__((flatten)) static OrderResponse
  run_through_book(const EntryType, const std::int32_t, const std::uint32_t,
                   const std::span<BucketType> &mem_span,
                   StatsSpanType &) noexcept;

  __attribute__((flatten)) static bool
  price_in_range(const std::uint32_t, const std::uint32_t) noexcept;
};

// return std::optional with index of first non empty price bucket from
// start_index to end_index returns optional containing the price (NOT INDEX!)
// of liquid bucket, empty optional if search is unsuccesful returns empty
// optional if std::nullopt is passed as end_index
template <typename BucketType_, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType_>
std::optional<std::uint32_t>
OrderBookHelpers<BucketType_, book_length>::find_liquid_bucket(
    const std::uint32_t start_price,
    const std::optional<std::uint32_t> end_price,
    const std::uint32_t base_price,
    const std::span<BucketType> &mem_span) noexcept {
  const std::uint32_t start_index{start_price - base_price};
  const std::uint32_t end_index{
      end_price.has_value() ? end_price.value() - base_price : start_index};
  const std::int8_t increment = 1 - 2 * (start_index > end_index);
  [[assume((increment == -1) || (increment == 1))]];
  // skip loop if negative/dummy index was passed
  std::uint32_t ret_price{0}, index{start_index};
  bool end_index_reached{false};
  bool success{false};
  while (!success && !end_index_reached) {
    success = mem_span[index].get_volume() != 0;
    ret_price += success * (base_price + index);
    end_index_reached = index == end_index;
    index += increment;
  };
  return success ? std::make_optional(ret_price) : std::nullopt;
};

// iterates over portion of the order book to withdraw liquidity/ fill volume
// to be used for market orders and limit orders that can be filled
// immediately
template <typename BucketType_, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType_>
__attribute__((flatten))
typename OrderBookHelpers<BucketType_, book_length>::OrderResponse
OrderBookHelpers<BucketType_, book_length>::run_through_book(
    const EntryType volume, const std::int32_t end_price,
    const std::uint32_t base_price, const std::span<BucketType> &mem_span,
    StatsSpanType &stats) noexcept {
  auto &best_bid = stats[0];
  auto &lowest_bid = stats[1];
  auto &best_offer = stats[2];
  auto &highest_offer = stats[3];
  const bool buy_order = volume < 0;
  const std::int32_t start_price =
      buy_order * best_offer.value_or(-1) + !buy_order * best_bid.value_or(-1);
  const bool dummy_run = end_price < 0;
  const std::int8_t increment = -1 + 2 * buy_order;
  std::int32_t n_iterations = (end_price - start_price) * increment;
  EntryType open_vol{volume * !dummy_run}, filled_vol{0}, order_revenue{0};
  std::int32_t index{start_price - static_cast<std::int32_t>(base_price)};
  std::int32_t transaction_index{0};
  while (n_iterations >= 0 && open_vol != 0 && index >= 0) {
    filled_vol = mem_span[index].consume_liquidity(-open_vol);
    transaction_index = filled_vol != 0 ? index : transaction_index;
    order_revenue -= filled_vol * (index + base_price);
    // open volume and filled volume will have opposite signs, addition hence
    // shrinks absolute value of open volume
    open_vol += filled_vol;
    index += increment;
    --n_iterations;
  };
  // calculate end_price args for find_liquid_bucket for both: buy and sell
  // order
  const std::optional<std::uint32_t> flb_end_price_arr[2]{
      lowest_bid.and_then([&](std::uint32_t arg) {
        return dummy_run ? std::nullopt : std::make_optional(arg);
      }),
      highest_offer.and_then([&](std::uint32_t arg) {
        return dummy_run ? std::nullopt : std::make_optional(arg);
      })};
  // update stats for best_bid/best_offer
  best_offer = best_offer.and_then([&](std::uint32_t arg) {
    return buy_order ? find_liquid_bucket(best_offer.value(),
                                          flb_end_price_arr[buy_order],
                                          base_price, mem_span)
                     : arg;
  });
  highest_offer = highest_offer.and_then([&](std::uint32_t arg) {
    return best_offer.has_value() ? std::make_optional(arg) : std::nullopt;
  });

  best_bid = best_bid.and_then([&](std::uint32_t arg) {
    return !buy_order ? find_liquid_bucket(best_bid.value(),
                                           flb_end_price_arr[buy_order],
                                           base_price, mem_span)
                      : arg;
  });
  lowest_bid = lowest_bid.and_then([&](std::uint32_t arg) {
    return best_bid.has_value() ? std::make_optional(arg) : std::nullopt;
  });

  return OrderResponse{!dummy_run * (base_price + transaction_index),
                       !dummy_run * (volume - open_vol), order_revenue, 0};
};

template <typename BucketType_, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType_>
__attribute__((flatten)) bool
OrderBookHelpers<BucketType_, book_length>::price_in_range(
    const std::uint32_t price, const std::uint32_t base_price) noexcept {
  return price <= (base_price + book_length) && price >= base_price;
};

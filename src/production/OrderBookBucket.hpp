#pragma once

#include <cstdint>

#include "Auxil.hpp"

namespace OrderBookBucket {
template <std::uint32_t alignment, typename EntryType_>
  requires std::signed_integral<EntryType_>
struct alignas(alignment) OrderBookBucket {
  using EntryType = EntryType_;

private:
  EntryType volume = 0;

public:
  EntryType consume_liquidity(const EntryType) noexcept;
  void add_liquidity(const EntryType) noexcept;
  EntryType get_volume() const noexcept;
};
}; // namespace OrderBookBucket

namespace OrderBookBucket {
template <std::uint32_t alignment, typename EntryType>
  requires std::signed_integral<EntryType>
EntryType OrderBookBucket<alignment, EntryType>::consume_liquidity(
    const EntryType fill_volume) noexcept {
  const bool demand_liquidity = Auxil::is_negative(this->volume);
  const bool withdraw_demand = Auxil::is_negative(fill_volume);
  const bool liquidity_match = demand_liquidity == withdraw_demand;
  const bool sufficient_liquidity =
      (demand_liquidity && (this->volume <= fill_volume)) ||
      (!demand_liquidity && (volume >= fill_volume));
  const EntryType volume_filled =
      0 + liquidity_match * (fill_volume * sufficient_liquidity +
                             !sufficient_liquidity * this->volume);
  this->volume -= volume_filled;
  return volume_filled;
};

template <std::uint32_t alignment, typename EntryType>
  requires std::signed_integral<EntryType>
void OrderBookBucket<alignment, EntryType>::add_liquidity(
    const EntryType add_volume) noexcept {
  this->volume += add_volume;
};

template <std::uint32_t alignment, typename EntryType>
  requires std::signed_integral<EntryType>
EntryType OrderBookBucket<alignment, EntryType>::get_volume() const noexcept {
  return this->volume;
};

template <typename Bucket, std::uint32_t alignment = 0>
struct IsOrderBookBucketLogic : std::false_type {};

template <std::uint32_t alignment, typename... Ts>
struct IsOrderBookBucketLogic<OrderBookBucket<alignment, Ts...>>
    : std::true_type {};

template <typename T>
constexpr inline bool IsOrderBookBucket =
    IsOrderBookBucketLogic<std::remove_cvref_t<T>>::value;
}; // namespace OrderBookBucket

#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "OrderBookBucket.hpp"
#include "helpers.hpp"
#include "param_pack.hpp"
#include "type_pack_check.hpp"

namespace MsgTypeChecks {

// functionality to ensure every type in a variant has a handle_order() member
// function template as required by the OrderBook class
template <typename FixMsgClass, typename BucketType, std::uint32_t book_length>
concept FixMsgHandlingInterface =
    requires(const FixMsgClass &order, const std::uint32_t base_price,
             const std::span<BucketType> &mem_span,
             const std::span<std::optional<std::uint32_t>, 4> &stats_span) {
      {
        FixMsgClass::template handle_order<BucketType, book_length>(
            order, base_price, mem_span, stats_span)
      }
      -> std::same_as<std::tuple<std::uint32_t, typename BucketType::EntryType,
                                 typename BucketType::EntryType, std::uint8_t>>;
    };

template <typename BucketType, std::uint32_t book_length>
struct FixMsgHandlingInterface_validator {
private:
  template <typename...> struct type_validator {
    static_assert(false);
  };

  // validate single message type
  template <typename MsgType> struct type_validator<MsgType> {
    static constexpr bool value =
        FixMsgHandlingInterface<MsgType, BucketType, book_length>;
  };

  // aggregation
  template <typename Val1, typename Val2>
    requires helpers::specializes_class_template_v<type_validator, Val1> &&
             helpers::specializes_class_template_v<type_validator, Val2>
  struct type_validator<Val1, Val2> {
    static constexpr bool value = Val1::value && Val2::value;
  };

public:
  template <typename MsgCollection>
  static constexpr bool validate =
      type_pack_check::monoid_check_t<type_validator, 1, MsgCollection>::value;
};

template <typename MsgClassPack, typename BucketType, std::uint32_t book_length>
  requires OrderBookBucket::IsOrderBookBucket<BucketType> &&
               param_pack::type_pack_convertible_v<MsgClassPack>
constexpr inline bool has_FIX_msg_handling_interface =
    FixMsgHandlingInterface_validator<BucketType, book_length>::
        template validate<param_pack::generate_type_pack_t<MsgClassPack>>;

template <typename FixMsgClass>
concept FixMsgClassGeneral = requires(const std::span<std::uint8_t> &charSpan) {
  { FixMsgClass::msg_length } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::header_length } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::length_offset } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::length_offset } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::delimiter_value } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::volume_offset } -> std::same_as<const std::uint32_t &>;
  { FixMsgClass::fix_msg_obj(charSpan) } -> std::same_as<FixMsgClass>;
};

template <typename...> struct FixMsgClassGeneralValidator {
  static_assert(false);
};

template <typename MsgType> struct FixMsgClassGeneralValidator<MsgType> {
  static constexpr bool value = FixMsgClassGeneral<MsgType>;
};

template <typename Val1, typename Val2>
  requires helpers::specializes_class_template_v<FixMsgClassGeneralValidator,
                                                 Val1> &&
           helpers::specializes_class_template_v<FixMsgClassGeneralValidator,
                                                 Val2>
struct FixMsgClassGeneralValidator<Val1, Val2> {
  static constexpr bool value = Val1::value && Val2::value;
};

template <typename MsgClassPack>
  requires param_pack::type_pack_convertible_v<MsgClassPack>
constexpr bool FixMsgClassGeneralCheck = type_pack_check::monoid_check_t<
    FixMsgClassGeneralValidator, 1,
    param_pack::generate_type_pack_t<MsgClassPack>>::value;

// check to ensure no two messages in variant have the same message length
template <typename FixMsg1, typename FixMsg2> struct MsgLenNotEqual {
  static constexpr bool value = FixMsg1::msg_length != FixMsg2::msg_length;
};

// check to ensure postion of message length in byte stream is the same across
// all message types in variant
template <typename FixMsg1, typename FixMsg2> struct LenOffsetEqual {
  static constexpr bool value =
      FixMsg1::length_offset == FixMsg1::length_offset;
};

// check to ensure postion of header length in byte stream is the same across
// all message types in variant
template <typename FixMsg1, typename FixMsg2> struct HeaderLenEqual {
  static constexpr bool value =
      FixMsg1::header_length == FixMsg2::header_length;
};

// check to ensure postion of delimiter in byte stream is the same across all
// message types in variant
template <typename FixMsg1, typename FixMsg2> struct DelimOffsetEqual {
  static constexpr bool value =
      FixMsg1::delimiter_offset == FixMsg2::delimiter_offset;
};

// check to ensure value of delimiter in byte stream is the same across all
// message types in variant
template <typename FixMsg1, typename FixMsg2> struct DelimValEqual {
  static constexpr bool value =
      FixMsg1::delimiter_value == FixMsg2::delimiter_value;
};

template <typename FixMsg1, typename FixMsg2> struct VolOffsetEqual {
  static constexpr bool value =
      FixMsg1::volume_offset == FixMsg2::volume_offset;
};

// combines all the checks specified above
template <typename...> struct MsgConsistencyCheck {
  static_assert(false);
};

template <typename FixMsg1, typename FixMsg2>
  requires(!helpers::specializes_class_template_v<MsgConsistencyCheck,
                                                  FixMsg1>) &&
          (!helpers::specializes_class_template_v<MsgConsistencyCheck, FixMsg2>)
struct MsgConsistencyCheck<FixMsg1, FixMsg2> {
  static constexpr bool value = MsgLenNotEqual<FixMsg1, FixMsg2>::value &&
                                LenOffsetEqual<FixMsg1, FixMsg2>::value &&
                                HeaderLenEqual<FixMsg1, FixMsg2>::value &&
                                DelimOffsetEqual<FixMsg1, FixMsg2>::value &&
                                VolOffsetEqual<FixMsg1, FixMsg2>::value &&
                                DelimValEqual<FixMsg1, FixMsg2>::value;
};

template <typename ConsistencyCheck1, typename ConsistencyCheck2>
  requires(helpers::specializes_class_template_v<MsgConsistencyCheck,
                                                 ConsistencyCheck1>) &&
          (helpers::specializes_class_template_v<MsgConsistencyCheck,
                                                 ConsistencyCheck2>)
struct MsgConsistencyCheck<ConsistencyCheck1, ConsistencyCheck2> {
  static constexpr bool value =
      ConsistencyCheck1::value && ConsistencyCheck2::value;
};

// perform consistency check across all types in MsgClassPack
template <typename MsgClassPack>
  requires param_pack::type_pack_convertible_v<MsgClassPack>
constexpr inline bool MsgConsistencyCheck_v = type_pack_check::monoid_check_t<
    MsgConsistencyCheck, 2,
    param_pack::generate_type_pack_t<MsgClassPack>>::value;

// determine longest message length across all message types in variant
template <typename...> struct MaxMsgLenValidator {
  static_assert(false);
};

template <typename FixMsg> struct MaxMsgLenValidator<FixMsg> {
  static constexpr std::uint32_t value = FixMsg::msg_length;
};

template <typename Val1, typename Val2>
  requires helpers::specializes_class_template_v<MaxMsgLenValidator, Val1> &&
           helpers::specializes_class_template_v<MaxMsgLenValidator, Val2>
struct MaxMsgLenValidator<Val1, Val2> {
  static constexpr std::uint32_t value = std::max(Val1::value, Val2::value);
};

template <typename MsgClassPack>
  requires param_pack::type_pack_convertible_v<MsgClassPack>
constexpr inline std::uint32_t determine_max_msg_len =
    type_pack_check::monoid_check_t<
        MaxMsgLenValidator, 1,
        param_pack::generate_type_pack_t<MsgClassPack>>::value;

// determine shortest message length across all message types in variant
template <typename...> struct MinMsgLenValidator {
  static_assert(false);
};

template <typename FixMsg> struct MinMsgLenValidator<FixMsg> {
  static constexpr std::uint32_t value = FixMsg::msg_length;
};

template <typename Val1, typename Val2>
  requires helpers::specializes_class_template_v<MinMsgLenValidator, Val1> &&
           helpers::specializes_class_template_v<MinMsgLenValidator, Val2>
struct MinMsgLenValidator<Val1, Val2> {
  static constexpr std::uint32_t value = std::min(Val1::value, Val2::value);
};

template <typename MsgClassPack>
  requires param_pack::type_pack_convertible_v<MsgClassPack>
constexpr inline std::uint32_t determine_min_msg_len =
    type_pack_check::monoid_check_t<
        MinMsgLenValidator, 1,
        param_pack::generate_type_pack_t<MsgClassPack>>::value;

// ensure all message classes are default constructible
template <typename...> struct DefConstructibleValidator {
  static_assert(false);
};

template <typename T> struct DefConstructibleValidator<T> {
  static constexpr bool value = std::is_default_constructible_v<T>;
};

template <typename Val1, typename Val2>
  requires helpers::specializes_class_template_v<DefConstructibleValidator,
                                                 Val1> &&
           helpers::specializes_class_template_v<DefConstructibleValidator,
                                                 Val2>
struct DefConstructibleValidator<Val1, Val2> {
  static constexpr bool value = Val1::value && Val2::value;
};

template <typename MsgClassPack>
  requires param_pack::type_pack_convertible_v<MsgClassPack>
constexpr inline bool AlternativesDefaultConstructible =
    type_pack_check::monoid_check_t<
        DefConstructibleValidator, 1,
        param_pack::generate_type_pack_t<MsgClassPack>>::value;

}; // namespace MsgTypeChecks

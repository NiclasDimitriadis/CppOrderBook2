#pragma once

#include <cstddef>
#include <utility>

// funtionality to generate a std::index_sequence of all not repeating
// combinations of integers 1 through n of length r
namespace non_rep_combinations {

template <size_t n, size_t r, typename Seq, bool zero_indexed, size_t... Is>
  requires(r > 1) && (n >= r)
struct non_rep_combinations_logic {
  static_assert(false);
};

template <size_t n, size_t r, bool zero_indexed, size_t... Is>
  requires(r > 1) && (n >= r)
struct non_rep_combinations_logic<n, r, std::integer_sequence<size_t, Is...>,
                                  zero_indexed> {

private:
  // computes all values in combination but the final one
  template <size_t cohort_len_, size_t I> struct calc_index {
    // describes value in first column in relation to n, r and cohort length
    static constexpr size_t value =
        I + 2 - zero_indexed + (n - r) - cohort_len_;
  };

  // default case
  template <size_t cohort_len, size_t cohort_len_rem, size_t... comb_Is>
  struct compute {
    using type = compute<cohort_len, cohort_len_rem - 1, comb_Is...,
                         calc_index<cohort_len, Is>::value...,
                         calc_index<cohort_len, r - 1>::value + cohort_len -
                             cohort_len_rem>::type;
  };

  // last combination in cohort
  template <size_t cohort_len, size_t... comb_Is>
  struct compute<cohort_len, 1, comb_Is...> {
    using type =
        compute<cohort_len - 1, cohort_len - 1, comb_Is...,
                calc_index<cohort_len, Is>::value...,
                calc_index<cohort_len, r - 1>::value + cohort_len - 1>::type;
  };

  // final combination
  template <size_t... comb_Is> struct compute<1, 1, comb_Is...> {
    using type =
        std::integer_sequence<size_t, comb_Is..., calc_index<1, Is>::value...,
                              calc_index<1, r - 1>::value>;
  };

public:
  // intialize with length of first cohort
  using type = compute<n - r + 1, n - r + 1>::type;
};

template <size_t n, size_t r, bool zero_indexed>
  requires(r > 0) && (n >= r)
struct non_rep_combinations {
  using type = non_rep_combinations_logic<n, r, std::make_index_sequence<r - 1>,
                                          zero_indexed>::type;
};

// n combinations of length 1 is equivalent to 1 combiantion of size n
template <size_t n, bool zero_indexed>
struct non_rep_combinations<n, 1, zero_indexed> {
  using type = non_rep_combinations<n, n, zero_indexed>::type;
};

template <size_t n, size_t r>
using non_rep_combinations_t = non_rep_combinations<n, r, false>::type;

// computes zero indexed combinations, to be used when computing combinations
// for indices
template <size_t n, size_t r>
using non_rep_index_combinations_t = non_rep_combinations<n, r, true>::type;
}; // namespace non_rep_combinations

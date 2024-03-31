#pragma once

// #include <concepts>
// #include <cstddef>
// #include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Auxil.hpp"

namespace FileToTuples {
template <typename LineTuple, int... Is>
void file_to_tuples_logic(const std::string &file_path,
                          std::vector<LineTuple> &ret_vector,
                          std::integer_sequence<int, Is...>, char delimiter) {
  std::ifstream input_stream(file_path);
  if (!input_stream.is_open()) {
    std::cout << "unable to open file " << file_path << "\n";
    return;
  }
  // generate sequence for indeces of line tuple to use in fold expression
  std::string line;
  std::string buffer;
  while (std::getline(input_stream, line)) {
    std::istringstream sstream(line);
    LineTuple temp_tuple;
    (([&]() {
       std::getline(sstream, buffer, delimiter);
       // insert 0 if csv entry is empty or missing
       // use type of tuple entry to find the matching overload of function
       // template str_to_arith to convert string to respective type
       try {
         std::get<Is>(temp_tuple) =
             Auxil::str_to_arith<std::tuple_element_t<Is, LineTuple>>(buffer);
       } catch (const std::exception &ex) {
         std::get<Is>(temp_tuple) = 0;
       }
     })(),
     ...); // invoke lambda, repeat for every index in Is
    ret_vector.push_back(temp_tuple);
  };
};

// wrapper function that inserts indices of tuple as parameter pack
template <typename LineTuple>
std::vector<LineTuple> file_to_tuples(const std::string &file_path,
                                      char delimiter = ',') {
  static constexpr auto Is =
      std::make_integer_sequence<int, std::tuple_size_v<LineTuple>>();
  std::vector<LineTuple> ret_vector;
  file_to_tuples_logic(file_path, ret_vector, Is, delimiter);
  return ret_vector;
};
} // namespace FileToTuples

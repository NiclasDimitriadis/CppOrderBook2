#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <variant>
#include <vector>

#include "FIXSocketHandler.hpp"
#include "FIXMockSocket.hpp"
#include "FIXMsgClasses.hpp"
#include "FileToTuples.hpp"
#include "OrderBook.hpp"
#include "BusyWait.hpp"

using line_tuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
using MsgClassVar =
    std::variant<FIXMsgClasses::AddLimitOrder,
                 FIXMsgClasses::WithdrawLimitOrder, FIXMsgClasses::MarketOrder>;
static constexpr std::uint32_t two_pow_20 = 1048576;
using OrderBookClass =
    OrderBook::OrderBook<MsgClassVar, std::int64_t, 6000, false>;
using SockHandler =
    FIXSocketHandler::FIXSocketHandler<MsgClassVar,
                                       FIXMockSocket::FIXMockSocket>;


int main(int argc, char* argv[]) {
  // process and validate command line arguments
  std::string csv_path(argv[1]);
  if (argc == 3) {
    int n_cores = std::thread::hardware_concurrency();
    int core_index = std::atoi(argv[2]);
    bool invalid_index = core_index < 0 || core_index > n_cores;
    if (invalid_index) {
      std::cout << "invalid CPU-index argument. teminating."
                << "\n";
      std::terminate();
    }
    // set up affinity of thread
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core_index, &cpu_set);
    int set_aff_ret =
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (set_aff_ret != 0) {
      std::cout << "setting up CPU-affinity failed. teminating."
                << "\n";
      std::terminate();
    }
  }
  // process path argumement, set up objects
  auto tuple_vec = FileToTuples::file_to_tuples<line_tuple>(csv_path);
  auto mock_sock = FIXMockSocket::FIXMockSocket(tuple_vec, nullptr);
  auto s_handler = SockHandler(&mock_sock);
  auto book = OrderBookClass(7000);
  const size_t n_msgs = tuple_vec.size();
  std::optional<MsgClassVar> read_ret;


  std::vector<double> read_times(n_msgs), proc_times(n_msgs), latencies(n_msgs);

  for (size_t i = 0; i < n_msgs; ++i) {
    const auto start_time = std::chrono::high_resolution_clock::now();

    read_ret = s_handler.read_next_message();
    const auto read_done_time = std::chrono::high_resolution_clock::now();
    book.process_order(read_ret.value());
    const auto completion_time = std::chrono::high_resolution_clock::now();

    const double read_time = static_cast<double>(
        duration_cast<std::chrono::nanoseconds>(read_done_time - start_time)
            .count());
    const double proc_time = static_cast<double>(
        duration_cast<std::chrono::nanoseconds>(completion_time - read_done_time)
            .count());
    const double latency = static_cast<double>(
        duration_cast<std::chrono::nanoseconds>(completion_time - start_time)
            .count());
    read_times[i] = read_time;
    proc_times[i] = proc_time;
    latencies[i] = latency;

    BusyWait::busy_wait(1000);
  }

  // use current time as seed for random generator, generate random index
  std::srand(std::time(nullptr));
  int random_price = 7000 + (std::rand() % 6000);
  // query volume at random index to prohibit compiler from optimizing away the
  // program logic
  std::cout << "volume at randomly generated price " << random_price << ": "
            << book.volume_at_price(random_price) << "\n";

  // write vectors of all latencies to csv
  {
    std::ofstream csv("latencies_single_threaded.csv");
    csv << "read_time" << "," << "processing_time" << "," << "total_latency" << "\n";
    for(int i = 0; i < n_msgs; ++i) {
      csv  << read_times[i] << "," << proc_times[i] << "," << latencies[i] << "\n";
    }
  }
}

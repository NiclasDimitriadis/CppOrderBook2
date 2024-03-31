#include <pthread.h>
#include <sched.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <string>
#include <thread>
#include <vector>

#include "BusyWait.hpp"
#include "FIXMockSocket.hpp"
#include "FIXMsgClasses.hpp"
#include "FIXSocketHandler.hpp"
#include "FileToTuples.hpp"
#include "OrderBook.hpp"
#include "Queue.hpp"

using LineTuple = std::tuple<std::uint8_t, std::int32_t, std::uint32_t>;
using MsgClassVar =
    std::variant<FIXMsgClasses::AddLimitOrder,
                 FIXMsgClasses::WithdrawLimitOrder, FIXMsgClasses::MarketOrder>;
static constexpr std::uint32_t two_pow_20 = 1048576;
using OrderBookClass =
    OrderBook::OrderBook<MsgClassVar, std::int64_t, 6000, false>;
using SeqLockQueue = Queue::SeqLockQueue<MsgClassVar, two_pow_20, true, false>;
using SockHandler =
    FIXSocketHandler::FIXSocketHandler<MsgClassVar,
                                       FIXMockSocket::FIXMockSocket>;
using TimePoint = decltype(std::chrono::high_resolution_clock::now());

FIXMockSocket::FIXMockSocket create_mock_socket(const std::string &csv_path,
                                                std::atomic_flag *flag_ptr) {
  auto tuple_vec = FileToTuples::file_to_tuples<LineTuple>(csv_path);
  return FIXMockSocket::FIXMockSocket(tuple_vec, flag_ptr);
}

void enqueue_logic(SeqLockQueue *queue_ptr, const std::string &csv_path,
                   std::atomic_flag *end_of_buffer_flag, int core_index,
                   std::atomic_flag *signal_flag,
                   std::vector<TimePoint> *start_times) noexcept {
  // set up CPU-affinity for enqueueing thread
  if (core_index > -1) {
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(core_index, &cpu_set);
    int set_aff_ret =
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (set_aff_ret != 0) {
      std::cout
          << "setting up CPU-affinity to enqueueing thread failed. teminating."
          << "\n";
      std::terminate();
    }
  }

  // set up objects for reading messages
  auto mock_sock = create_mock_socket(csv_path, end_of_buffer_flag);
  auto s_handler = SockHandler(&mock_sock);
  std::optional<MsgClassVar> read_ret;

  // coordinate with dequeueing thread
  signal_flag->test_and_set();
  while (signal_flag->test()) {
  };

  // read all messages for socket and enqueue them to be processed by other
  // thread
  int index{0};
  TimePoint start_time;

  while (!end_of_buffer_flag->test(std::memory_order_acquire)) {
    start_time = std::chrono::high_resolution_clock::now();
    read_ret = s_handler.read_next_message();
    queue_ptr->enqueue(read_ret.value());
    (*start_times)[index] = start_time;
    ++index;
    BusyWait::busy_wait(1000);
  }
}

int main(int argc, char *argv[]) {
  // process and validate command line arguments
  std::string csv_path(argv[1]);
  // std::string csv_path = "RandTestDataRW.csv";
  int enq_core_index = -1;
  int deq_core_index = -1;
  if (argc == 4) {
    int n_cores = std::thread::hardware_concurrency();
    enq_core_index = std::atoi(argv[2]);
    deq_core_index = std::atoi(argv[3]);
    bool invalid_index = enq_core_index < 0 || enq_core_index > n_cores ||
                         deq_core_index < 0 || deq_core_index > n_cores;
    if (invalid_index) {
      std::cout << "invalid CPU-index argument. teminating."
                << "\n";
      std::terminate();
    }

    // set up affinity for dequeueing thread
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(deq_core_index, &cpu_set);
    int set_aff_ret =
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (set_aff_ret != 0) {
      std::cout
          << "setting up CPU-affinity to dequeueing thread failed. teminating."
          << "\n";
      std::terminate();
    }
  }
  int n_msgs(FileToTuples::file_to_tuples<LineTuple>(csv_path).size());

  // give entire cacheline to end-of-buffer flag to avoid false sharing
  alignas(64) std::atomic_flag end_of_buffer{false};
  std::atomic_flag signal_flag{false};
  std::vector<TimePoint> start_times(n_msgs);

  // set up objects/variables to dequeueing messages and inserting them into
  // order book
  alignas(64) auto book = OrderBookClass(7000);
  SeqLockQueue slq;
  auto reader = slq.get_reader();
  std::vector<TimePoint> completion_times(n_msgs);
  std::optional<MsgClassVar> deq_ret;

  // launch enqueueing thread
  std::thread enq_thread(enqueue_logic, &slq, csv_path, &end_of_buffer,
                         enq_core_index, &signal_flag, &start_times);

  while (!signal_flag.test()) {
  };
  int index{0};
  signal_flag.clear();
  TimePoint completion_time;

  // dequeue messages and insert them into order book
  do {
    deq_ret = reader.read_next_entry();
    if (deq_ret.has_value()) {
      book.process_order(deq_ret.value());
      completion_time = std::chrono::high_resolution_clock::now();
      completion_times[index] = completion_time;
      ++index;
    };
  } while (
      !(end_of_buffer.test(std::memory_order_acquire) && !deq_ret.has_value()));

  enq_thread.join();

  // use current time as seed for random generator, generate random index
  std::srand(std::time(nullptr));
  int random_price = 7000 + (std::rand() % 6000);

  // query volume at random index to prohibit compiler from optimizing away the
  // program logic
  std::cout << "volume at randomly generated price " << random_price << ": "
            << book.volume_at_price(random_price) << "\n";

  // compute latencies
  std::vector<double> latencies(n_msgs);
  for (int i = 0; i < n_msgs; ++i) {
    latencies[i] = static_cast<double>(duration_cast<std::chrono::nanoseconds>(
                                           completion_times[i] - start_times[i])
                                           .count());
  }

  // export latencies to csv
  {
    std::ofstream csv("latencies_multithreaded.csv");
    csv << "latency"
        << "\n";
    for (auto &&elem : latencies) {
      csv << elem << "\n";
    }
  }
}
//

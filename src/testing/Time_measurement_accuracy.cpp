#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>



int main(int argc, char* argv[]) {

  const size_t n_iter = 1'000'000;
  std::vector<double> latencies(n_iter);

  for (int i = 0; i < n_iter; ++i) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    const auto completion_time = std::chrono::high_resolution_clock::now();
    const double latency = static_cast<double>(
        duration_cast<std::chrono::nanoseconds>(completion_time - start_time)
            .count());
    latencies[i] = latency;

  }


  // write vectors of all latencies to csv
  {
    std::ofstream csv("latency_measurements_do_nothing.csv");
    csv << "latency" << "\n";
    for(int i = 0; i < n_iter; ++i) {
      csv << latencies[i] << "\n";
    }
  }
}

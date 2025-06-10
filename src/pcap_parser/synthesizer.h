/**
 * @file synthesizer.h
 * @author dromniscience (you@domain.com)
 * @brief
 * @date 2022-08-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <common/data.h>
#include <iostream>
#include <random>

namespace OmniSketch::Util {

/**
 * @brief Synthesize datasets based on given zipf distribution
 *
 * @tparam key_len  length of flowkey
 */
template <int32_t key_len> class ZipfSynthesizer {
public:
  /**
   * @brief Generate synthesized datasets
   *
   * @param skew        power in zipf distribution
   * @param maximum     maximum flow size
   * @param flow_number #flows
   * @param fmt         format string (specify output format)
   * @param output_file output file
   */
  void Generate(const double skew, const int32_t maximum,
                const int32_t flow_number, const Data::DataFormat &fmt,
                const std::string_view output_file) const {
    // open the output file
    std::ofstream out(std::string(output_file), std::ios::out | std::ios::binary);
    if (!out.is_open()) {
      out.close();
    }

    std::vector<double> cdf(maximum + 1);
    std::vector<size_t> flow_size(flow_number);
    std::set<FlowKey<key_len>> flow_keys;

    // Compute unnormalized CDF
    for (int32_t i = 1; i <= maximum; ++i) {
      cdf[i] = cdf[i - 1] + std::pow(i, -skew);
    }
    std::cout << "Total mass before normalization: " << cdf[maximum]
              << std::endl;
    // Sample the size of each flow
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0, cdf[maximum]);
    std::uniform_int_distribution<> key_distribution(0, 255);

    int64_t total_size = 0;
    for (int32_t i = 0; i < flow_number; ++i) {
      double number = 0.0;
      while (number == 0.0) {
        number = distribution(generator);
      }
      flow_size[i] =
          std::lower_bound(cdf.begin(), cdf.end(), number) - cdf.begin();
      assert(flow_size[i] > 0 && flow_size[i] <= maximum);
      total_size += flow_size[i];
    }
    std::cout << "Average flow size: " << 1.0 * total_size / flow_number
              << std::endl;
    std::cout << "Total: " << total_size << std::endl;

    // Sample randomized flow keys
    for (int32_t i = 0; i < flow_number; ++i) {
      int8_t key[key_len];
      while (true) {
        for (int32_t j = 0; j < key_len; ++j)
          key[j] = key_distribution(generator);
        FlowKey<key_len> flow_key(key);
        if (!flow_keys.count(flow_key)) {
          flow_keys.insert(flow_key);
          break;
        }
      }
    }

    // Dump to the file
    int64_t cnt = 0, time = 0;
    int8_t byte[fmt.getRecordLength()];
    for (const auto &flow_key : flow_keys) {
      for (int32_t i = 0; i < flow_size[cnt]; ++i) {
        Data::Record<key_len> record = {flow_key, time++, 1};
        fmt.writeAsFormat(record, byte);
        out.write(reinterpret_cast<char *>(byte), fmt.getRecordLength());
      }
      cnt++;
    }

    // Close the file
    out.close();
  }
};

} // namespace OmniSketch::Util

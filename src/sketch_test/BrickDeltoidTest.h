/**
 * @file BrickDeltoidTest.h
 * @author hc (you@domain.com)
 * @brief Test deltoid with Brick
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "BrickTest.h"
#include <sketch/BrickDeltoid.h>

#define BRICK_DT_TEST_PATH "Brick.DT.test"
#define BRICK_DT_PARA_PATH "Brick.DT.para"
#define BRICK_DT_DATA_PATH "Brick.DT.data"

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class BrickDeltoidTest : public BrickTestBase<key_len, no_layer, T> {

  Data::HXMethod hx_method;
  double num_heavy_hitter;

public:

  BrickDeltoidTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : BrickTestBase<key_len, no_layer, T>("Brick Deltoid", config_file, BRICK_DT_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test deltoid with ACS
   * @details An overriden method
   */
  void runTest() override;
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickDeltoidTest<key_len, no_layer, T, hash_t>::initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t num_hash, num_group;
  std::string method;
  parser.setWorkingNode(BRICK_DT_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(num_hash, "num_hash"))
    return;
  if (!parser.parseConfig(num_group, "num_group"))
    return;

  parser.setWorkingNode(BRICK_DT_DATA_PATH);
  if (!parser.parseConfig(num_heavy_hitter, "threshold_heavy_hitter"))
    return;
  hx_method = Data::TopK;
  if (!parser.parseConfig(method, "hx_method"))
    return;
  if (!method.compare("Percentile")) {
    hx_method = Data::Percentile;
  }
  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::BrickDeltoid<key_len, no_layer, T, hash_t>>(num_hash, num_group, counter_num, counter);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickDeltoidTest<key_len, no_layer, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth, gnd_truth_heavy_hitters;
  gnd_truth.getGroundTruth(this->data.begin(), this->data.end(), this->cnt_method);
  gnd_truth_heavy_hitters.getHeavyHitter(gnd_truth, num_heavy_hitter, hx_method);

  this->testSize(this->ptr);
  if (hx_method == Data::TopK) {
    this->testHeavyHitter(this->ptr, gnd_truth_heavy_hitters.min(), 
                          gnd_truth_heavy_hitters); // metrics of interest are in config file
  } else {
    this->testHeavyHitter(
        this->ptr, std::floor(gnd_truth.totalValue() * num_heavy_hitter + 1),
        gnd_truth_heavy_hitters); // gnd_truth_heavy_hitter: >, yet HashPipe: >=
  }
  this->show();
  
  return;
}

} // namespace OmniSketch::Test

#undef BRICK_DT_TEST_PATH
#undef BRICK_DT_PARA_PATH
#undef BRICK_DT_DATA_PATH
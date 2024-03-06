/**
 * @file BrickFlowRadarTest.h
 * @author hc (you@domain.com)
 * @brief Test Flow Radar with Brick
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "BrickTest.h"
#include <sketch/BrickFlowRadar.h>

#define BRICK_FR_PARA_PATH "Brick.FlowRadar.para"
#define BRICK_FR_TEST_PATH "Brick.FlowRadar.test"

namespace OmniSketch::Test {

/**
 * @brief Testing class for Flow Radar
 *
 */
template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class BrickFlowRadarTest : public BrickTestBase<key_len, no_layer, T> {
  using TestBase<key_len, T>::config_file;

public:

  BrickFlowRadarTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : BrickTestBase<key_len, no_layer, T>("Brick Flow Radar", config_file, BRICK_FR_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test Flow Radar with ACS
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
void BrickFlowRadarTest<key_len, no_layer, T, hash_t>::initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t flow_filter_bit, flow_filter_hash, count_table_num,
      count_table_hash;  // sketch config
  parser.setWorkingNode(BRICK_FR_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(flow_filter_bit, "flow_filter_bit"))
    return;
  if (!parser.parseConfig(flow_filter_hash, "flow_filter_hash"))
    return;
  if (!parser.parseConfig(count_table_num, "count_table_num"))
    return;
  if (!parser.parseConfig(count_table_hash, "count_table_hash"))
    return;

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::BrickFlowRadar<key_len, no_layer, T, hash_t>>(
        flow_filter_bit, flow_filter_hash, count_table_num, count_table_hash, counter_num, counter);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickFlowRadarTest<key_len, no_layer, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(this->data.begin(), this->data.end(), this->cnt_method);

  this->testSize(this->ptr);
  this->testDecode(this->ptr, gnd_truth);
  this->show();

  return;
}

} // namespace OmniSketch::Test

#undef BRICK_FR_PARA_PATH
#undef BRICK_FR_TEST_PATH


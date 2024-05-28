/**
 * @file LcCMTest.h
 * @author hc (you@domain.com)
 * @brief Test Count Min Sketch with counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "LcTest.h"
#include <sketch/LcCMSketch.h>

#define LC_CM_TEST_PATH "CM.test"
#define LC_CM_PARA_PATH "CM.para"

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class LcCMTest : public LcTestBase<key_len, no_layer, T> {

public:

  LcCMTest(const std::string_view show_name, const std::string_view config_file,
            Data::StreamData<key_len>& data_, Data::CntMethod method)
      : LcTestBase<key_len, no_layer, T>(show_name, config_file, LC_CM_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::LayerCounter<no_layer, T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test CM sketch with Layer Counter
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
void LcCMTest<key_len, no_layer, T, hash_t>::initPtr(int32_t counter_num, Counter::LayerCounter<no_layer, T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t depth, width;
  parser.setWorkingNode(LC_CM_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::LcCMSketch<key_len, no_layer, T, hash_t>>(depth, width, counter_num, counter);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void LcCMTest<key_len, no_layer, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(this->data.begin(), this->data.end(), this->cnt_method);

  /// Insert the samples and then look up all the flows
  ///
  ///        1. query for all the flowkeys
  this->testQuery(this->ptr, gnd_truth); // metrics of interest are in config file
  ///        2. size
  this->testSize(this->ptr);
  ///        3. show metrics
  this->show();

  return;
}

} // namespace OmniSketch::Test

#undef LC_CM_TEST_PATH
#undef LC_CM_PARA_PATH

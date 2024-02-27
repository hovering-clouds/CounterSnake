/**
 * @file BrickCMTest.h
 * @author hc (you@domain.com)
 * @brief Test Count Min Sketch with counter sharing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "BrickTest.h"
#include <sketch/BrickCMSketch.h>

#define BRICK_CM_TEST_PATH "Brick.CM.test"
#define BRICK_CM_PARA_PATH "Brick.CM.para"

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class BrickCMTest : public BrickTestBase<key_len, no_layer, T> {

public:

  BrickCMTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : BrickTestBase<key_len, no_layer, T>("Brick CM Sketch", config_file, BRICK_CM_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test CM sketch with ACS
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
void BrickCMTest<key_len, no_layer, T, hash_t>::initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser){

  /// step i. List Sketch Config
  int32_t depth, width;
  parser.setWorkingNode(BRICK_CM_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(depth, "depth"))
    return;
  if (!parser.parseConfig(width, "width"))
    return;

  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::BrickCMSketch<key_len, no_layer, T, hash_t>>(depth, width, counter_num, counter);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickCMTest<key_len, no_layer, T, hash_t>::runTest() {

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

#undef BRICK_CM_TEST_PATH
#undef BRICK_CM_PARA_PATH

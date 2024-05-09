/**
 * @file BrickPRTest.h
 * @author hc (you@domain.com)
 * @brief Test PRSketch with counter sharing
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "BrickTest.h"
#include <sketch/BrickPRSketch.h>

#define BRICK_PR_TEST_PATH "Brick.PR.test"
#define BRICK_PR_PARA_PATH "Brick.PR.para"

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class BrickPRTest : public BrickTestBase<key_len, no_layer, T> {

public:

  BrickPRTest(const std::string_view config_file, Data::StreamData<key_len>& data_, Data::CntMethod method)
      : BrickTestBase<key_len, no_layer, T>("Brick PR Sketch", config_file, BRICK_PR_TEST_PATH, data_, method) {}

  void initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser) override;

  /**
   * @brief Test CM sketch with Brick
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
void BrickPRTest<key_len, no_layer, T, hash_t>::initPtr(int32_t counter_num, 
      Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser){
  /// step i. List Sketch Config
  int32_t counter_length, counter_hash_num, filter_length, filter_hash_num;   
  T phi;
  parser.setWorkingNode(BRICK_PR_PARA_PATH);

  /// Step ii. Parse
  if (!parser.parseConfig(counter_length, "counter_length"))
    return;
  if (!parser.parseConfig(counter_hash_num, "counter_hash_num"))
    return;
  if (!parser.parseConfig(filter_length, "filter_length"))
    return;
  if (!parser.parseConfig(filter_hash_num, "filter_hash_num"))
    return;
  if (!parser.parseConfig(phi, "phi"))
    return;
  /// Step iii. Prepare Sketch
  /// remember that the left ptr must point to the base class in order to call
  /// the methods in it
  this->ptr = std::make_unique<Sketch::BrickPRSketch<key_len, no_layer, T, hash_t>>(
    counter_length, counter_hash_num, filter_length, filter_hash_num, phi, counter_num, counter);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickPRTest<key_len, no_layer, T, hash_t>::runTest() {

  Data::GndTruth<key_len, T> gnd_truth;
  gnd_truth.getGroundTruth(this->data.begin(), this->data.end(), this->cnt_method);

  /// Insert the samples and then look up all the flows
  ///
  ///        1. query for all the flowkeys
  this->testDecode(this->ptr, gnd_truth); // metrics of interest are in config file
  ///        2. size
  this->testSize(this->ptr);
  ///        3. show metrics
  this->show();

  return;
}

} // namespace OmniSketch::Test

#undef BRICK_PR_TEST_PATH
#undef BRICK_PR_PARA_PATH

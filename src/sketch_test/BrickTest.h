/**
 * @file BrickTestBase.h
 * @author hc (you@domain.com)
 * @brief Brick Test base class
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <common/test.h>
#include <common/Brick.h>

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T>
class BrickTestBase : public TestBase<key_len, T> {

public:
  
  Data::StreamData<key_len>& data;
  Data::CntMethod cnt_method;
  std::unique_ptr<Sketch::SketchBase<key_len, T>> ptr;

  BrickTestBase(const std::string_view show_name, const std::string_view config_file,
              const std::string_view test_path, Data::StreamData<key_len>& data_,
              Data::CntMethod method)
      :TestBase<key_len, T>(show_name, config_file, test_path), data(data_), cnt_method(method) {}

  virtual void initPtr(int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser);
  void doUpdate();
  int32_t getCntNum();

};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

template <int32_t key_len, int32_t no_layer, typename T>
int32_t BrickTestBase<key_len, no_layer, T>::getCntNum(){
  return ptr->cntNum();
}

template <int32_t key_len, int32_t no_layer, typename T>
void BrickTestBase<key_len, no_layer, T>::doUpdate(){
  this->testUpdate(ptr, data.begin(), data.end(), cnt_method);
}

template <int32_t key_len, int32_t no_layer, typename T>
void BrickTestBase<key_len, no_layer, T>::initPtr(
    int32_t counter_num, Counter::Brick<no_layer, T>& counter, Util::ConfigParser& parser){
  LOG(ERROR, "You should override BrickTestBase::initPtr() in subclass.");
  return;
}

} // namespace OmniSketch::Test

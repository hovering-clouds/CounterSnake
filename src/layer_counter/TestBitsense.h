/**
 * @file TestBitsense.h
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "Bitsense.h"
#include <sketch_test/LcTest.h>
#include <sketch_test/LcCMTest.h>
#include <sketch_test/LcHashPipeTest.h>
#include <sketch_test/LcFlowRadarTest.h>
#include <sketch_test/LcDeltoidTest.h>
#include <sketch_test/LcPRTest.h>
#include <sketch_test/LcESTest.h>
#include <sketch_test/LcMVTest.h>
#include <sketch_test/LcSLTest.h>

#define BS_CONFIG_PATH "Lc.bs"

#define KEYLEN 13 // 不同的key_type可以共享在一起，但是受限于实现方法暂时控制住
#define COUNTER_TYPR int32_t // 不同的counter_type不应共享在一起
#define LAYERNUM 4 // 层数需要与config文件中一致

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class TestBitsense {
private:
  using Ptr = std::unique_ptr<LcTestBase<KEYLEN, LAYERNUM, COUNTER_TYPR>>;
  const std::string_view config_file;
  Counter::BitSense<LAYERNUM, COUNTER_TYPR> counter;
  int32_t counter_num;
  std::vector<Ptr> testPtr;

public:
  TestBitsense(const std::string_view config_file_): config_file(config_file_){
    counter_num = 0;
  }
  
  void initPtr(toml::array& sketch_list, Data::StreamData<KEYLEN>& data, 
               Data::CntMethod cnt_method);

  void runTest();
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

void TestBitsense::initPtr(toml::array& sketch_list, 
                             Data::StreamData<KEYLEN>& data, Data::CntMethod cnt_method){
  for(auto& node: sketch_list){
    std::string str = node.as_string()->value_or<std::string>("");
    if(str.compare("CM")==0){ // CM sketch
      testPtr.push_back(std::make_unique<LcCMTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS CMSketch", config_file, data, cnt_method));
    } else if(str.compare("FR")==0){ // FlowRadar
      testPtr.push_back(std::make_unique<LcFlowRadarTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS FlowRadar", config_file, data, cnt_method));
    } else if(str.compare("HP")==0){ //HashPipe
      testPtr.push_back(std::make_unique<LcHashPipeTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS HashPipe", config_file, data, cnt_method));
    } else if(str.compare("DT")==0){ //Deltoid
      testPtr.push_back(std::make_unique<LcDeltoidTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS Deltoid", config_file, data, cnt_method));
    } else if(str.compare("PR")==0){ //PR sketch
      testPtr.push_back(std::make_unique<LcPRTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS PRSketch", config_file, data, cnt_method));
    } else if(str.compare("ES")==0){ //Elastic sketch
      testPtr.push_back(std::make_unique<LcESTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS ElasticSketch", config_file, data, cnt_method));
    } else if(str.compare("MV")==0){ //MV sketch
      testPtr.push_back(std::make_unique<LcMVTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS MVSketch", config_file, data, cnt_method));
    } else if(str.compare("SL")==0){ //SketchLearn
      testPtr.push_back(std::make_unique<LcSLTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BS SketchLearn", config_file, data, cnt_method));
    }
  }
}

void TestBitsense::runTest() {
  srand(20240228);
  /// step i: parse ACS param
  std::string data_file, cmethod;
  toml::array sketch_list, fmt_list;
  std::vector<size_t> width_cnt, no_cnt, no_hash;
  double no_cnt_ratio, aux_width_ratio;
  size_t aux_depth, aux_width;
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  parser.setWorkingNode(BS_CONFIG_PATH);
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(sketch_list, "sketch"))
    return;
  if (!parser.parseConfig(fmt_list, "format"))
    return;
  if (!parser.parseConfig(width_cnt, "width_cnt"))
    return;
  if (!parser.parseConfig(no_hash, "no_hash"))
    return;
  if (!parser.parseConfig(aux_depth, "aux_depth"))
    return;
  if (!parser.parseConfig(aux_width_ratio, "aux_width_ratio"))
    return;
  if (!parser.parseConfig(no_cnt_ratio, "no_cnt_ratio"))
    return;
  // check ratio
  if (no_cnt_ratio <= 0.0 || no_cnt_ratio >= 1.0) {
    throw std::out_of_range("Out of Range: Ratio of #counters of adjacent "
                            "layers in BS should be in (0, 1), but got " +
                            std::to_string(no_cnt_ratio) + " instead.");
  }
  Data::DataFormat format(fmt_list);
  Data::CntMethod cnt_method = Data::InLength;
  if (!parser.parseConfig(cmethod, "cnt_method"))
    return;
  if (!cmethod.compare("InPacket")) {
    cnt_method = Data::InPacket;
  }

  /// Step ii. prepare data
  Data::StreamData<KEYLEN> data(data_file, format); // specify both data file and data format
  if (!data.succeed())
    return;
  // [optional] show data info
  fmt::print("DataSet: {:d} records with xxx keys ({})\n", data.size(), data_file);
  
  /// Step iii. init sketch
  initPtr(sketch_list, data, cnt_method);
  for(auto&& ptr: testPtr){
    ptr->initPtr(counter_num, counter, parser);
    counter_num += ptr->getCntNum();
  }
  // prepare no_cnt
  no_cnt.push_back(counter_num);
  for (int32_t i = 1; i < LAYERNUM; ++i) {
    size_t last_layer = no_cnt.back();
    no_cnt.push_back(Util::NextPrime(std::ceil(last_layer * no_cnt_ratio)));
  }
  aux_width = Util::NextPrime(std::ceil(no_cnt[1]*aux_width_ratio));
  counter.initBs(no_cnt, width_cnt, no_hash, false, true, aux_depth, aux_width);
  for(auto&& ptr: testPtr){
    ptr->doUpdate();
  }
  counter.decode();
  /// Step iv. test sketch
  ///
  ///        1. update records into the sketch
  //this->testUpdate(ptr, data.begin(), data.end(),
  //                 cnt_method); // metrics of interest are in config file
  ///        2. query for all the flowkeys
  for(auto&& ptr: testPtr){
    ptr->runTest();
  }
  std::ofstream outf("csize-bs.txt", std::ios::out);
  counter.dumpCntSize(outf);
  counter.validate();
  for(int32_t lr = 0;lr<LAYERNUM-1;++lr){
    std::cout << "lr" << lr << " overflow: " << counter.getOfNum(lr) << '/' << no_cnt[lr+1] << std::endl;
  }
  std::cout << "mem consumption of tags: "<< counter.tagsize()/1024 << " KB." << std::endl;
  return;
}

} // namespace OmniSketch::Test

#undef BS_CONFIG_PATH

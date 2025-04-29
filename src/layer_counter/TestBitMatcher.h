/**
 * @file TestBitMatcher.h
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "BitMatcher.h"
#include <sketch_test/LcTest.h>
#include <sketch_test/LcCMTest.h>
#include <sketch_test/LcHashPipeTest.h>
#include <sketch_test/LcFlowRadarTest.h>
#include <sketch_test/LcDeltoidTest.h>
#include <sketch_test/LcPRTest.h>
#include <sketch_test/LcESTest.h>
#include <sketch_test/LcMVTest.h>
#include <sketch_test/LcSLTest.h>

#define BM_CONFIG_PATH "Lc.bitmatcher"

#define KEYLEN 13 // 不同的key_type可以共享在一起，但是受限于实现方法暂时控制住
#define COUNTER_TYPR int32_t // 不同的counter_type不应共享在一起
#define LAYERNUM 0 // 层数需要与config文件中一致

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class TestBitMatcher {
private:
  using Ptr = std::unique_ptr<LcTestBase<KEYLEN, LAYERNUM, COUNTER_TYPR>>;
  const std::string_view config_file;
  Counter::BitMatcher<COUNTER_TYPR> counter;
  int32_t counter_num;
  std::vector<Ptr> testPtr;

public:
  TestBitMatcher(const std::string_view config_file_): config_file(config_file_){
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

void TestBitMatcher::initPtr(toml::array& sketch_list, 
                             Data::StreamData<KEYLEN>& data, Data::CntMethod cnt_method){
  for(auto& node: sketch_list){
    std::string str = node.as_string()->value_or<std::string>("");
    if(str.compare("CM")==0){ // CM sketch
      testPtr.push_back(std::make_unique<LcCMTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM CMSketch", config_file, data, cnt_method));
    } else if(str.compare("FR")==0){ // FlowRadar
      testPtr.push_back(std::make_unique<LcFlowRadarTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM FlowRadar", config_file, data, cnt_method));
    } else if(str.compare("HP")==0){ //HashPipe
      testPtr.push_back(std::make_unique<LcHashPipeTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM HashPipe", config_file, data, cnt_method));
    } else if(str.compare("DT")==0){ //Deltoid
      testPtr.push_back(std::make_unique<LcDeltoidTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM Deltoid", config_file, data, cnt_method));
    } else if(str.compare("PR")==0){ //PR sketch
      testPtr.push_back(std::make_unique<LcPRTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM PRSketch", config_file, data, cnt_method));
    } else if(str.compare("ES")==0){ //Elastic sketch
      testPtr.push_back(std::make_unique<LcESTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM ElasticSketch", config_file, data, cnt_method));
    } else if(str.compare("MV")==0){ //MV sketch
      testPtr.push_back(std::make_unique<LcMVTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM MVSketch", config_file, data, cnt_method));
    } else if(str.compare("SL")==0){ //SketchLearn
      testPtr.push_back(std::make_unique<LcSLTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("BM SketchLearn", config_file, data, cnt_method));
    }
  }
}

void TestBitMatcher::runTest() {
  srand(20250429);
  /// step i: parse ACS param
  std::string data_file, cmethod;
  toml::array sketch_list, fmt_list;
  double bkt_rate;
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  parser.setWorkingNode(BM_CONFIG_PATH);
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(sketch_list, "sketch"))
    return;
  if (!parser.parseConfig(fmt_list, "format"))
    return;
  if (!parser.parseConfig(bkt_rate, "bkt_rate"))
    return;
  if(bkt_rate<=0){return;}
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
  size_t bkt_num = counter_num/bkt_rate;
  counter.initBucket(counter_num, bkt_num);
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
  //std::cout << "overflow: " << counter.getOfNum() << std::endl;
  //counter.validate();
  //std::cout << "mem consumption of tags: "<< counter.tagsize()/1024 << " KB." << std::endl;
  //std::cout << counter.bsize() << std::endl;
  //std::ofstream outf("tmpCnt.txt", std::ios::out);
  //std::ofstream outf2("tmpOri.txt", std::ios::out);
  //counter.dumpCnt(outf);
  //counter.dumpOri(outf2);
  return;
}

} // namespace OmniSketch::Test

#undef BM_CONFIG_PATH

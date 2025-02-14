/**
 * @file TestLc.h
 * @author hc (you@domain.com)
 * @brief Test Additive Counter Shaing
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "Dway.h"
#include <sketch_test/LcTest.h>
#include <sketch_test/LcCMTest.h>
#include <sketch_test/LcHashPipeTest.h>
#include <sketch_test/LcFlowRadarTest.h>
#include <sketch_test/LcDeltoidTest.h>
#include <sketch_test/LcPRTest.h>
#include <sketch_test/LcESTest.h>
#include <sketch_test/LcMVTest.h>
#include <sketch_test/LcSLTest.h>

#define LC_CONFIG_PATH "Lc.dway"

#define KEYLEN 13 // 不同的key_type可以共享在一起，但是受限于实现方法暂时控制住
#define COUNTER_TYPR int32_t // 不同的counter_type不应共享在一起
#define LAYERNUM 3 // 层数需要与config文件中一致

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class TestDway {
private:
  using Ptr = std::unique_ptr<LcTestBase<KEYLEN, LAYERNUM, COUNTER_TYPR>>;
  const std::string_view config_file;
  Counter::Dway<LAYERNUM, COUNTER_TYPR> counter;
  int32_t counter_num;
  std::vector<Ptr> testPtr;

public:
  TestDway(const std::string_view config_file_): config_file(config_file_){
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

void TestDway::initPtr(toml::array& sketch_list, 
                             Data::StreamData<KEYLEN>& data, Data::CntMethod cnt_method){
  for(auto& node: sketch_list){
    std::string str = node.as_string()->value_or<std::string>("");
    if(str.compare("CM")==0){ // CM sketch
      testPtr.push_back(std::make_unique<LcCMTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway CMSketch", config_file, data, cnt_method));
    } else if(str.compare("FR")==0){ // FlowRadar
      testPtr.push_back(std::make_unique<LcFlowRadarTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway FlowRadar", config_file, data, cnt_method));
    } else if(str.compare("HP")==0){ //HashPipe
      testPtr.push_back(std::make_unique<LcHashPipeTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway HashPipe", config_file, data, cnt_method));
    } else if(str.compare("DT")==0){ //Deltoid
      testPtr.push_back(std::make_unique<LcDeltoidTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway Deltoid", config_file, data, cnt_method));
    } else if(str.compare("PR")==0){ //PR sketch
      testPtr.push_back(std::make_unique<LcPRTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway PRSketch", config_file, data, cnt_method));
    } else if(str.compare("ES")==0){ //Elastic sketch
      testPtr.push_back(std::make_unique<LcESTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway ElasticSketch", config_file, data, cnt_method));
    } else if(str.compare("MV")==0){ //MV sketch
      testPtr.push_back(std::make_unique<LcMVTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway MVSketch", config_file, data, cnt_method));
    } else if(str.compare("SL")==0){ //SketchLearn
      testPtr.push_back(std::make_unique<LcSLTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>("Dway SketchLearn", config_file, data, cnt_method));
    } //else if(str.compare("CMH")==0){ //CMHeap
    //  testPtr.push_back(std::make_unique<ACSCMHeapTest<KEYLEN, COUNTER_TYPR, Hash::AwareHash>>(config_file, data, cnt_method));
    //}
  }
}

void TestDway::runTest() {
  srand(20240228);
  /// step i: parse ACS param
  size_t group_num, width_total;
  std::string data_file, cmethod;
  toml::array sketch_list, fmt_list;
  std::vector<size_t> dway, width_cnt;
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  parser.setWorkingNode(LC_CONFIG_PATH);
  if (!parser.parseConfig(group_num, "group_num"))
    return;
  if (!parser.parseConfig(data_file, "data"))
    return;
  if (!parser.parseConfig(sketch_list, "sketch"))
    return;
  if (!parser.parseConfig(fmt_list, "format"))
    return;
  if (!parser.parseConfig(dway, "dway"))
    return;
  if (!parser.parseConfig(width_cnt, "width_cnt"))
    return;
  width_total = std::accumulate(width_cnt.begin(),width_cnt.end(),0);
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
  counter.initCounter(counter_num, group_num, dway, width_cnt);
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
    std::cout << "Original Counter Size: " << (width_total*ptr->getCntNum())/(8*1024) << " KB" << std::endl;
  }
  //std::cout << "overflow: " << counter.getOfNum() << std::endl;
  //std::ofstream outf("tmpCnt.txt", std::ios::out);
  //std::ofstream outf2("tmpOri.txt", std::ios::out);
  std::ofstream outf3("tmpFree.txt", std::ios::out);
  std::ofstream outf4("tmpOfIdx.txt", std::ios::out);
  //counter.dumpCnt(outf);
  //counter.dumpOri(outf2);
  counter.dumpFreeCnt(outf3);
  counter.dumpOfIdx(outf4);
  counter.validate();
  std::cout << "mem consumption of counters: "<< counter.bsize()/1024 << " KB." << std::endl;
  std::cout << "mem consumption of tags: "<< counter.tagsize()/1024 << " KB." << std::endl;
  return;
}

} // namespace OmniSketch::Test

#undef LC_CONFIG_PATH

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
#include <sketch_test/LazyResetCMTest.h>
#include <vector>

#define LC_CONFIG_PATH "Lc.dwaylr"

#define KEYLEN 13 // 不同的key_type可以共享在一起，但是受限于实现方法暂时控制住
#define COUNTER_TYPR int32_t // 不同的counter_type不应共享在一起
#define LAYERNUM 4 // 层数需要与config文件中一致

namespace OmniSketch::Test {

/**
 * @brief Testing class for ACS
 *
 */
class TestDwayLazyReset {
private:
  using Ptr = std::unique_ptr<LcTestBase<KEYLEN, LAYERNUM, COUNTER_TYPR>>;
  const std::string_view config_file;
  Counter::Dway<LAYERNUM, COUNTER_TYPR> counter;
  std::vector<short> chunk_tags;
  int32_t counter_num;
  std::vector<Ptr> testPtr;

public:
  TestDwayLazyReset(const std::string_view config_file_): config_file(config_file_){
    counter_num = 0;
  }
  void initPtr(std::string sketch_name, int32_t chunk_size, std::vector<Data::StreamData<KEYLEN>>& data, Data::CntMethod cnt_method);
  void runTest();
};

} // namespace OmniSketch::Test

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Test {

void TestDwayLazyReset::initPtr(std::string sketch_name, int32_t chunk_size,
                std::vector<Data::StreamData<KEYLEN>>& data, Data::CntMethod cnt_method){
  for (size_t i = 0; i < data.size(); ++i) {
    if(sketch_name.compare("CM")==0){ // CM sketch
      testPtr.push_back(std::make_unique<LazyResetCMTest<KEYLEN, LAYERNUM, COUNTER_TYPR, Hash::AwareHash>>(i, chunk_size, "LazyResetDway CMSketch", config_file, data[i], chunk_tags, cnt_method));
    }
  }
}

void TestDwayLazyReset::runTest() {
  srand(20240228);
  /// step i: parse ACS param
  size_t group_num, chunk_size, width_total, sliding_sketch_num, data_window_num;
  std::string sketch_name, data_prefix, data_suffix, cmethod;
  toml::array fmt_list;
  std::vector<size_t> dway, width_cnt;
  Util::ConfigParser parser(config_file);
  if (!parser.succeed()) {
    return;
  }
  parser.setWorkingNode(LC_CONFIG_PATH);
  if (!parser.parseConfig(chunk_size, "chunk_size"))
    return;
  if (!parser.parseConfig(group_num, "group_num"))
    return;
  if (!parser.parseConfig(data_window_num, "data_window_num"))
    return;
  if (!parser.parseConfig(sliding_sketch_num, "sliding_sketch_num"))
    return;
  if (!parser.parseConfig(data_prefix, "data_prefix"))
    return;
  if (!parser.parseConfig(data_suffix, "data_suffix"))
    return;
  if (!parser.parseConfig(sketch_name, "sketch_name"))
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
  std::vector<Data::StreamData<KEYLEN>> data_windows;
  for (size_t window_idx = 0; window_idx < data_window_num; ++window_idx) {
    std::string data_file =
        data_prefix + std::to_string(window_idx) + data_suffix;
    auto data = Data::StreamData<KEYLEN>(data_file, format);
    if (data->succeed())
      return;
    // [optional] show data info
    fmt::print("DataSet-{:d}: {:d} records with xxx keys ({})\n", window_idx, data.size(), data_file);
    data_windows.push_back(std::move(data));
  }
  /// Step iii. init sketch
  initPtr(sketch_name, chunk_size, data_windows, cnt_method);
  if (testPtr.size() == 0) {
    fmt::print("Error: No sketch is initialized.\n");
    return;
  }
  counter_num = testPtr[0]->getCntNum();
  for (size_t i = 0;i<testPtr.size();++i){
    auto&& ptr = testPtr[i];
    int32_t offset = (i%sliding_sketch_num)*counter_num;
    ptr->initPtr(offset, counter, parser);
  }
  chunk_tags.resize(counter_num*sliding_sketch_num/chunk_size);
  std::fill_n(chunk_tags.begin(), chunk_tags.size(), -1);
  counter.initCounter(counter_num*sliding_sketch_num, group_num, dway, width_cnt);
  auto MY_TICK = std::chrono::steady_clock::now();
  for(auto&& ptr: testPtr){
    ptr->doUpdate();
  }
  auto MY_TOCK = std::chrono::steady_clock::now();
  auto MY_TIMER = std::chrono::duration_cast<std::chrono::microseconds>(MY_TOCK - MY_TICK);
  printf("\nUpdate thrpt %lfMops\n", double(counter.update_num)/MY_TIMER.count());
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
  std::ofstream outf5("csize-dway.txt", std::ios::out);
  //counter.dumpCnt(outf);
  //counter.dumpOri(outf2);
  counter.dumpFreeCnt(outf3);
  counter.dumpOfIdx(outf4);
  //counter.dumpCntSize(outf5);
  counter.validate();
  std::cout << "mem consumption of counters: "<< counter.bsize()/1024 << " KB." << std::endl;
  std::cout << "mem consumption of tags: "<< counter.tagsize()/1024 << " KB." << std::endl;
  return;
}

} // namespace OmniSketch::Test

#undef LC_CONFIG_PATH

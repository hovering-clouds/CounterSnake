/**
 * @file test_ACS.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_SAC
#include "test_factory.h"
#include <layer_counter/Sac.h>
using OmniSketch::Counter::SaCounter;
using OmniSketch::Counter::Sac;

void TestSaCounter(){
  SaCounter<uint32_t>::set_len(4);
  SaCounter<uint32_t> bkt;
  bkt.update(4); // no overflow
  VERIFY(bkt.query()==4);
  VERIFY(bkt.findSplit()==0);
  bkt.update(4); // overflow
  VERIFY(bkt.query()==8);
  VERIFY(bkt.findSplit()==1);
  bkt.update(4);
  VERIFY(bkt.query()==12);
  VERIFY(bkt.findSplit()==1);
  bkt.update(4);
  VERIFY(bkt.query()==16);
  VERIFY(bkt.findSplit()==2);
  std::cout << "pass test_sac_counter" << std::endl;
}

void TestSaCounterNeg() {
  SaCounter<int32_t>::set_len(4);
  SaCounter<int32_t> bkt;
  bkt.update(4); // no overflow
  VERIFY(bkt.query()==4);
  VERIFY(bkt.findSplit()==0);
  bkt.update(4); // overflow
  VERIFY(bkt.query()==8);
  VERIFY(bkt.findSplit()==1);
  bkt.update(-6);
  VERIFY(bkt.query()==2);
  VERIFY(bkt.findSplit()==0);
  bkt.update(-4);
  VERIFY(bkt.query()==-2);
  VERIFY(bkt.findSplit()==0);
  bkt.update(-6);
  VERIFY(bkt.query()==-8);
  VERIFY(bkt.findSplit()==1);
  bkt.update(24);
  VERIFY(bkt.query()==16);
  VERIFY(bkt.findSplit()==2);
  std::cout << "pass test_sac_counter_neg" << std::endl;
}

void TestSac(){
    Sac<int32_t> bkt;
    bkt.initBucket(8, 4);
    for(int i = 0;i<8;++i){
      if(i%2)
        bkt.update(i, 2*i);
      else
        bkt.update(i, -2*i);
    }
    for(int i = 0;i<8;++i){
      if(i%2)
        VERIFY(bkt.query(i)==2*i);
      else
        VERIFY(bkt.query(i)==-2*i);
    }
    bkt.decode();
    for(int i = 0;i<8;++i){
      VERIFY(bkt.getOriCnt(i)==bkt.getCnt(i));
    }
    std::cout << "pass test_sac" << std::endl;
}

/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(SAC) {
  TestSaCounter();
  TestSaCounterNeg();
  TestSac();
}
/** @endcond */
#undef TEST_SAC
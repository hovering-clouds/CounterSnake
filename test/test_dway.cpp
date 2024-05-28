/**
 * @file test_dway.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_DWAY
#include "test_factory.h"
#include <common/layer.h>
#include <layer_counter/Dway.h>
using OmniSketch::Counter::DwayCntLayer;
using OmniSketch::Counter::Dway;

void TestDwayCntLayerConstruct() {
  try{
    DwayCntLayer<0, int32_t> bkt({4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    DwayCntLayer<1, int32_t> bkt({4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    DwayCntLayer<2, int32_t> bkt({4,4,4},{2,2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    DwayCntLayer<3, int32_t> bkt({4,4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    DwayCntLayer<3, int32_t> bkt({4,4,4},{10,20,30});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    DwayCntLayer<3, int32_t> bkt({4,4,2},{2,2,5});
  }
  catch(const std::exception& e){
    VERIFY_NO_EXCEPTION(e);
  }
  std::cout << "pass test_DwayCntLayer_construct" << std::endl;
}

void TestDwayCntLayerNormal() {
  DwayCntLayer<3, int32_t> bkt({4,4,4},{2,2,2});
  VERIFY(bkt.updateSegment(0,0,2)==0);
  VERIFY(bkt.updateSegment(0,0,2)==1);
  VERIFY(bkt.updateSegment(0,0,1)==0);
  VERIFY(bkt.getSegment(0,0)==1);
  VERIFY(bkt.updateSegment(1,1,13)==3);
  bkt.resetSegment(1,1);
  VERIFY(bkt.getSegment(1,1)==0);
  bkt.setTag(1,0,2);
  VERIFY(bkt.getTag(1,0)==2);
  VERIFY(bkt.getTag(2,1)==DTAG_INVALID);
  bkt.setTag(1,1,3);
  bkt.setTag(2,2,1);
  VERIFY(bkt.getUnusedNum(0)==0);
  VERIFY(bkt.getUnusedNum(1)==2);
  VERIFY(bkt.getUnusedNum(2)==3);
  bkt.clearAll();
  VERIFY(bkt.getUnusedNum(0)==0);
  VERIFY(bkt.getUnusedNum(1)==4);
  VERIFY(bkt.getUnusedNum(2)==4);
  VERIFY(bkt.getSegment(0,0)==0);
  VERIFY(bkt.bits_num(2)==40);
  std::cout << "pass test_DwayCntLayer_normal" << std::endl;
}

void TestDwayNormal(){
  Dway<3, int32_t> bkt(8, 4, {4,4,4},{2,2,2});
  bkt.pseed = 1; // do not use permutation for convenience
  bkt.iseed = 1;
  bkt.update(2,2);
  bkt.update(2,2);
  bkt.update(1,5);
  bkt.update(0,16);
  VERIFY(bkt.cnt_ptr->getTag(1,0)==2+4);
  VERIFY(bkt.cnt_ptr->getTag(1,1)==1+4);
  VERIFY(bkt.cnt_ptr->getTag(1,2)==0+4);
  VERIFY(bkt.cnt_ptr->getTag(1,3)==DTAG_INVALID);
  VERIFY(bkt.cnt_ptr->getTag(2,0)==2+4);
  VERIFY(bkt.cnt_ptr->getTag(2,1)==DTAG_INVALID);
  VERIFY(bkt.cnt_ptr->getTag(2,2)==DTAG_INVALID);
  VERIFY(bkt.cnt_ptr->getTag(2,3)==DTAG_INVALID);
  VERIFY(bkt.cnt_ptr->getSegment(0,0)==0);
  VERIFY(bkt.cnt_ptr->getSegment(0,1)==1);
  VERIFY(bkt.cnt_ptr->getSegment(0,2)==0);
  VERIFY(bkt.cnt_ptr->getSegment(0,3)==0);
  VERIFY(bkt.cnt_ptr->getSegment(1,0)==1);
  VERIFY(bkt.cnt_ptr->getSegment(1,1)==1);
  VERIFY(bkt.cnt_ptr->getSegment(1,2)==0);
  VERIFY(bkt.cnt_ptr->getSegment(1,3)==0);
  VERIFY(bkt.cnt_ptr->getSegment(2,0)==1);
  VERIFY(bkt.cnt_ptr->getSegment(2,1)==0);
  VERIFY(bkt.cnt_ptr->getSegment(2,2)==0);
  VERIFY(bkt.cnt_ptr->getSegment(2,3)==0);
  VERIFY(bkt.query_with_layer(0).first==16);
  VERIFY(bkt.query_with_layer(0).second==3);
  VERIFY(bkt.query_with_layer(1).first==5);
  VERIFY(bkt.query_with_layer(1).second==2);
  VERIFY(bkt.query_with_layer(2).first==4);
  VERIFY(bkt.query_with_layer(2).second==2);
  VERIFY(bkt.query_with_layer(3).first==0);
  VERIFY(bkt.query_with_layer(3).second==1);
  bkt.clear_cnt(0);
  VERIFY(bkt.cnt_ptr->getSegment(2,0)==0);
  VERIFY(bkt.cnt_ptr->getTag(2,0)==DTAG_INVALID);
  bkt.update(1, 12);
  VERIFY(bkt.cnt_ptr->getSegment(2,0)==1);
  VERIFY(bkt.cnt_ptr->getTag(2,0)==1+4);
  VERIFY(bkt.getOriCnt(0)==0);
  VERIFY(bkt.getOriCnt(1)==17);
  VERIFY(bkt.getOriCnt(2)==4);
  VERIFY(bkt.getOriCnt(3)==0); 
  bkt.decode();
  VERIFY(bkt.getCnt(0)==0); 
  VERIFY(bkt.getCnt(1)==17); 
  VERIFY(bkt.getCnt(2)==4); 
  VERIFY(bkt.getCnt(3)==0);
  std::cout << "pass test_Dway_normal" << std::endl;
}

void TestDway(){
  Dway<3, int32_t> brk(100, 4, {4,4,4},{2,2,2});
  for(size_t i = 0;i<100;++i){
    brk.update(i,rand()%6);
  }
  brk.decode();
  for(size_t i = 0;i<100;++i){
    VERIFY(brk.getCnt(i)==brk.getOriCnt(i));
    VERIFY(brk.query(i)==brk.getOriCnt(i));
  }
  brk[99] = 100;
  VERIFY(brk.getCnt(99)>brk.query(99));
  brk.clear();
  for(size_t i = 0;i<100;++i){
    brk.update(i,2);
  }
  brk.decode();
  for(size_t i = 0;i<100;++i){
    VERIFY(brk.getCnt(i)==2);
    VERIFY(brk.getOriCnt(i)==brk.getCnt(i));
  }
  std::cout << "pass test_Dway" << std::endl;
}

void TestDwayOverflow(){
  Dway<3, int32_t> bkt(12, 4, {0,2,2},{2,2,2});
  bkt.pseed = 1;
  bkt.iseed = 1;
  bkt.update(1,5);// occupy seg (1,0)
  bkt.update(2,6);// occupy seg (1,1)
  bkt.update(3,7);// report overflow
  bkt.update(4,17);// occupy seg (1,2) and (2,0)
  bkt.update(0,17);// report overflow at layer 0
  bkt.update(2,10);// occupy seg (2,1)
  bkt.update(6,9);// occupy seg (1,3)
  bkt.update(6,9);// report overflow at layer 1
  VERIFY(bkt.report_ofl.size()==3);
  // the last two layer1 segments are allocated with two layer2 segments so won't overflow
  bkt.update(9,18);// occupy seg (1,4) and (2,2)
  bkt.update(10,18);// occupy seg (1,5) and (2,3)
  VERIFY(bkt.report_ofl.size()==3);
  VERIFY(bkt.query_with_layer(0).first==1);
  VERIFY(bkt.query_with_layer(0).second==1);
  VERIFY(bkt.query_with_layer(1).first==5);
  VERIFY(bkt.query_with_layer(1).second==2);
  VERIFY(bkt.query_with_layer(2).first==16);
  VERIFY(bkt.query_with_layer(2).second==3);
  VERIFY(bkt.query_with_layer(3).first==3);
  VERIFY(bkt.query_with_layer(3).second==1);
  VERIFY(bkt.query_with_layer(4).first==17);
  VERIFY(bkt.query_with_layer(4).second==3);
  VERIFY(bkt.query_with_layer(6).first==2);
  VERIFY(bkt.query_with_layer(6).second==2);
  VERIFY(bkt.query_with_layer(9).first==18);
  VERIFY(bkt.query_with_layer(9).second==3);
  VERIFY(bkt.query_with_layer(10).first==18);
  VERIFY(bkt.query_with_layer(10).second==3);
  bkt.decode();
  for(size_t i = 0;i<12;++i){
    VERIFY(bkt.getCnt(i)==bkt.getOriCnt(i));
  }
}

void TestDwayCntLayerNegativeException() {
  Dway<3, int32_t> bkt(12, 4, {4,4,4},{2,2,2});
  bkt.update(10,1);
  bkt.update(11,4);
  bkt.update(2,2);
  bkt.update(2,4);
  bkt.update(1,5);
  try{
    bkt.update(10,-2);
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
}

/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(Dway) {
  TestDwayCntLayerConstruct();
  TestDwayCntLayerNormal();
  TestDwayNormal();
  TestDway();
  TestDwayOverflow();
  TestDwayCntLayerNegativeException();
}
/** @endcond */
#undef TEST_BRICK
/**
 * @file test_pyramid.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_PYRAMID
#include "test_factory.h"
#include <layer_counter/Pyramid.h>
using OmniSketch::Counter::Pyramid;

void TestPyramidNormal() {
  Pyramid<3, int32_t> bkt(4,{2,2,2});
  bkt.pseed = 1;
  bkt.iseed = 1;
  bkt.update(2,2);
  bkt.update(2,2);
  bkt.update(1,5);
  bkt.update(0,16);
  VERIFY(bkt.status_bits[0][0]);
  VERIFY(bkt.status_bits[0][1]);
  VERIFY(bkt.status_bits[0][2]);
  VERIFY(!bkt.status_bits[0][3]);
  VERIFY(bkt.status_bits[1][0]);
  VERIFY(bkt.cnt_array[0][0].getVal()==0);
  VERIFY(bkt.cnt_array[0][1].getVal()==1);
  VERIFY(bkt.cnt_array[0][2].getVal()==0);
  VERIFY(bkt.cnt_array[0][3].getVal()==0);
  VERIFY(bkt.cnt_array[1][0].getVal()==1);
  VERIFY(bkt.cnt_array[1][1].getVal()==1);
  VERIFY(bkt.cnt_array[2][0].getVal()==1);
  VERIFY(bkt.query(0)==16);
  VERIFY(bkt.query(1)==17);
  VERIFY(bkt.query(2)==4);
  VERIFY(bkt.query(3)==0);
  VERIFY(bkt.getOriCnt(0)==16);
  VERIFY(bkt.getOriCnt(1)==5);
  VERIFY(bkt.getOriCnt(2)==4);
  VERIFY(bkt.getOriCnt(3)==0); 
  bkt.decode();
  VERIFY(bkt.getCnt(0)==16); 
  VERIFY(bkt.getCnt(1)==17); 
  VERIFY(bkt.getCnt(2)==4); 
  VERIFY(bkt.getCnt(3)==0);
  VERIFY(bkt.size_cnt[0]==5);
  VERIFY(bkt.size_cnt[1]==5);
  VERIFY(bkt.size_cnt[2]==6);
  VERIFY(bkt.size_cnt[3]==3);
  VERIFY(bkt.rsz==0);
  std::cout << "pass test_pyramid_normal" << std::endl;
}

void TestPyramidClearCnt() {
  Pyramid<3, int32_t> bkt(4,{2,2,2});
  bkt.pseed = 1;
  bkt.iseed = 1;
  bkt.update(2,2);
  bkt.update(2,2);
  bkt.update(1,5);
  bkt.update(0,16);
  bkt.clear_cnt(0);
  VERIFY(!bkt.status_bits[0][0]);
  VERIFY(bkt.status_bits[0][1]);
  VERIFY(bkt.status_bits[0][2]);
  VERIFY(!bkt.status_bits[0][3]);
  VERIFY(!bkt.status_bits[1][0]);
  VERIFY(bkt.cnt_array[0][0].getVal()==0);
  VERIFY(bkt.cnt_array[0][1].getVal()==1);
  VERIFY(bkt.cnt_array[0][2].getVal()==0);
  VERIFY(bkt.cnt_array[0][3].getVal()==0);
  VERIFY(bkt.cnt_array[1][0].getVal()==0);
  VERIFY(bkt.cnt_array[1][1].getVal()==1);
  VERIFY(bkt.cnt_array[2][0].getVal()==0);
  VERIFY(bkt.query(0)==0);
  VERIFY(bkt.query(1)==1);
  VERIFY(bkt.query(2)==4);
  VERIFY(bkt.query(3)==0);
  VERIFY(bkt.getOriCnt(0)==0);
  VERIFY(bkt.getOriCnt(1)==5);
  VERIFY(bkt.getOriCnt(2)==4);
  VERIFY(bkt.getOriCnt(3)==0); 
  std::cout << "pass test_pyramid_clear_cnt" << std::endl;
}

void TestPyramidNegativeUpdate() {
  Pyramid<3, int32_t> bkt(8,{2,2,2});
  bkt.pseed = 1;
  bkt.iseed = 1;
  bkt.update(2,2);
  bkt.update(2,2);
  bkt.update(1,5);
  bkt.update(0,16);
  bkt.update(0,-14);
  bkt.update(1,-2);
  VERIFY(bkt.status_bits[0][0]);
  VERIFY(bkt.status_bits[0][1]);
  VERIFY(bkt.status_bits[0][2]);
  VERIFY(!bkt.status_bits[0][3]);
  VERIFY(bkt.status_bits[1][0]);
  VERIFY(bkt.cnt_array[0][0].getVal()==2);
  VERIFY(bkt.cnt_array[0][1].getVal()==3);
  VERIFY(bkt.cnt_array[0][2].getVal()==0);
  VERIFY(bkt.cnt_array[0][3].getVal()==0);
  VERIFY(bkt.cnt_array[1][0].getVal()==0);
  VERIFY(bkt.cnt_array[1][1].getVal()==1);
  VERIFY(bkt.cnt_array[2][0].getVal()==0);
  VERIFY(bkt.query(0)==2);
  VERIFY(bkt.query(1)==3);
  VERIFY(bkt.query(2)==4);
  VERIFY(bkt.query(3)==0);
  VERIFY(bkt.getOriCnt(0)==2);
  VERIFY(bkt.getOriCnt(1)==3);
  VERIFY(bkt.getOriCnt(2)==4);
  VERIFY(bkt.getOriCnt(3)==0); 
  bkt.decode();
  VERIFY(bkt.getCnt(0)==2); 
  VERIFY(bkt.getCnt(1)==3); 
  VERIFY(bkt.getCnt(2)==4); 
  VERIFY(bkt.getCnt(3)==0);
  VERIFY(bkt.size_cnt[0]==5);
  VERIFY(bkt.size_cnt[1]==5);
  VERIFY(bkt.size_cnt[2]==6);
  VERIFY(bkt.size_cnt[3]==3);
  VERIFY(bkt.rsz==8);
  std::cout << "pass test_pyramid_negative" << std::endl;
}


/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(pyramid) {
  TestPyramidNormal();
  TestPyramidClearCnt();
  TestPyramidNegativeUpdate();
}
/** @endcond */
#undef TEST_PYRAMID
/**
 * @file test_diamond.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_DIAMOND
#include "test_factory.h"
#include <layer_counter/Diamond.h>
using OmniSketch::Counter::Diamond;

void TestDiamondNormal() {
  Diamond<3, int32_t> bkt({4,4,4},{2,2,2},4,4,2,2,2);
  bkt.update(0, 4);
  VERIFY(bkt.query(0)==4);
  VERIFY(bkt.query_carry_part(0)==1);
  VERIFY(bkt.query_del_part(0)==0);
  VERIFY(bkt.query_inc_part(0,0)==0);
  VERIFY(bkt.query_inc_part(1,0)==1);
  VERIFY(bkt.getOriCnt(0)==4);
  bkt.update(0, -2);
  VERIFY(bkt.query(0)==2);
  VERIFY(bkt.query_carry_part(0)==1);
  VERIFY(bkt.query_del_part(0)==2);
  VERIFY(bkt.query_inc_part(0,0)==0);
  VERIFY(bkt.query_inc_part(1,0)==1);
  VERIFY(bkt.getOriCnt(0)==2);
  bkt.clear_cnt(0);
  VERIFY(bkt.query(0)==0);
  VERIFY(bkt.query_carry_part(0)==1);
  VERIFY(bkt.query_del_part(0)==4);
  VERIFY(bkt.query_inc_part(0,0)==0);
  VERIFY(bkt.query_inc_part(1,0)==1);
  VERIFY(bkt.getOriCnt(0)==0);
  bkt.decode();
  VERIFY(bkt.getCnt(0)==0); 
  VERIFY(bkt.size_cnt[0]==4);
  VERIFY(bkt.rsz==44);
  std::cout << "pass test_diamond_normal" << std::endl;
}



/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(diamond) {
  TestDiamondNormal();
}
/** @endcond */
#undef TEST_DIAMOND
/**
 * @file test_stingy.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_STINGY
#include "test_factory.h"
#include <layer_counter/Stingy.h>
using OmniSketch::Counter::Stingy;

void TestStingyUtility() {
  Stingy<3, int32_t> bkt(4);
  bkt.pseed = 2;
  bkt.cnt_array[0][0] = 2;
  bkt.cnt_array[0][2] = 3;
  VERIFY(bkt.get_nonempty_child(1,0)==0);
  VERIFY(bkt.get_nonempty_child(1,1)==2);
  VERIFY(bkt.check_parent_empty(0,0));
  VERIFY(bkt.check_sibling_empty(0,0));
  VERIFY(!bkt.check_sibling_empty(0,1));
  bkt.kick_out(0); // kickout to 0,2
  VERIFY(bkt.cnt_array[0][0]==KICK_TAG);
  VERIFY(bkt.cnt_array[0][2]==4); // become the sum
  bkt.cnt_array[0][0] = NULL_VAL;
  bkt.cnt_array[0][1] = 3;
  bkt.cnt_array[1][0] = 2;
  bkt.kick_out(1,0); //  (0,1) to (0,3), (0,2) to (0,0)
  VERIFY(bkt.cnt_array[0][0]==4);
  VERIFY(bkt.cnt_array[1][1]==2);
  VERIFY(bkt.cnt_array[0][3]==3);
  VERIFY(bkt.find_next_pos(1)==3);
  std::cout << "pass test_stingy_utility" << std::endl;
}

void TestStingySetQuery() {
  Stingy<3, int32_t> bkt(8);
  bkt.pseed = 2;
  VERIFY(bkt.set_counter(0, 256)); // (9->2->2) (8+1*62+1*3*62)
  VERIFY(bkt.cnt_array[0][0]==9);
  VERIFY(bkt.cnt_array[1][0]==2);
  VERIFY(bkt.cnt_array[2][0]==2);
  VERIFY(!bkt.set_counter(1, 1)); // fail because parent non-empty
  VERIFY(bkt.set_counter(3,2));
  VERIFY(bkt.cnt_array[0][3]==3);
  VERIFY(!bkt.set_counter(2, 64)); // then fail
  VERIFY(bkt.query(0)==256);
  VERIFY(bkt.query(3)==2);
  VERIFY(bkt.query(5)==0);
  VERIFY(bkt.query(7)==0);
  std::cout << "pass test_stingy_set_query" << std::endl;
}

void TestStingyUpdateQuery() {
  Stingy<3, int32_t> bkt(8);
  bkt.pseed = 2;
  bkt.update(0,256); // (9->2->2)
  bkt.update(1, 1); // kickout to 3
  bkt.update(2, 64); // kick 2 to 4
  bkt.update(3, 61); // kick 3 to 5 then to 7
  bkt.update(5, 1); // (2->2)
  bkt.update(6, 2); // kickout to 0
  VERIFY(bkt.query(0)==258);
  VERIFY(bkt.query(1)==63);
  VERIFY(bkt.query(2)==64);
  VERIFY(bkt.query(3)==63);
  VERIFY(bkt.query(4)==64);
  VERIFY(bkt.query(5)==63);
  VERIFY(bkt.query(6)==258);
  VERIFY(bkt.query(7)==63);
  bkt.clear_cnt(3);
  VERIFY(bkt.getOriCnt(0)==256);
  VERIFY(bkt.getOriCnt(1)==1);
  VERIFY(bkt.getOriCnt(2)==64);
  VERIFY(bkt.getOriCnt(3)==0); 
  VERIFY(bkt.getOriCnt(4)==0); 
  VERIFY(bkt.getOriCnt(5)==1); 
  VERIFY(bkt.getOriCnt(6)==2); 
  VERIFY(bkt.getOriCnt(7)==0); 
  bkt.decode();
  VERIFY(bkt.getCnt(0)==258); 
  VERIFY(bkt.getCnt(1)==0); 
  VERIFY(bkt.getCnt(2)==64); 
  VERIFY(bkt.getCnt(3)==0);
  VERIFY(bkt.getCnt(5)==63);
  VERIFY(bkt.size_cnt[0]==10);
  VERIFY(bkt.size_cnt[1]==6);
  VERIFY(bkt.size_cnt[2]==8);
  VERIFY(bkt.size_cnt[3]==6);
  VERIFY(bkt.size_cnt[4]==8); 
  VERIFY(bkt.size_cnt[5]==8); 
  VERIFY(bkt.size_cnt[6]==10); 
  VERIFY(bkt.size_cnt[7]==8);   
  std::cout << "pass test_stingy_update_query" << std::endl;
}


/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(stingy) {
  TestStingyUtility();
  TestStingySetQuery();
  TestStingyUpdateQuery();
}
/** @endcond */
#undef TEST_STINGY
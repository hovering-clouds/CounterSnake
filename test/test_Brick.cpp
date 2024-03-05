/**
 * @file test_ACS.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_BRICK
#include "test_factory.h"
#include <common/Brick.h>
using OmniSketch::Counter::Bucket;
using OmniSketch::Counter::Brick;

void TestBucketConstruct() {
  try{
    Bucket<0, int32_t> bkt({4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    Bucket<1, int32_t> bkt({4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    Bucket<2, int32_t> bkt({4,4,4},{2,2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    Bucket<3, int32_t> bkt({4,4,4},{2,2});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    Bucket<3, int32_t> bkt({4,4,4},{10,20,30});
    VERIFY(false);
  }
  catch(const std::exception& e){
    VERIFY_EXCEPTION(e);
  }
  try{
    Bucket<3, int32_t> bkt({4,4,2},{2,2,5});
  }
  catch(const std::exception& e){
    VERIFY_NO_EXCEPTION(e);
  }
  std::cout << "pass test_bucket_construct" << std::endl;
}

void TestBucketNormal() {
  Bucket<3, int32_t> bkt({4,4,4},{2,2,2});
  bkt.update(2,2);
  bkt.update(2,2);
  bkt.update(1,5);
  bkt.update(0,16);
  VERIFY(bkt.status_array[1][0]==2);
  VERIFY(bkt.status_array[1][1]==1);
  VERIFY(bkt.status_array[1][2]==0);
  VERIFY(bkt.status_array[1][3]==4);
  VERIFY(bkt.status_array[2][0]==2);
  VERIFY(bkt.status_array[2][1]==4);
  VERIFY(bkt.status_array[2][2]==4);
  VERIFY(bkt.status_array[2][3]==4);
  VERIFY(bkt.cnt_array[0][0].getVal()==0);
  VERIFY(bkt.cnt_array[0][1].getVal()==1);
  VERIFY(bkt.cnt_array[0][2].getVal()==0);
  VERIFY(bkt.cnt_array[0][3].getVal()==0);
  VERIFY(bkt.cnt_array[1][0].getVal()==1);
  VERIFY(bkt.cnt_array[1][1].getVal()==1);
  VERIFY(bkt.cnt_array[1][2].getVal()==0);
  VERIFY(bkt.cnt_array[1][3].getVal()==0);
  VERIFY(bkt.cnt_array[2][0].getVal()==1);
  VERIFY(bkt.cnt_array[2][1].getVal()==0);
  VERIFY(bkt.cnt_array[2][2].getVal()==0);
  VERIFY(bkt.cnt_array[2][3].getVal()==0);
  VERIFY(bkt.query(0)==16);
  VERIFY(bkt.query(1)==5);
  VERIFY(bkt.query(2)==4);
  VERIFY(bkt.query(3)==0);
  VERIFY(bkt.getOriCnt(0)==16);
  VERIFY(bkt.getOriCnt(1)==5);
  VERIFY(bkt.getOriCnt(2)==4);
  VERIFY(bkt.getOriCnt(3)==0); 
  bkt.decode();
  VERIFY(bkt.getCnt(0)==16); 
  VERIFY(bkt.getCnt(1)==5); 
  VERIFY(bkt.getCnt(2)==4); 
  VERIFY(bkt.getCnt(3)==0); 
  std::cout << "pass test_bucket_normal" << std::endl;
}

void TestBucketOverflow() {
  Bucket<3, int32_t> bkt({12,3,1},{2,2,2});
  bkt.update(10,1);
  bkt.update(11,4);
  bkt.update(2,2);
  bkt.update(2,4);
  bkt.update(1,5);
  VERIFY(!bkt.isOverflow());
  VERIFY(bkt.bsize()==4+2);
  bkt.update(0,16);
  VERIFY(bkt.isOverflow());
  VERIFY(bkt.bsize()==4+2+9);
  bkt.update(0,15);
  VERIFY(bkt.query(0)==31);
  VERIFY(bkt.query(1)==5);
  VERIFY(bkt.query(2)==6);
  VERIFY(bkt.query(3)==0);
  VERIFY(bkt.query(7)==0);
  VERIFY(bkt.query(10)==1);
  VERIFY(bkt.query(11)==4);
  bkt.decode();
  VERIFY(bkt.getCnt(0)==31);
  VERIFY(bkt.getCnt(1)==5);
  VERIFY(bkt.getCnt(2)==6);
  VERIFY(bkt.getCnt(3)==0);
  VERIFY(bkt.getCnt(7)==0);
  VERIFY(bkt.getCnt(10)==1);
  VERIFY(bkt.getCnt(11)==4);
  std::cout << "pass test_bucket_overflow" << std::endl;
}

void TestBucketClear() {
  Bucket<4, int32_t> bkt({16,8,4,2},{4,2,2,2});
  for(size_t i = 0;i<16;++i){
    bkt.update(i,3*i);
  }
  VERIFY(bkt.isOverflow());
  for(size_t i = 0;i<16;++i){
    VERIFY(bkt.query(i)==3*i);
  }
  bkt.clear();
  VERIFY(!bkt.isOverflow());
  for(size_t i = 0;i<16;++i){
    bkt.update(i,i);
  }
  VERIFY(!bkt.isOverflow());
  bkt.decode();
  for(size_t i = 0;i<16;++i){
    VERIFY(bkt.getCnt(i)==i);
    VERIFY(bkt.getOriCnt(i)==i);
  }
  std::cout << "pass test_bucket_clear" << std::endl;
}

void TestBrick(){
  Brick<3, int32_t> brk;
  brk.initBucket(100, {8,4,2}, {2,2,2});
  for(size_t i = 0;i<100;++i){
    brk.update(i,rand()%6);
  }
  brk.decode();
  for(size_t i = 0;i<100;++i){
    VERIFY(brk.getCnt(i)==brk.getOriCnt(i));
    VERIFY(brk.query(i)==brk.getOriCnt(i));
    brk[99] = 100;
    VERIFY(brk.getCnt(99)>brk.query(99));
    brk.clear();
  }
  for(size_t i = 0;i<100;++i){
    brk.update(i,2);
  }
  brk.decode();
  for(size_t i = 0;i<100;++i){
    VERIFY(brk.getCnt(i)==2);
    VERIFY(brk.getOriCnt(i)==brk.getCnt(i));
  }
  std::cout << "pass test_brick" << std::endl;
}

void TestBucketStress(){
  int32_t cnt[] = {5, 2, 3, 4, 3, 5, 2, 21, 10, 3, 0, 0, 0, 1, 6, 466,
                   0, 3, 1, 2, 0, 0, 1, 2, 0, 5, 5, 425, 1, 0, 6, 1,
                   0, 0, 0, 2, 0, 0, 0, 5, 0, 5, 5, 29961, 1, 9, 0, 2,
                   0, 10, 0, 1, 6, 0, 0, 8, 0, 6, 32, 4, 8, 1, 0, 0};
  Bucket<3, int32_t> bkt({64,16,4},{8,4,4});
  for (size_t i = 0; i < 64; i++) {
    for (int32_t j = 0;j<cnt[i];++j)
      bkt.update(i, 1);
  }
  bkt.decode();
  for (size_t i = 0; i < 64; i++) {
    VERIFY(bkt.query(i)==cnt[i]);
    VERIFY(bkt.getCnt(i)==cnt[i]);    
  }
  std::cout << "pass test_bucket_stress" << std::endl;
}

void TestBucketSize() {
  Bucket<3, int32_t> bkt({12,3,1},{2,2,2});
  bkt.update(10,1);
  bkt.update(11,4);
  bkt.update(2,2);
  bkt.update(2,4);
  bkt.update(1,5);
  VERIFY(!bkt.isOverflow());
  VERIFY(bkt.bsize()==4+2);
  bkt.decode();
  VERIFY(bkt.csize(0)==2);
  VERIFY(bkt.csize(1)==2+2+4);
  VERIFY(bkt.csize(2)==2+2+4);
  VERIFY(bkt.csize(3)==2);
  VERIFY(bkt.csize(10)==2);
  VERIFY(bkt.csize(11)==2+2+4);
  VERIFY(bkt.rsize()==4);
  size_t csz = bkt.rsize();
  for(size_t i = 0;i<12;++i){
    csz+=bkt.csize(i);
  }
  VERIFY((csz+7)/8==bkt.bsize());
  bkt.update(0,16);
  VERIFY(bkt.isOverflow());
  VERIFY(bkt.bsize()==4+2+9);
  bkt.update(0,15);
  bkt.decode();
  VERIFY(bkt.csize(0)==2+6);
  VERIFY(bkt.csize(1)==2+2+4+6);
  VERIFY(bkt.csize(2)==2+2+4+6);
  VERIFY(bkt.csize(3)==2+6);
  VERIFY(bkt.csize(10)==2+6);
  VERIFY(bkt.csize(11)==2+2+4+6);
}

void TestBucketNegative() {
  Bucket<3, int32_t> bkt({12,3,1},{2,2,2});
  bkt.update(10,1);
  bkt.update(11,4);
  bkt.update(2,2);
  bkt.update(2,4);
  VERIFY(bkt.getFreeCnt(0)==0);
  VERIFY(bkt.getFreeCnt(1)==1);
  VERIFY(bkt.getFreeCnt(2)==1);  
  bkt.update(1,5);
  VERIFY(!bkt.isOverflow());
  bkt.update(2,-4);
  bkt.update(11,-2);
  VERIFY(bkt.getFreeCnt(1)==0);
  VERIFY(bkt.query(0)==0);
  VERIFY(bkt.query(1)==5);
  VERIFY(bkt.query(2)==2);
  VERIFY(bkt.query(7)==0);
  VERIFY(bkt.query(10)==1);
  VERIFY(bkt.query(11)==2);
  VERIFY(!bkt.isOverflow());
  bkt.decode();
  VERIFY(bkt.csize(0)==2);
  VERIFY(bkt.csize(1)==2+2+4);
  VERIFY(bkt.csize(2)==2+2+4);
  VERIFY(bkt.csize(3)==2);
  VERIFY(bkt.csize(10)==2);
  VERIFY(bkt.csize(11)==2+2+4);
  VERIFY(bkt.rsize()==4);
  for(size_t i = 0; i < 12; ++i){
    VERIFY(bkt.getCnt(i)==bkt.getOriCnt(i));
  }
  bkt.update(0, 16);
  bkt.update(0, 15);
  VERIFY(bkt.isOverflow());
  VERIFY(bkt.query(0)==31);
  VERIFY(bkt.query(1)==5);
  VERIFY(bkt.query(2)==2);
  VERIFY(bkt.query(7)==0);
  VERIFY(bkt.query(10)==1);
  VERIFY(bkt.query(11)==2);
}

void TestBucketNegativeException() {
  Bucket<3, int32_t> bkt({12,3,1},{2,2,2});
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
OMNISKETCH_DECLARE_TEST(BRICK) {
  TestBucketConstruct();
  TestBucketNormal();
  TestBucketOverflow();
  TestBucketClear();
  TestBrick();
  TestBucketStress();
  TestBucketSize();
  TestBucketNegative();
  TestBucketNegativeException();
}
/** @endcond */
#undef TEST_BRICK
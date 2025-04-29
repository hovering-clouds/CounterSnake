/**
 * @file test_ACS.cpp
 * @author hc (you@domain.com)
 * @brief Test routines in utils.hpp
 *
 * @copyright Copyright (c) 2023
 *
 */
#define TEST_BITMATCHER
#include "test_factory.h"
#include <layer_counter/BitMatcher.h>
using OmniSketch::Counter::Bucket;
using OmniSketch::Counter::BitMatcher;

void TestBucketBits(){
    Bucket<uint32_t> bkt;
    VERIFY(bkt.calculate_bits(1)==1);
    VERIFY(bkt.calculate_bits(2)==2);
    VERIFY(bkt.calculate_bits(0)==1);
    VERIFY(bkt.calculate_bits(17)==5);
    VERIFY(bkt.calculate_bits(-1)==32);
    Bucket<int32_t> bkt2;
    VERIFY(bkt2.calculate_bits(-1)==1);
    VERIFY(bkt2.calculate_bits(0)==1);
    VERIFY(bkt2.calculate_bits(-2)==2);
    VERIFY(bkt2.calculate_bits(1)==2);
    VERIFY(bkt2.calculate_bits(-4)==3);
    VERIFY(bkt2.calculate_bits(2)==3);
    std::cout << "pass test_bucket_bits" << std::endl;
}

void TestBucketNormal() {
  Bucket<uint32_t> bkt;
  bkt.insertCounter(1, 2); // occupy 2bits
  bkt.insertCounter(2, 3); // occupy 3bits
  bkt.insertCounter(3, 8); // occupy 4bits
  bkt.insertCounter(4, 16); // occupy 5bits
  bkt.insertCounter(5, 32); // occupy 6bits
  VERIFY(!bkt.insertCounter(6, 2)); // cannot insert more
  bkt.updateCounter(3, 9); // overflow to 1
  bkt.updateCounter(4, 6);
  bkt.setCounter(5, 64); // overflow to 0
  VERIFY(!bkt.setCounter(6, 10));
  VERIFY(bkt.is_exist(0));
  VERIFY(bkt.is_exist(1));
  VERIFY(bkt.is_exist(2));
  VERIFY(bkt.is_exist(3));
  VERIFY(bkt.is_exist(4));
  uint32_t result = 0;
  VERIFY(bkt.queryCounter(1, result));
  VERIFY(result==2);
  VERIFY(bkt.queryCounter(2, result));
  VERIFY(result==3);
  VERIFY(bkt.queryWithPos(3, result)==2);
  VERIFY(result==1);
  VERIFY(bkt.queryCounter(4, result));
  VERIFY(result==16+6);
  VERIFY(bkt.queryCounter(5, result));
  VERIFY(result==0);
  bkt.freeCounter(1);
  VERIFY(!bkt.is_exist(1));
  VERIFY(!bkt.queryCounter(2, result));
  VERIFY(bkt.csize(2)==0);
  VERIFY(bkt.csize(1)==2+8);
  VERIFY(bkt.rsize()==3+8+8);
  VERIFY(bkt.getCntLen(4)==6);
  VERIFY(bkt.getCntNum()==5);
  std::cout << "pass test_bucket_normal" << std::endl;
}

void TestBucketNeg(){
    Bucket<int32_t> bkt;
    bkt.insertCounter(1, -1); // occupy 2bits
    bkt.insertCounter(2, -2); // occupy 3bits
    bkt.insertCounter(3, -3); // occupy 4bits
    bkt.insertCounter(4, -4); // occupy 5bits
    bkt.insertCounter(5, -5); // occupy 6bits
    int32_t result;
    VERIFY(bkt.queryCounter(1, result));
    VERIFY(result==-1);
    VERIFY(bkt.queryCounter(2, result));
    VERIFY(result==-2);
    VERIFY(bkt.queryCounter(3, result));
    VERIFY(result==-3);
    VERIFY(bkt.queryCounter(4, result));
    VERIFY(result==-4);
    VERIFY(bkt.queryCounter(5, result));
    VERIFY(result==-5);
    bkt.setCounter(2, -4);
    VERIFY(bkt.queryCounter(2, result));
    VERIFY(result==-4);
    std::cout << "pass test_bucket_negative" << std::endl;
}

void TestBucketTransition(){
    Bucket<uint32_t> bkt1;
    bkt1.insertCounter(1, 2); // occupy 2bits
    bkt1.insertCounter(2, 3); // occupy 3bits
    bkt1.insertCounter(3, 8); // occupy 4bits
    bkt1.insertCounter(4, 16); // occupy 5bits
    bkt1.insertCounter(5, 32); // occupy 6bits
    VERIFY(!bkt1.compress());
    bkt1.setCounter(5, 1);
    VERIFY(!bkt1.compress());
    uint8_t fp1;
    uint32_t val1;
    bkt1.sacrifice(fp1, val1);
    VERIFY(fp1==1 && val1==2);
    VERIFY(bkt1.compress());
    uint32_t result = 0;
    VERIFY(!bkt1.queryCounter(1, result));
    VERIFY(bkt1.queryCounter(2, result));
    VERIFY(result==3);
    VERIFY(bkt1.queryWithPos(3, result)==1);
    VERIFY(result==8);
    VERIFY(bkt1.queryCounter(4, result));
    VERIFY(result==16);
    VERIFY(bkt1.queryCounter(5, result));
    VERIFY(result==1);
    VERIFY(bkt1.getCntLen(0)==4);
    VERIFY(bkt1.getCntLen(1)==5);
    VERIFY(bkt1.getCntLen(2)==6);
    VERIFY(bkt1.getCntLen(3)==13);
    std::cout << "pass test_bucket_transition" << std::endl;
}

void TestBucketTransitionNeg(){
    Bucket<int32_t> bkt2;
    bkt2.insertCounter(1, -1); // occupy 2bits
    bkt2.insertCounter(2, -2); // occupy 3bits
    bkt2.insertCounter(3, -3); // occupy 4bits
    bkt2.insertCounter(4, -4); // occupy 5bits
    bkt2.insertCounter(5, -5); // occupy 6bits
    uint8_t fp2;
    int32_t val2;
    bkt2.sacrifice(fp2, val2);
    VERIFY(fp2==1 && val2==-1);
    VERIFY(bkt2.compress());
    int32_t result2 = 0;
    VERIFY(!bkt2.queryCounter(1, result2));
    VERIFY(bkt2.queryCounter(2, result2));
    VERIFY(result2==-2);
    std::cout << result2 << std::endl;
    VERIFY(bkt2.queryWithPos(3, result2)==1);
    VERIFY(result2==-3);
    std::cout << result2 << std::endl;
    VERIFY(bkt2.queryCounter(4, result2));
    VERIFY(result2==-4);
    VERIFY(bkt2.queryCounter(5, result2));
    VERIFY(result2==-5);
    std::cout << "pass test_bucket_transition_neg" << std::endl;
}

void TestBitMatcher(){
  BitMatcher<uint32_t> bm;
  bm.initBucket(8, 2);
  for(size_t i = 0;i<8;++i){
    for(int j = 0;j<i;++j){
        bm.update(i, 1);
    }
  }
  bm.decode();
  for(size_t i = 0;i<8;++i){
    if(bm.getCnt(i)==bm.getOriCnt(i)){
        VERIFY(bm.query(i)==i);
    }
  }
  bm.clear();

  for(size_t i = 0;i<8;++i){
    bm.update(i,2);
  }
  bm.decode();
  for(size_t i = 0;i<8;++i){
    if(bm.getCnt(i)==2){
        VERIFY(bm.getOriCnt(i)==bm.getCnt(i));
    }
  }
  std::cout << "pass TEST_BITMATCHER" << std::endl;
}

/**
 * @brief other methods in utils
 *
 */
OMNISKETCH_DECLARE_TEST(BM) {
  TestBucketBits();
  TestBucketNormal();
  TestBucketNeg();
  TestBucketTransition();
  TestBitMatcher();
}
/** @endcond */
#undef TEST_BITMATCHER
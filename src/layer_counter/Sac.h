/**
 * @file Sac.h
 * @author hc (you@domain.com)
 * @brief Counter type for Sac (dynamic splitting version)
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#define DEBUG_SAC
#define TEST_DECODE_TIME
#include <common/utils.h>
#include <common/layer.h>
#include <common/hash.h>
#include <type_traits>
#include <random>
#include <numeric>
#include <vector>
#include <iostream>
#include <chrono>


namespace OmniSketch::Counter{

/**
 * @brief Counters used in SAC. Only support non-negative values
 *  
 * @tparam T Counter Type (which should be numerical types)
 */
template <typename T>
class SaCounter{
#ifdef TEST_SAC
public:
#else
private:
#endif
  static int32_t len; // counter length
  static std::default_random_engine engine;
  static std::uniform_int_distribution<uint32_t> udist;
  bool sign_bit; // 1 if negative
  uint32_t counter;
  
  void setBits(uint32_t start, uint32_t end, uint32_t val){
    assert(start<end && start<len && end<=len);
    uint32_t shft_val = val << start;
    uint32_t mask = ((uint32_t(1) << (end-start)) - 1) << start;
    counter &= ~mask; // reset the counter to 0
    shft_val &= mask; // trim to only `end-start` bits
    counter |= shft_val;
  }

  uint32_t getBits(uint32_t start, uint32_t end) const {
    assert(start<end && start<len && end<=len);
    uint32_t mask = (1 << (end-start)) - 1;
    return (counter >> start) & mask;
  }

  void update_add(uint32_t val);
  void update_sub(uint32_t val);
  uint32_t query_abs();
public:
  static int32_t calculate_bits(uint32_t val){
    // for an n-bit unsigned integer, range [0, 2^{n}-1]
    if(val==0){
      return 1;
    } else {
      return floor(log2(val))+1;
    }
  }

  static void set_len(int32_t length){
    len = length;
  }

  SaCounter(){
    sign_bit = 0;
    counter = 0;
  }

  /**
   * @brief Find the position of the split bit 
   * 
   * @return int32_t The index of the split bit from left to right
   */
  int32_t findSplit() const {
    for(int32_t i = len-1;i>=0;--i){
      if((counter & (1<<i))==0){
        return len-1-i;
      }
    }
    assert(false); // should not reach here
  }

  /**
   * @brief Update the inner counter
   * 
   * @param val Value to be updated
   */
  void update(T val);

  /**
   * @brief Query the inner counter
   *
   * @return The value of the counter
   */
  T query();

  /**
   * @brief Get the memory usage of a specific counter, return in bits.
   * 
   * @return 0 if the fingerprint does not exist.
   * 
   */
  static size_t csize() {
    return len;
  }

  /**
   * @brief Clear all the counters
   * 
   */
  void clear(){
    sign_bit = 0;
    counter = 0;
  }
};

/**
 * @brief Sac main structure.
 * 
 * @tparam T Inner counter type (which should be numerical types).
 */
template <typename T>
class Sac : public LayerCounter<0, T> {
  /**
   * @brief Number of counters (some of them may be missed out because of hash collision)
   * 
   */
  size_t cNum;
  /**
   * @brief The length of each counter in bits
   * 
   */
  int32_t cnt_len;
  /**
   * @brief Original counters(ground truth)
   *
   */
  std::vector<T> original_cnt;
  /**
   * @brief Decoded counters(used after decoding)
   * 
   */
  std::vector<T> decoded_cnt;
  /**
   * @brief The counters
   * 
   */
  std::vector<SaCounter<T>> counters;

  Sac(const Sac &) = delete;
  Sac(Sac &&) = delete;

public:
  /**
   * @brief Construct Sac and initialize inner Buckets.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param bucket_num Number of buckets.
   * 
   */
  Sac(size_t counter_num, int32_t counter_len){
    initBucket(counter_num, counter_len);
  }

  /**
   * @brief Construct without initialize Buckets, need to initialize later. 
   * 
   */
  Sac(){}
  
  /**
   * @brief Release Bricks
   * 
   */
  ~Sac(){}

  /**
   * @brief Initialize Buckets
   * 
   * @param counter_num Number of counters you wish to use.
   * @param bucket_num Number of buckets.
   * 
   */
  void initBucket(size_t counter_num, int32_t counter_len);

  /**
   * @brief Update a counter
   * 
   * @param index Counter index
   * @param val Value to be added
   */
  void update(size_t index, T val) override{
    original_cnt[index] += val;
    counters[index].update(val);
  }

  /**
   * @brief Clear counter
   * 
   */
  void clear_cnt(size_t index) override{
    original_cnt[index] = 0;
    counters[index].clear();
  }

  /**
   * @brief Decode all Buckets
   * 
   */
  void decode();
  /**
   * @brief Query a counter online
   * 
   * @param index Counter index
   * @return The counter value
   */
  T query(size_t index) override{
    return counters[index].query();
  }
  /**
   * @brief Get the value of a counter offline. Should be used after decoding
   * 
   * @param index Counter index
   * @return The counter value
   */
  T getCnt(size_t index) const override{
    return decoded_cnt.at(index);
  }

  /**
   * @brief Get the value of the original counter, i.e. the ground truth.
   * 
   */
  T getOriCnt(size_t index) const{
    return original_cnt[index];
  }
  /**
   * @brief Get the reference of a decoded counter. Should be used after decoding
   * 
   * @param index Counter index
   * @return T& The counter reference
   */
  T& operator[](size_t index){
    return decoded_cnt[index];
  }
  /**
   * @brief Get memory consumption of this Sac in bytes
   * 
   * 
   */
  size_t bsize() const{
    return (cnt_len+1)*cNum/8;
  }

  size_t csize(const std::vector<size_t>& idxs) const override{
    return (cnt_len+1)*idxs.size()/8;
  }

  /**
   * @brief Get cNum, the number of counters
   * 
   */
  size_t getcNum() const{
    return cNum;
  }

  /**
   * @brief Dump decoded counters to ostream
   * 
   */
  void dumpCnt(std::ostream& os) const{
    for (size_t i = 0; i < cNum; i++){
      os << decoded_cnt[i] << ' ';
      if(i%100==99){
        os << std::endl;
      }
    }
  }
  /**
   * @brief Dump original counters to ostream
   * 
   */
  void dumpOri(std::ostream& os) const{
    for (size_t i = 0; i < cNum; i++){
      os << original_cnt[i] << ' ';
      if(i%100==99){
        os << std::endl;
      }
    }
  }
  /**
   * @brief Check the consistency between counters and ori_counters
   * 
   */
  void validate() const{
    size_t num = 0;
    size_t err = 0;
    for (size_t i = 0; i < cNum; i++){
      if(getOriCnt(i)!=getCnt(i)){
        num++;
        err += std::abs(getOriCnt(i)-getCnt(i));
      }
    }
    std::cout << "cNum: " << cNum << std::endl;
    std::cout << "#Inconsistency: " << num << ", which may due to clear_cnt" << std::endl;
    std::cout << "Inconsistency ratio: " << (double)num/cNum << std::endl;
    std::cout << "Counter AAE: " << (double)err/cNum << std::endl;
  }
  /**
   * @brief Clear the counters
   * 
   */
  void clear(){
    for (size_t i = 0; i < cNum; i++){
      counters.clear();
    }
    std::fill_n(original_cnt.begin(), cNum, 0);
    std::fill_n(decoded_cnt.begin(), cNum, 0);
  }
};

} //namespace Ominisketch
namespace OmniSketch::Counter {

template <typename T>
int32_t SaCounter<T>::len = 0;
  

template <typename T>
std::default_random_engine SaCounter<T>::engine(20250501);

template <typename T>
std::uniform_int_distribution<uint32_t> SaCounter<T>::udist(0, 65535);

template <typename T>
void SaCounter<T>::update_add(uint32_t val){
  int32_t pos = findSplit();
  if(pos==len-1){ // reach maximum
    return;
  }
  uint32_t stage = 1<<pos;
  uint32_t val_high = val / stage;
  uint32_t val_low = val % stage;
  uint32_t val_rdm = udist(engine) % stage;
  if(val_rdm<val_low){val_high++;}
  uint32_t val_cur = getBits(0, len-pos-1)+val_high;
  uint32_t maxval = 1 << (len-pos-1);
  if(val_cur >= maxval){ // overflow
    uint32_t of_val = (val_cur-maxval) << pos;
    setBits(0, len-pos-1, 0); // clear previous counts
    setBits(len-pos-1, len-pos, 1); // move right split bits
    update_add(of_val);
  } else { // no overflow
    setBits(0, len-pos-1, val_cur);
  }
}

template <typename T>
void SaCounter<T>::update_sub(uint32_t val){
  int32_t pos = findSplit();
  if(pos==len-1){ // reach maximum
    return;
  }
  uint32_t stage = 1<<pos;
  uint32_t val_high = val / stage;
  uint32_t val_low = val % stage;
  uint32_t val_rdm = udist(engine) % stage;
  if(val_rdm<val_low){val_high++;}
  uint32_t val_cur = getBits(0, len-pos-1);
  if(val_high>val_cur){ // underflow
    setBits(0, len-pos-1, 0); // clear previous counts
    if(pos==0){ // change sign
      sign_bit = !sign_bit;
      uint32_t nxt_val = val_high-val_cur;
      update_add(nxt_val);
    } else {
      setBits(len-pos, len-pos+1, 0); // move left split bits
      uint32_t nxt_val = (val_high-val_cur) << pos;
      nxt_val -= 1<<(pos-1); // borrow 1 from previous stage
      setBits(0, len-pos, -1); // set the counter to the maximum value of the previous stage 0xffffffff
      update_sub(nxt_val);
    }
  } else { // no underflow
    setBits(0, len-pos-1, val_cur-val_high);
  }
}

template <typename T>
void SaCounter<T>::update(T val){
  int32_t pos = findSplit();
  bool neg = val<0;
  if(sign_bit == neg){ // same sign
    if constexpr (std::is_unsigned_v<T>){
      update_add(val);
    } else {
      update_add(std::abs(val));
    }
  } else {
    if constexpr (std::is_unsigned_v<T>){
      update_sub(val);
    } else {
      update_sub(std::abs(val));
    }
  }
}

template <typename T>
uint32_t SaCounter<T>::query_abs(){
  int32_t pos = findSplit();
  uint32_t stage = pos*(1<<(len-1));
  uint32_t val = 0;
  if(pos<len-1)
    val = getBits(0, len-pos-1);
  return stage+val*(1<<pos);
}

template <typename T>
T SaCounter<T>::query(){
  if(sign_bit)
    return -query_abs();
  else
    return query_abs();
}


template <typename T>
void Sac<T>::initBucket(size_t counter_num, int32_t counter_len){
  cNum = counter_num;
  cnt_len = counter_len;
  SaCounter<T>::set_len(counter_len);
  counters.resize(cNum);
  std::fill_n(counters.begin(), cNum, SaCounter<T>());
  original_cnt.resize(cNum);
  std::fill_n(original_cnt.begin(), cNum, 0);
  decoded_cnt.resize(cNum);
  std::fill_n(decoded_cnt.begin(), cNum, 0);
}

template <typename T>
void Sac<T>::decode(){
#ifdef TEST_DECODE_TIME
  auto MY_TIMER = std::chrono::microseconds::zero();
  auto MY_TICK = std::chrono::steady_clock::now();
  auto MY_TOCK = std::chrono::steady_clock::now();
#endif
  for (size_t i = 0; i < cNum; i++){
    decoded_cnt[i] = query(i);
  }
#ifdef TEST_DECODE_TIME
  MY_TOCK = std::chrono::steady_clock::now();
  MY_TIMER = std::chrono::duration_cast<std::chrono::microseconds>(MY_TOCK -
                                                                   MY_TICK);
  printf("\nDECODE COST %jdms\n", static_cast<intmax_t>(MY_TIMER.count()));
#endif
}


}// end of namespace Counter

#undef DEBUG_SAC
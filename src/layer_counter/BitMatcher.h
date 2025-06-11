/**
 * @file BitMatcher.h
 * @author hc (you@domain.com)
 * @brief Counter type for BitMatcher
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#define DEBUG_BITMATCHER
#define TEST_DECODE_TIME
#include <common/utils.h>
#include <common/layer.h>
#include <common/hash.h>
#include <type_traits>
#include <climits>
#include <numeric>
#include <vector>
#include <iostream>
#include <chrono>


namespace OmniSketch::Counter{

/**
 * @brief Buckets used in BitMatcher. One Bucket holds 64 bits.
 *  
 * @tparam T Counter Type (which should be numerical types)
 */
template <typename T>
class Bucket{
#ifdef TEST_BITMATCHER
public:
#else
private:
#endif
  uint8_t flag;
  uint8_t exist;
  // contains several counters and their fingerprints, arranged in the order of their sizes 
  uint64_t counters;
  static inline const int counter_num[13] = {5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, -1};
  static inline const int layout[12][6] = {{0, 10, 21, 33, 46, 60}, {0, 11, 23, 36, 60, -1}, {0, 12, 25, 39, 60, -1}, {0, 13, 27, 42, 60, -1},
                                    {0, 12, 25, 60, -1, -1}, {0, 13, 27, 60, -1, -1}, {0, 14, 29, 60, -1, -1}, {0, 15, 31, 60, -1, -1}, 
                                    {0, 16, 33, 60, -1, -1}, {0, 17, 35, 60, -1, -1}, {0, 18, 37, 60, -1, -1}, {0, 19, 39, 60, -1, -1}};
  
  void setBits(uint32_t start, uint32_t end, uint64_t val){
    assert(start<end && start<64 && end<64);
    uint64_t shft_val = val << start;
    uint64_t mask = ((uint64_t(1) << (end-start)) - 1) << start;
    counters &= ~mask; // reset the counter to 0
    shft_val &= mask; // trim to only `end-start` bits
    counters |= shft_val;
  }

  uint64_t getBits(uint32_t start, uint32_t end) const {
    assert(start<end && start<64 && end<64);
    uint64_t mask = (1 << (end-start)) - 1;
    return (counters >> start) & mask;
  }

  T getBitsVal(uint32_t start, uint32_t end) const {
    uint64_t bits = getBits(start, end);
    T val = T(bits);
    int32_t leading_zeros = 8*sizeof(T)-(end-start);
    val <<= leading_zeros;
    val >>= leading_zeros;
    return val;
  }

  bool is_exist(int32_t pos) const {
    return (exist >> pos) & 1;
  }

  void set_exist(int32_t pos){
    exist |= 1 << pos;
  }

  void reset_exist(int32_t pos){
    exist &= ~(1 << pos);
  }

public:
  static int32_t calculate_bits(T val){
    if constexpr (std::is_signed<T>::value){
      // for an n-bit signed integer, range [-2^{n-1}, 2^{n-1}-1]
      // turn a negative value into positive value with the same size
      if (val < 0){
        val = -(val+1); 
      }
      if(val==0){
        return 1;
      } else {
        return floor(log2(val))+2;
      }
    } else {
      // for an n-bit unsigned integer, range [0, 2^{n}-1]
      if(val==0){
        return 1;
      } else {
        return floor(log2(val))+1;
      }
    }
  }

  Bucket(){
    flag = 0;
    exist = 0;
    counters = 0;
  }
  /**
   * @brief Update an inner counter, which should not cause overflow
   * 
   * @note Overflows are handled in the outside class
   *
   * @param fingerprint The fingerprint of the counter (to check if it's in this bucket)
   * @param val Value to be updated
   * 
   * @return If the counter is in this bucket
   */
  bool updateCounter(const uint8_t fingerprint, const T val);

  /**
   * @brief Query an inner counter
   *
   * @param fingerprint The fingerprint of the counter (to check if it's in this bucket)
   * @param result Hold the query result
   * 
   * @return If the counter is in this bucket
   */
  bool queryCounter(const uint8_t fingerprint, T &result);

  /**
   * @brief Query an inner counter with its position
   * 
   * @param fingerprint The fingerprint of the counter (to check if it's in this bucket)
   * @param result Hold the query result
   * 
   * @return The position of this counter. If not exist return -1.
   */
  int32_t queryWithPos(const uint8_t fingerprint, T &result);

  /**
   * @brief Insert a new counter into this bucket.
   * 
   * @return true if successful
   */
  bool insertCounter(const uint8_t fingerprint, T initial_val);
  
  /**
   * @brief Set the counter with `fingerprint` to `val` 
   * 
   * @note Make sure the size of this couter fits in `val`
   * 
   * @return If the counter is in this bucket
   */
  bool setCounter(const uint8_t fingerprint, const T val);
  
  /**
   * @brief Free the counter at the given position
   * 
   */
  void freeCounter(int32_t pos);

  /**
   * @brief State transition type I, compress the largest counter, will rearrange fingerprints and counters
   * 
   * @return If the largest counter can be compressed
   */
  bool compress();

  /**
   * @brief State transition type II, sacrifice the smallest counter, will rearrange fingerprints and counters
   * 
   * @return If the smallest counter exists (non-empty)
   */
  bool sacrifice(uint8_t &fingerprint, T &value);

  /**
   * @brief Get the numbre of counters in this bucket
   * 
   */
  int32_t getCntNum() const{
    return counter_num[flag];
  }

  int32_t getCntLen(int32_t pos) const{
    return layout[flag][pos+1]-layout[flag][pos]-8;
  }

  bool isMaxCnt(int32_t pos){
    return pos==getCntNum()-1;
  }

  bool isMinCnt(int32_t pos){
    return pos==0;
  }

  bool isCompressLimit(){
    return counter_num[flag+1]<counter_num[flag];
  }

  bool isSacrificeLimit(){
    return flag>3;
  }
  /**
   * @brief Get the memory usage of this bucket in bytes.
   * 
   */
  static size_t bsize(){
    return (64+8)/8;
  }

  /**
   * @brief Get the memory usage of a specific counter, return in bits.
   * 
   * @return 0 if the fingerprint does not exist.
   * 
   */
  size_t csize(const uint8_t fingerprint) const;

  /**
   * @brief Get the total sizes of empty counters in bits. (counters and fingerprints)
   * 
   */
  size_t rsize() const;

  size_t empty_cnt_bits() const;

  size_t empty_num() const;

  void dump(std::ostream& os) const {
    int32_t cnt_num = getCntNum();
    os << "exist: ";
    for(int i = 0;i<cnt_num;++i){
      os << is_exist(i) << ' ';
    }
    os << std::endl << "len: ";
    for(int i = 0;i<cnt_num;++i){
      os << getCntLen(i) << ' ';
    }
    os << std::endl << "fp: ";
    for(int i = 0;i<cnt_num;++i){
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      int fp = getBits(start, start+8);
      os << fp << ' ';
    }
    os << std::endl << "val: ";
    for(int i = 0;i<cnt_num;++i){
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      T val = getBitsVal(start+8, end);
      os << val << ' ';
    }
    os << std::endl;
    return;
  }
  /**
   * @brief Clear all the counters
   * 
   */
  void clear(){
    flag = 0;
    exist = 0;
    counters = 0;
  }
};

/**
 * @brief Bitmatcher main structure.
 * 
 * @tparam T Inner counter type (which should be numerical types).
 */
template <typename T>
class BitMatcher : public LayerCounter<0, T> {
  /**
   * @brief Number of counters (some of them may be missed out because of hash collision)
   * 
   */
  size_t cNum;
  /**
   * @brief Number of Buckets, should be one of 2's powers
   * 
   */
  size_t bNum;
  /**
   * @brief bNum = 2^{bPow}
   * 
   */
  size_t bPow;
  /**
   * @brief Size of empty counters (because loading rate < 1)
   * 
   */
  size_t rsz;
  /**
   * @brief Seed used to compute fingerprint. The formula is: fingerprint = (index*pseed)%256.
   * 
   */
  size_t pseed;
  /**
   * @brief Fail to kick out or insert new items.
   * 
   */
  size_t failure_num;
  /**
   * @brief Two hash functions used to select bucket
   * 
   */
  Hash::AwareHash hash_fn;
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
   * @brief Bits of each counter
   * 
   */
  std::vector<size_t> size_cnt;
  /**
   * @brief Buckets used to store the counters
   * 
   */
  std::vector<Bucket<T>> buckets;

  BitMatcher(const BitMatcher &) = delete;
  BitMatcher(BitMatcher &&) = delete;

  bool sacrifice_smallest(size_t bktId);
  void kickout(size_t bktId, uint8_t fp);
  void update_exist(size_t bktId, int32_t pos, uint8_t fp, T new_val);
public:
  size_t update_num;
  size_t update_mem_access;
  size_t max_update_mem_access;
  size_t query_mem_access;
  size_t max_query_mem_access;
  /**
   * @brief Construct BitMatcher and initialize inner Buckets.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param bucket_num Number of buckets.
   * 
   */
  BitMatcher(size_t counter_num, size_t bucket_num){
    initBucket(counter_num, bucket_num);
  }

  /**
   * @brief Construct without initialize Buckets, need to initialize later. 
   * 
   */
  BitMatcher(){}
  
  /**
   * @brief Release Bricks
   * 
   */
  ~BitMatcher(){}

  /**
   * @brief Initialize Buckets
   * 
   * @param counter_num Number of counters you wish to use.
   * @param bucket_num Number of buckets.
   * 
   */
  void initBucket(size_t counter_num, size_t bucket_num);

  /**
   * @brief Update a counter
   * 
   * @param index Counter index
   * @param val Value to be added
   */
  void update(size_t index, T val) override;

  /**
   * @brief Clear counter
   * 
   */
  void clear_cnt(size_t index) override{
    original_cnt[index] = 0;
    size_t bktIdx1 = hash_fn(index)%bNum;
    uint8_t fp = (index*pseed)%256;
    size_t bktIdx2 = (bktIdx1^fp)%bNum;
    buckets.at(bktIdx1).setCounter(fp, 0);
    buckets.at(bktIdx1).setCounter(fp, 0);
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
    size_t bktIdx1 = hash_fn(index)%bNum;
    uint8_t fp = (index*pseed)%256;
    size_t bktIdx2 = (bktIdx1^fp)%bNum;
    T result;
    if(buckets.at(bktIdx1).queryCounter(fp, result)){
      return result;
    } else if(buckets.at(bktIdx2).queryCounter(fp, result)) {
      return result;
    }
    return 0;
  }
  /**
   * @brief Query a counter with its size
   * 
   * @param index Counter index
   * @param len Counter size
   * @return The counter value
   */
  T queryWithSize(size_t index, size_t &len){
    size_t tmp_mem_access = 1;
    size_t bktIdx1 = hash_fn(index)%bNum;
    uint8_t fp = (index*pseed)%256;
    size_t bktIdx2 = (bktIdx1^fp)%bNum;
    Bucket<T> &bkt1 = buckets.at(bktIdx1);
    Bucket<T> &bkt2 = buckets.at(bktIdx2);
    T result;
    int32_t pos1 = bkt1.queryWithPos(fp, result);
    if(pos1!=-1){
      len = bkt1.getCntLen(pos1);
      query_mem_access += tmp_mem_access;
      max_query_mem_access = std::max(max_query_mem_access, tmp_mem_access);
      return result;
    } else {
      tmp_mem_access += 1;
      query_mem_access += tmp_mem_access;
      max_query_mem_access = std::max(max_query_mem_access, tmp_mem_access);
      int32_t pos2 = bkt2.queryWithPos(fp, result);
      if(pos2!=-1){
        len = bkt2.getCntLen(pos2);
        return result;
      } else {
        len = 0;
        return 0;    
      }
    }
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
   * @brief Get memory consumption of this BitMatcher in bytes
   * 
   * 
   */
  size_t bsize() const;

  size_t rsize() const{
    return rsz;
  }
  /**
   * @brief Get the memory usage of the given counters, return in bits
   * 
   */
  size_t csize(const std::vector<size_t>& idxs) const override;

  /**
   * @brief Get cNum, the number of counters
   * 
   */
  size_t getcNum() const{
    return cNum;
  }

  size_t getFailureNum() const{
    return failure_num;
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
   * @brief Dump the actual counter size and its ideal size
   * 
   */
  void dumpCntSize(std::ostream& os) const;
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
    /*std::vector<int32_t> hash_cnt(bNum, 0);
      for (size_t i = 0;i<cNum;++i){
        hash_cnt[hash_fn(i)%bNum]++;
      }
      for (size_t i = 0;i<bNum;++i){
        std::cout << hash_cnt[i] << " ";
        if(i%100==99){
          std::cout << std::endl;
      }
    }*/
    std::cout << "cNum: " << cNum << " bNum: " << bNum << std::endl;
    std::cout << "failure num: " << failure_num << std::endl;
    std::cout << "#Inconsistency: " << num << ", which may due to clear_cnt" << std::endl;
    std::cout << "Inconsistency ratio: " << (double)num/cNum << std::endl;
    std::cout << "Counter AAE: " << (double)err/cNum << std::endl;
    size_t fp_num = 0, empty_size = 0;
    for(size_t i = 0;i<bNum;++i){
      fp_num+=buckets[i].getCntNum();
      empty_size += buckets[i].empty_cnt_bits();
    }
    std::cout << "Tag size: " << (bNum*4/8)+fp_num*8/8 << " ,"  << "empty counter size: " << empty_size/8 << std::endl;
  }
  /**
   * @brief Clear the counters
   * 
   */
  void clear(){
    for (size_t i = 0; i < bNum; i++){
      buckets[i].clear();
    }
    rsz = 0;
    update_num = 0;
    update_mem_access = 0;
    query_mem_access = 0;
    max_update_mem_access = 0;
    max_query_mem_access = 0;
    failure_num = 0;
    std::fill_n(original_cnt.begin(), cNum, 0);
    std::fill_n(decoded_cnt.begin(), cNum, 0);
    std::fill_n(size_cnt.begin(), cNum, 0);  
  }
};

} //namespace Ominisketch
namespace OmniSketch::Counter {

template <typename T>
bool Bucket<T>::updateCounter(const uint8_t fingerprint, const T val){
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      continue;
    }
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    uint8_t fp = getBits(start, start+8);
    if(fingerprint==fp){
      T old_val = getBitsVal(start+8, end);
      T new_val = old_val + val;
      setBits(start+8, end, new_val);
      return true;
    }
  }
  return false;
}

template <typename T>
bool Bucket<T>::queryCounter(const uint8_t fingerprint, T &result){
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      continue;
    }
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    uint8_t fp = getBits(start, start+8);
    if(fingerprint==fp){
      result = getBitsVal(start+8, end);
      return true;
    }
  }
  return false;
}

template <typename T>
int32_t Bucket<T>::queryWithPos(const uint8_t fingerprint, T &result){
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      continue;
    }
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    uint8_t fp = getBits(start, start+8);
    if(fingerprint==fp){
      result = getBitsVal(start+8, end);
      return i;
    }
  }
  return -1;
}

template <typename T>
bool Bucket<T>::insertCounter(const uint8_t fingerprint, T initial_val){
  int bits = calculate_bits(initial_val);
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    if(is_exist(i)||(end-start-8)<bits){
      continue;
    }
    // find one empty counter that has enough bits
    set_exist(i);
    setBits(start, start+8, fingerprint);
    setBits(start+8, end, initial_val);
    return true;
  }
  return false;
}

template <typename T>
bool Bucket<T>::setCounter(const uint8_t fingerprint, const T val){
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      continue;
    }
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    uint8_t fp = getBits(start, start+8);
    if(fingerprint==fp){
      setBits(start+8, end, val);
      return true;
    }
  }
  return false;
}

template <typename T>
void Bucket<T>::freeCounter(int32_t pos){
  reset_exist(pos);
  int start = layout[flag][pos];
  int end = layout[flag][pos+1];
  setBits(start, end, 0);
}

template <typename T>
size_t Bucket<T>::csize(const uint8_t fingerprint) const{
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      continue;
    }
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    uint8_t fp = getBits(start, start+8);
    if(fingerprint==fp){
      return end-start;
    }
  }
  return 0;
}

template <typename T>
size_t Bucket<T>::rsize() const{
  size_t rsz = 8; // flag & empty
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      rsz += end-start; 
    }
  }
  return rsz;
}

template <typename T>
size_t Bucket<T>::empty_cnt_bits() const{
  size_t rsz = 0; // flag & empty
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      rsz += end-start-8; 
    }
  }
  return rsz;
}

template <typename T>
size_t Bucket<T>::empty_num() const{
  size_t num = 0; // flag & empty
  int cnt_num = counter_num[flag];
  for(int i = 0;i<cnt_num;++i){
    if(!is_exist(i)){
      num++;
    }
  }
  return num;
}

template <typename T>
bool Bucket<T>::compress(){
  if(counter_num[flag+1]<counter_num[flag]){
    return false; //maximum
  }
  int cnt_num = counter_num[flag];
  int start_l = layout[flag][cnt_num-1];
  int end_l = layout[flag][cnt_num];
  T largest_cnt = getBitsVal(start_l+8, end_l);
  int bits = end_l-start_l-8-calculate_bits(largest_cnt);
  if(bits>=cnt_num-1){ // can be compressed
    std::vector<uint8_t> fps;
    std::vector<T> vals;
    for(int i = 0;i<cnt_num;++i){ // record old values
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      fps.push_back(getBits(start, start+8));
      vals.push_back(getBitsVal(start+8, end));
    }
    flag++;
    for(int i = 0;i<cnt_num;++i){ // rearrange the recorded values
      int start = layout[flag][i];
      int end = layout[flag][i+1];
      setBits(start, start+8, fps[i]);
      setBits(start+8, end, vals[i]);
    }
    return true;
  } else {
    return false;
  }
}

template <typename T>
bool Bucket<T>::sacrifice(uint8_t &fingerprint, T &value){
  bool exist_smallest = is_exist(0);
  exist >>= 1; // delete the smallest counter
  int cnt_num = counter_num[flag];
  std::vector<uint8_t> fps;
  std::vector<T> vals;
  for(int i = 0;i<cnt_num;++i){ // record old values
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    fps.push_back(getBits(start, start+8));
    vals.push_back(getBitsVal(start+8, end));
  }
  if(flag==0){
    flag++;
  } else if (flag<4){
    flag+=3;
  } else { // should not reach here
    assert(false);
  }
  cnt_num--;
  for(int i = 0;i<cnt_num;++i){ // rearrange the recorded values
    int start = layout[flag][i];
    int end = layout[flag][i+1];
    setBits(start, start+8, fps[i+1]);
    setBits(start+8, end, vals[i+1]);
  }
  fingerprint = fps[0];
  value = vals[0];
  return exist_smallest;
}



template <typename T>
void BitMatcher<T>::initBucket(size_t counter_num, size_t bucket_num){
  cNum = counter_num;
  bPow = Util::Next2Pow(bucket_num);
  bNum = 1<<bPow;
  rsz = 0;
  update_num = 0;
  update_mem_access = 0;
  query_mem_access = 0;
  max_update_mem_access = 0;
  max_query_mem_access = 0;
  failure_num = 0;
  hash_fn = Hash::AwareHash();
  int32_t candidate = 31;
  int32_t cNum32 = static_cast<int32_t>(cNum);
  while(true){
    if(Util::IsCoprime(candidate, cNum32)){
      pseed = static_cast<size_t>(candidate);
      break;
    }else{candidate++;}
  }
  for(size_t i = 0;i<bNum;++i){
    buckets.push_back(Bucket<T>());
  }
  original_cnt.resize(cNum);
  std::fill_n(original_cnt.begin(), cNum, 0);
  decoded_cnt.resize(cNum);
  std::fill_n(decoded_cnt.begin(), cNum, 0);
  size_cnt.resize(cNum);
  std::fill_n(size_cnt.begin(), cNum, 0);
}

template <typename T>
bool BitMatcher<T>::sacrifice_smallest(size_t bktId){
  Bucket<T> &bkt = buckets.at(bktId);
  uint8_t fp;
  T val;
  if(bkt.isSacrificeLimit()){return false;}
  bool kick_out = bkt.sacrifice(fp, val);
  if(kick_out){
    Bucket<T> &bkt_other = buckets.at((bktId^fp)%bNum);
    bool suc = bkt_other.insertCounter(fp, val);
    if(!suc){failure_num++;}
  }
  return true;
}

template <typename T>
void BitMatcher<T>::kickout(size_t bktId, uint8_t fp){
  Bucket<T> &bkt = buckets.at(bktId);
  Bucket<T> &bkt_other = buckets.at((bktId^fp)%bNum);
  T val;
  int32_t pos = bkt.queryWithPos(fp, val);
  if(pos!=-1){
    bkt.freeCounter(pos);
    bool suc = bkt_other.insertCounter(fp, val);
    if(!suc){failure_num++;}
  }
}

template <typename T>
void BitMatcher<T>::update_exist(size_t bktId, int32_t pos, uint8_t fp, T new_val){
  Bucket<T> &bkt = buckets.at(bktId);
  int32_t len = bkt.getCntLen(pos);
  int32_t bits = Bucket<T>::calculate_bits(new_val);
  if(bits>len){ // overflow
    bool suc = bkt.insertCounter(fp, new_val); 
    if(suc){ // case1: swap this counter into a larger one in this bucket
      bkt.freeCounter(pos);
    } else if(bkt.isMaxCnt(pos)){ // case2: counter_max overflow, then sacrifice
      bool can_sacr = sacrifice_smallest(bktId);
      if(!can_sacr){failure_num++; return;}
      update_exist(bktId, pos-1, fp, new_val); // recursively update, in case one sacrifice not enough
      // bkt.setCounter(fp, new_val);
    } else if(bkt.isCompressLimit()){ // case3: Y maximum
      if(bkt.isMinCnt(pos)){ // case3-1: the smallest counter overflows, kick out
        kickout(bktId, fp);
      } else { // case3-2: other counters overflow, sacrifice smallest counter and then compress
        bool can_sacr = sacrifice_smallest(bktId);
        if(!can_sacr){failure_num++; return;}
        bkt.compress(); // must succeed
        update_exist(bktId, pos-1, fp, new_val);
        // bkt.setCounter(fp, new_val);
      }
    } else { // case4: Y not maximum
      bool suc = bkt.compress();
      if(suc){ // case4-1: compressible
        update_exist(bktId, pos, fp, new_val);
        // bkt.setCounter(fp, new_val);
      } else { // case4-2: incompressible
        kickout(bktId, fp);
      }
    }
  } else { // no overflow
    bkt.setCounter(fp, new_val);
  }
}

template <typename T>
void BitMatcher<T>::update(size_t index, T val){
  update_num+=1;
  size_t tmp_mem_access = 0;
  original_cnt[index] += val;
  uint8_t fp = (index*pseed)%256;
  uint64_t hashval = hash_fn(index);
  Bucket<T> &bkt1 = buckets.at(hashval%bNum);
  Bucket<T> &bkt2 = buckets.at((hashval^fp)%bNum);
  T result1, result2;
  tmp_mem_access +=1;
  int32_t pos1 = bkt1.queryWithPos(fp, result1);
  int32_t pos2 = bkt2.queryWithPos(fp, result2);
  if(pos1!=-1){
    update_exist(hashval%bNum, pos1, fp, result1+val);
  } else if(pos2!=-1) {
    tmp_mem_access +=1;
    update_exist((hashval^fp)%bNum, pos2, fp, result2+val);
  } else { // new item
    bool suc1 = bkt1.insertCounter(fp, val);
    if(!suc1){
      bool suc2 = bkt2.insertCounter(fp, val);
      if(!suc2){failure_num++;}
    }
  }
  update_mem_access += tmp_mem_access;
  max_update_mem_access = std::max(max_update_mem_access, tmp_mem_access);
}

template <typename T>
void BitMatcher<T>::decode(){
  query_mem_access = 0;
  max_query_mem_access = 0;
#ifdef TEST_DECODE_TIME
  auto MY_TIMER = std::chrono::microseconds::zero();
  auto MY_TICK = std::chrono::steady_clock::now();
  auto MY_TOCK = std::chrono::steady_clock::now();
#endif
  rsz = 0;
  int num = 0;
  for (size_t i = 0; i < cNum; i++){
    decoded_cnt[i] = queryWithSize(i, size_cnt[i]);
    if(size_cnt[i]==0){
      num++;
    } else {
      size_cnt[i]+=8;
    }
  }
#ifdef TEST_DECODE_TIME
  MY_TOCK = std::chrono::steady_clock::now();
  MY_TIMER = std::chrono::duration_cast<std::chrono::microseconds>(MY_TOCK -
                                                                   MY_TICK);
  printf("\nDECODE COST %jdms\n", static_cast<intmax_t>(MY_TIMER.count()));
  printf("\nQuery thrpt %lfMops\n", double(cNum)/MY_TIMER.count());
#endif
  std::cout << "un-inserted counters: " << num << " ";
  num = 0;
  for (size_t i = 0; i < bNum; i++){
    rsz+=buckets[i].rsize();
    num+=buckets[i].empty_num();
  }
  std::cout << "empty slots: " << num << std::endl;
  std::cout <<"update_access: average " << 1.0*update_mem_access/update_num << ", max " << max_update_mem_access << std::endl;
  std::cout <<"query_access: average " << 1.0*query_mem_access/cNum << ", max " << max_query_mem_access << std::endl;
}

template <typename T>
size_t BitMatcher<T>::bsize() const{
  size_t bytes = 0;
  for (size_t i = 0; i < bNum; i++){
    bytes+=buckets[i].bsize();
  }
  return bytes;
}

template <typename T>
size_t BitMatcher<T>::csize(const std::vector<size_t>& idxs) const{
  size_t num = idxs.size();
  size_t result = num*rsz/cNum;
  for(auto index:idxs){
    uint8_t fp = (index*pseed)%256;
    uint64_t hashval = hash_fn(index);
    const Bucket<T> &bkt1 = buckets.at(hashval%bNum);
    const Bucket<T> &bkt2 = buckets.at((hashval^fp)%bNum);
    result += bkt1.csize(fp);
    result += bkt2.csize(fp);
  }
  return result;
}

template <typename T>
void BitMatcher<T>::dumpCntSize(std::ostream& os) const{
  std::cout << double(rsz)/cNum << std::endl;
  os << std::setprecision(3);
  for(size_t i = 0;i<cNum;++i){
    double cz = size_cnt[i] + double(rsz)/cNum;
    T cnt_val = getOriCnt(i);
    size_t ideal;
    if(cnt_val==0){
      ideal = 1;
    } else {
      ideal = floor(log2(cnt_val))+1;
    }
    os << cz << " " << ideal << std::endl;
  }
}


}// end of namespace Counter

#undef DEBUG_BITMATCHER
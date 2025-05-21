/**
 * @file Diamond.h
 * @author hc (you@domain.com)
 * @brief Counter type for Diamond sketch
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#define TEST_DECODE_TIME

#include <common/hash.h>
#include <common/layer.h>
#include <common/utils.h>
#include <iostream>
#include <chrono>

namespace OmniSketch::Counter{

/**
 * @brief Counter layers used in Diamond Sketch
 * 
 * @tparam no_layer Number of Layers
 * @tparam T Counter Type (which should be numerical types) (current implementation only support signed types)
 */
template <int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class Diamond : public LayerCounter<no_layer, T>{
#ifdef TEST_DIAMOND
public:
#else
private:
#endif
  /**
   * @brief Number of counters on each layer, from low to high.
   *
   */
  std::vector<size_t> no_cnt;
  /**
   * @brief Number of counters in delete part
   * 
   */
  size_t no_cnt_del;
  /**
   * @brief Number of counters in carry part
   * 
   */
  size_t no_cnt_carry;
  /**
   * @brief Width of counters on each layer, from low to high.
   *
   */
  std::vector<size_t> width_cnt;
  /**
   * @brief Number of hash function used on increment part
   *
   */
  size_t no_hash_inc;
  /**
   * @brief Number of hash function used on deletion part
   *
   */
  size_t no_hash_del;
  /**
   * @brief Number of hash function used on carry part
   *
   */
  size_t no_hash_carry;
  /**
   * @brief array of hashing classes
   *
   */
  std::vector<hash_t> *hash_fns_inc;
  std::vector<hash_t> hash_fns_del;
  std::vector<hash_t> hash_fns_carry;
  /**
   * @brief Counter layers of the increment part
   *
   */
  std::vector<uint8_t> inc_part[no_layer];
  /**
   * @brief Counters of the carry part
   *
   */
  std::vector<uint8_t> carry_part;
  /**
   * @brief Counters of the delete part
   *
   */
  std::vector<uint16_t> del_part;
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
   * @brief Size of unused higher-layer counters
   * 
   */
  size_t rsz;
  /**
   * @brief Number of counters
   * 
   */
  size_t cNum;

  Diamond(const Diamond &) = delete;
  Diamond(Diamond &&) = delete;

  void update_add(size_t index, T val);
  void update_add_overflow(size_t index, T of_val);
  void update_sub(size_t index, T val);

  int32_t query_carry_part(size_t index);
  uint16_t query_del_part(size_t index);
  uint8_t query_inc_part(int32_t lr, size_t index);

public:
  size_t update_num;
  /**
   * @brief Construct Diamond and initialize inner counters.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param width_cnt Counter width of each layer.
   * 
   */
  Diamond(const std::vector<size_t> &no_cnt, const std::vector<size_t> &width_cnt, 
          size_t cnt_del, size_t cnt_carry, size_t hash_inc, size_t hash_del, size_t hash_carry){
    initDiamond(no_cnt, width_cnt, cnt_del, cnt_carry, hash_inc, hash_del, hash_carry);
  }
  /**
   * @brief Construct without initialize, need to initialize later. 
   * 
   */
  Diamond(){}
  
  /**
   * @brief Release Diamond
   * 
   */
  ~Diamond(){
    delete[] hash_fns_inc;
  }

  /**
   * @brief Initialize Diamond
   * 
   */
  void initDiamond(const std::vector<size_t> &no_cnt, const std::vector<size_t> &width_cnt, 
                  size_t cnt_del, size_t cnt_carry, size_t hash_inc, size_t hash_del, size_t hash_carry);
  
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
  void clear_cnt(size_t index) override;
  /**
   * @brief Query a counter online
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T query(size_t index) override;
  /**
   * @brief Decode all counters
   * 
   */
  void decode();
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
    return original_cnt.at(index);
  }

  size_t rsize() const{
    return rsz;
  }

  size_t csize(const std::vector<size_t>& idxs) const override;

  size_t tagsize() const;
  /**
   * @brief Get the number of overflow counters in layer `lr`
   * 
   */
  size_t getEmptyNum(int32_t lr) const;
  /**
   * @brief Get cNum, the number of counters
   * 
   */
  size_t getcNum() const{
    return cNum;
  }
  /**
   * @brief Clear the counters
   * 
   */
  void clear();
  /**
   * @brief Check the consistency between counters and ori_counters
   * 
   */
  void validate() const{
    size_t num = 0;
    size_t err = 0;
    for (size_t i = 0; i < cNum; i++){
      //std::cout << original_cnt[i] << " " << decoded_cnt[i] << std::endl;
      //if(i%100==99){std::cout << std::endl;}
      if(original_cnt[i]!=decoded_cnt[i]){
        //std::cout << i << ' ' << original_cnt[i] << ' ' << decoded_cnt[i] << std::endl;
        num++;
        err += std::abs(original_cnt[i]-decoded_cnt[i]);
      }
    }
    std::cout << "#Inconsistency: " << num << ", which may due to clear_cnt" << std::endl;
    std::cout << "Inconsistency ratio: " << (double)num/cNum << std::endl;
    std::cout << "Counter AAE: " << (double)err/cNum << std::endl;
    std::cout << "Tag size: " << tagsize() << " ,"  << "empty counter size: " << 0 << std::endl;
  }

  /**
   * @brief Dump decoded counters to ostream
   * 
   */
  void dumpCnt(std::ostream& os) const;
  /**
   * @brief Dump original counters to ostream
   * 
   */
  void dumpOri(std::ostream& os) const;
  /**
   * @brief Dump the actual counter size and its ideal size
   * 
   */
  void dumpCntSize(std::ostream& os) const;

};

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::initDiamond(const std::vector<size_t> &_no_cnt, 
    const std::vector<size_t> &_width_cnt, size_t cnt_del, size_t cnt_carry, 
    size_t hash_inc, size_t hash_del, size_t hash_carry){
  no_hash_inc = hash_inc;
  no_hash_del = hash_del;
  no_hash_carry = hash_carry;
  no_cnt_carry = cnt_carry;
  no_cnt_del = cnt_del;
  // validity check
  if (no_layer <= 1) {
    throw std::invalid_argument(
        "Invalid Template Argument: `no_layer` must > 1, got " +
        std::to_string(no_layer) + ".");
  }
  if (_no_cnt.size() != no_layer) {
    throw std::invalid_argument(
        "Invalid Argument: `no_cnt` should be of size " +
        std::to_string(no_layer) + ", but got size " +
        std::to_string(_no_cnt.size()) + ".");
  }
  if (_width_cnt.size() != no_layer) {
    throw std::invalid_argument(
        "Invalid Argument: `width_cnt` should be of size " +
        std::to_string(no_layer) + ", but got size " +
        std::to_string(_width_cnt.size()) + ".");
  }
  for (auto i : _no_cnt) {
    if (i == 0) {
      throw std::invalid_argument(
          "Invalid Argument: There is a zero in `no_cnt`.");
    }
  }
  for (auto i : _width_cnt) {
    if (i == 0) {
      throw std::invalid_argument(
          "Invalid Argument: There is a zero in `width_cnt`.");
    }
  }
  size_t length = 0;
  for (auto i : _width_cnt) {
    size_t tmp = length + i;
    if (tmp < length || tmp > sizeof(T) * 8) {
      throw std::invalid_argument(
          "Invalid Argument: Aggregate length of `width_cnt` is too large.");
    }
    length = tmp;
  }
  
  // initialize hash funcs
  hash_fns_inc = new std::vector<hash_t>[no_layer];
  for (int32_t i = 0; i < no_layer; ++i) {
    hash_fns_inc[i] = std::vector<hash_t>(no_hash_inc);
  }
  hash_fns_del = std::vector<hash_t>(no_hash_del);
  hash_fns_carry = std::vector<hash_t>(no_hash_carry);
  // initialize counter array
  no_cnt = _no_cnt;
  width_cnt = _width_cnt;
  for (int32_t i = 0; i < no_layer; ++i) {
    inc_part[i] = std::vector<uint8_t>(no_cnt[i]);
  }
  carry_part = std::vector<uint8_t>(no_cnt_carry);
  del_part = std::vector<uint16_t>(no_cnt_del);
  // original counters, value initialized
  original_cnt.resize(no_cnt[0]);
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  decoded_cnt.resize(no_cnt[0]);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  size_cnt.resize(no_cnt[0]);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
  rsz = 0;
  cNum = no_cnt[0];
  update_num = 0;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::update_add(size_t index, T val){
  T new_val = inc_part[0][index] + val;
  T of_val = new_val >> width_cnt[0];
  new_val = new_val & ((1 << width_cnt[0])-1);
  inc_part[0][index] = new_val;
  if(of_val!=0){
    update_add_overflow(index, of_val);
  }
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::update_add_overflow(size_t index, T val){
  int32_t lr;
  std::vector<size_t> idxs(no_hash_inc);
  for(lr = 1;lr<no_layer;++lr){
    uint8_t min_val = 255;
    for(size_t i = 0;i<no_hash_inc;++i){
      size_t inc_idx = hash_fns_inc[lr][i](index) % no_cnt[lr];
      idxs[i] = inc_idx;
      min_val = std::min(min_val, inc_part[lr][inc_idx]);
    }
    T newval = T(min_val) + val;
    T ofval = newval >> width_cnt[lr];
    newval = newval & ((1 << width_cnt[lr])-1);
    if(ofval!=0){ // overflow
      if(lr==no_layer-1){
        throw std::overflow_error(
            "Counter overflow at the last layer in Diamond, overflow by " +
            std::to_string(ofval) + ".");
      }
      for(size_t i = 0;i<no_hash_inc;++i){
        size_t inc_idx = idxs[i];
        inc_part[lr][inc_idx] = newval;
      }
      val = ofval;
    } else {
      for(size_t i = 0;i<no_hash_inc;++i){
        size_t inc_idx = idxs[i];
        inc_part[lr][inc_idx] = std::max(uint8_t(newval), inc_part[lr][inc_idx]);
      }
      break;
    }
  }
  // set carry part to at least `lr`
  for(size_t i = 0;i<no_hash_carry;++i){
    size_t carry_idx = hash_fns_carry[i](index) % no_cnt_carry;
    carry_part[carry_idx] = std::max(uint8_t(lr), carry_part[carry_idx]);
  }
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::update_sub(size_t index, T val){
  std::vector<size_t> idxs(no_hash_del);
  uint16_t min_val = std::numeric_limits<uint16_t>::max();
  for(size_t i = 0;i<no_hash_del;++i){
    size_t del_idx = hash_fns_del[i](index) % no_cnt_del;
    idxs[i] = del_idx;
    min_val = std::min(min_val, del_part[del_idx]);
  }
  min_val+=uint16_t(-val);
  for(size_t i = 0;i<no_hash_del;++i){
    size_t del_idx = idxs[i];
    del_part[del_idx] = std::max(min_val, del_part[del_idx]);
  }
}


template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::update(size_t index, T val){
  update_num += 1;
  original_cnt[index]+=val;
  if(val>=0){
    update_add(index, val);
  } else {
    update_sub(index, val);
  }
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::clear_cnt(size_t index){
  original_cnt[index] = 0; 
  T val = query(index);
  if(val>0){
    update_sub(index, -val);
  } else {
    update_add(index, -val);
  }
}

template <int32_t no_layer, typename T, typename hash_t>
T Diamond<no_layer, T, hash_t>::query(size_t index){
  // query carry part
  int32_t depth = query_carry_part(index);
  // increment part
  T result = inc_part[0][index];
  size_t cur_bits = width_cnt[0];
  for(int32_t lr = 1;lr<=depth;++lr){
    uint8_t val = query_inc_part(lr, index);
    result += T(val) << cur_bits;
    cur_bits += width_cnt[lr];
  }
  // delete part
  uint16_t del_val = query_del_part(index);
  result -= del_val;
  return result;
}

template <int32_t no_layer, typename T, typename hash_t>
int32_t Diamond<no_layer, T, hash_t>::query_carry_part(size_t index){
  int32_t depth = no_layer;
  for(size_t i = 0;i<no_hash_carry;++i){
    size_t carry_idx = hash_fns_carry[i](index) % no_cnt_carry;
    depth = std::min(depth, int32_t(carry_part[carry_idx]));
  }
  return depth;
}

template <int32_t no_layer, typename T, typename hash_t>
uint16_t Diamond<no_layer, T, hash_t>::query_del_part(size_t index){
  uint16_t min_val = std::numeric_limits<uint16_t>::max();
  for(size_t i = 0;i<no_hash_del;++i){
    size_t del_idx = hash_fns_del[i](index) % no_cnt_del;
    min_val = std::min(min_val, del_part[del_idx]);
  }
  return min_val;
}

template <int32_t no_layer, typename T, typename hash_t>
uint8_t Diamond<no_layer, T, hash_t>::query_inc_part(int32_t lr, size_t index){
  uint8_t min_val = 255;
  for(size_t i = 0;i<no_hash_inc;++i){
    size_t inc_idx = hash_fns_inc[lr][i](index) % no_cnt[lr];
    min_val = std::min(min_val, inc_part[lr][inc_idx]);
  }
  return min_val;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::decode(){
  std::vector<size_t> accumulate_bits(no_layer);
  accumulate_bits[0] = width_cnt[0];
  for(int32_t i = 1;i<no_layer;++i){
    accumulate_bits[i] = accumulate_bits[i-1]+width_cnt[i];
  }
  for(size_t i = 0;i<cNum;++i){
    size_cnt[i] = accumulate_bits[query_carry_part(i)];
  }
  //for(int32_t lr = 0;lr<no_layer;++lr){
  //  std::cout << lr << ' ' << getEmptyNum(lr) << std::endl;
  //  rsz+=width_cnt[lr]*getEmptyNum(lr);
  //}
  rsz=no_cnt_carry*2+no_cnt_del*8;
#ifdef TEST_DECODE_TIME
  auto MY_TIMER = std::chrono::microseconds::zero();
  auto MY_TICK = std::chrono::steady_clock::now();
  auto MY_TOCK = std::chrono::steady_clock::now();
#endif
  for(size_t i = 0;i<cNum;++i){
    decoded_cnt[i] = query(i);
  }
#ifdef TEST_DECODE_TIME
  MY_TOCK = std::chrono::steady_clock::now();
  MY_TIMER = std::chrono::duration_cast<std::chrono::microseconds>(MY_TOCK -
                                                                   MY_TICK);
  printf("\nDECODE COST %jdus\n", static_cast<intmax_t>(MY_TIMER.count()));
  printf("\nQuery thrpt %lfMops\n", double(cNum)/MY_TIMER.count());
#endif
}

template <int32_t no_layer, typename T, typename hash_t>
size_t Diamond<no_layer, T, hash_t>::getEmptyNum(int32_t lr) const{
  size_t num = 0;
  for(size_t i = 0;i<no_cnt[lr];++i){
    if(inc_part[lr][i]==0){
      num++;
    }
  }
  return num;
}

template <int32_t no_layer, typename T, typename hash_t>
size_t Diamond<no_layer, T, hash_t>::csize(const std::vector<size_t>& idxs) const{
  size_t num = idxs.size();
  size_t result = num*rsz/cNum;
  for(auto index:idxs){
    result += size_cnt[index];
  }
  return result;
}

template <int32_t no_layer, typename T, typename hash_t>
size_t Diamond<no_layer, T, hash_t>::tagsize() const{
  return no_cnt_carry*2/8+no_cnt_del*8/8;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::clear(){
  for (int32_t i = 0; i < no_layer; ++i) {
    inc_part[i] = std::vector<uint8_t>(no_cnt[i]);
  }
  del_part = std::vector<uint16_t>(no_cnt_del);
  carry_part = std::vector<uint16_t>(no_cnt_carry);
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
  rsz = 0;
  update_num = 0;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::dumpCnt(std::ostream& os) const{
  for(auto i: decoded_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::dumpOri(std::ostream& os) const{
  for(auto i: original_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T, typename hash_t>
void Diamond<no_layer, T, hash_t>::dumpCntSize(std::ostream& os) const{
  std::cout << "average rsize: " << double(rsz)/cNum << std::endl;
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

}
/**
 * @file Stingy.h
 * @author hc (you@domain.com)
 * @brief Counter type for Stingy sketch
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#define TEST_DECODE_TIME
#define MAX_TRY  100
#define KICK_TAG 63
#define NULL_VAL 0

#include <common/layer.h>
#include <common/utils.h>
#include <iostream>
#include <chrono>

namespace OmniSketch::Counter{

/**
 * @brief Counter layers used in Stingy Sketch (current implementation only support non-negative values)
 * 
 * @tparam no_layer Number of Layers
 * @tparam T Counter Type (which should be numerical types)
 */
template <int32_t no_layer, typename T>
class Stingy : public LayerCounter<no_layer, T>{
#ifdef TEST_STINGY
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
   * @brief Counters in each layer
   *
   */
  std::vector<uint8_t> cnt_array[no_layer];
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
   * @brief Size of unused higher-layer counters (in bits)
   * 
   */
  size_t rsz;
  /**
   * @brief Number of counters
   * 
   */
  size_t cNum;
  /**
   * @brief Seed used for kick out. The formula is: kick_idx = (original_idx + pseed)%cNum.
   * 
   */
  size_t pseed;

  Stingy(const Stingy &) = delete;
  Stingy(Stingy &&) = delete;

  /**
   * @brief Get the index of a sibling node (under the same parent node)
   * 
   */
  size_t get_sibling(size_t index){
    if(index%2==0){
      return index+1;
    } else {
      return index-1;
    }
  }

  size_t get_nonempty_child(int32_t lr, size_t index){
    assert(lr>0);
    if(cnt_array[lr-1][index*2]!=NULL_VAL){
      return index*2;
    } else if(cnt_array[lr-1][index*2+1]!=NULL_VAL){
      return index*2+1;
    } else {
      std::cerr << "broken carry chain" << std::endl;
      exit(0);
    }
    return 0;
  }

  bool check_parent_empty(int32_t lr, size_t index){
    if (lr==no_layer-1){
      return false;
    }
    return cnt_array[lr+1][index/2]==NULL_VAL;
  }

  bool check_sibling_empty(int32_t lr, size_t index){
    return cnt_array[lr][get_sibling(index)]==NULL_VAL;
  }

  /**
   * @brief Kick out the counter at `index` to another place
   * 
   * @return size_t The kick out place of the counter
   */
  size_t kick_out(size_t index);

  size_t kick_out(int32_t lr, size_t index);

  /**
   * @brief Find the next place with a value other than KICK_TAG
   * 
   * @note Exit if tried too many times (>= MAX_TRY)
   */
  size_t find_next_pos(size_t index){
    size_t try_num = 0;
    while (true){
      try_num++;
      index = (index+pseed)%cNum;
      if(cnt_array[0][index]!=KICK_TAG){
        return index;
      }
      if(try_num>MAX_TRY){
        std::cerr << "kick out fail" << std::endl;
        exit(0);
      }
    }
    return 0;  
  }

  /**
   * @brief Try to set the counter, return if success
   * 
   * @note If fail (return false), the original value may be overwritten. Make sure to store it first.
   * 
   * @return false if the original place can't hold the value
   */
  bool set_counter(size_t index, T val);

  /**
   * @brief Update a segment by one. 
   * 
   * @param index Counter index
   */
  void updateOne(int32_t lr, size_t index);

  std::pair<T, size_t> query_with_size(size_t ori_index);

public:
  /**
   * @brief Construct Stingy and initialize inner counters.
   * 
   * @param counter_num Number of counters you wish to use.
   * 
   */
  Stingy(size_t counter_num){
    initCounter(counter_num);
  }
  /**
   * @brief Construct without initialize, need to initialize later. 
   * 
   */
  Stingy(){}
  
  /**
   * @brief Release Stingy
   * 
   */
  ~Stingy(){}

  /**
   * @brief Initialize Stingy
   * 
   * @param counter_num Number of counters you wish to use.
   * 
   */
  void initCounter(size_t counter_num);
  
  /**
   * @brief Update a counter
   * 
   * @param ori_index Counter index
   * @param val Value to be added
   */
  void update(size_t ori_index, T val) override;

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
  T query(size_t ori_index) override{
    return query_with_size(ori_index).first;
  }

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
      if(getOriCnt(i)!=getCnt(i)){
        num++;
        if(getOriCnt(i)>getCnt(i))
          err += getOriCnt(i)-getCnt(i);
        else 
          err += getCnt(i)-getOriCnt(i);
      }
    }
    std::cout << "#Inconsistency: " << num << ", which may due to clear_cnt" << std::endl;
    std::cout << "Inconsistency ratio: " << (double)num/cNum << std::endl;
    std::cout << "Counter ARE: " << (double)err/cNum << std::endl;
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

};

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::initCounter(size_t counter_num){
  // validity check
  if (no_layer <= 1) {
    throw std::invalid_argument(
        "Invalid Template Argument: `no_layer` must > 1, got " +
        std::to_string(no_layer) + ".");
  }
  // initialize kickout seeds
  cNum = counter_num;
  int32_t candidate = 31;
  int32_t cNum32 = static_cast<int32_t>(cNum);
  while(true){
    if(Util::IsCoprime(candidate, cNum32)){
      pseed = static_cast<size_t>(candidate);
      break;
    }else{candidate++;}
  }
  // initialize counter array
  no_cnt.resize(size_t(no_layer));
  no_cnt[0] = cNum;
  for (int32_t i = 1; i< no_layer; ++i) {
    no_cnt[i] = (no_cnt[i-1]+1)/2;
  }
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<uint8_t>(no_cnt[i], 0);
  }
  // original counters, value initialized
  original_cnt.resize(no_cnt[0]);
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  decoded_cnt.resize(no_cnt[0]);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  size_cnt.resize(no_cnt[0]);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
}

template <int32_t no_layer, typename T>
bool Stingy<no_layer, T>::set_counter(size_t index, T val){
  // special handle the first layer
  if(cnt_array[0][index]==NULL_VAL){
    if (!check_parent_empty(0, index)){
      return false;
    }
  }
  cnt_array[0][index] = (val % 62)+1; // maps the value into [1, 62]
  val /= 62;
  if(val==0){
    return true;
  }
  // for the other layers
  size_t last_index;
  for(int32_t lr = 1;lr<no_layer;++lr){
    last_index = index;
    index/=2;
    if (cnt_array[lr][index]==NULL_VAL) { // first use
      if (!check_parent_empty(lr, index) && lr!=no_layer-1){
        return false;
      }
      if (!check_sibling_empty(lr-1, last_index)){
        // kick out the other carry chain, won't disrupt the intended value of the current chain
        kick_out(lr-1, get_sibling(last_index)); 
      }
    }
    cnt_array[lr][index] = (val % 3)+1; // maps the value into [1, 3]
    val /= 3;
    if(val==0){
      return true;
    }
  }
  return false;
}

template <int32_t no_layer, typename T>
size_t Stingy<no_layer, T>::kick_out(size_t index){
  T val = query(index);
  // Reset original counter
  clear_cnt(index);
  cnt_array[0][index] = KICK_TAG;
  // Find and set the next place
  size_t nxt_idx = index;
  while (true) {
    nxt_idx = find_next_pos(nxt_idx);
    val += query(nxt_idx);
    bool suc = set_counter(nxt_idx, val);
    if(!suc){
      clear_cnt(nxt_idx);
      cnt_array[0][nxt_idx] = KICK_TAG;
    } else {
      break;
    }
  }
  return nxt_idx;
}

template <int32_t no_layer, typename T>
size_t Stingy<no_layer, T>::kick_out(int32_t lr, size_t index){
  for(int32_t i = lr;i>0;--i){
    index = get_nonempty_child(i, index);
  }
  // special handle for the first layer
  if(cnt_array[0][index*2]!=NULL_VAL && cnt_array[0][index*2]!=KICK_TAG){
    index = index*2;
  } else if(cnt_array[0][index*2+1]!=NULL_VAL && cnt_array[0][index*2+1]!=KICK_TAG){
    index = index*2+1;
  } else {
    std::cerr << "broken carry chain" << std::endl;
    exit(0);
  }
  return kick_out(index);
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::updateOne(int32_t lr, size_t index){
  assert(lr<no_layer && cnt_array[lr][index]!=KICK_TAG);
  // first use
  if(cnt_array[lr][index]==NULL_VAL){
    if (!check_parent_empty(lr, index) && lr!=no_layer-1){
      size_t nxt_index = kick_out(lr, index); // cnt_arry[lr, index] is NULL, but we can still locate the carry chain anyway
      nxt_index = nxt_index >> lr; // find the grandfather in lr-th layer
      updateOne(lr, nxt_index); // After kicking out, the original value may change, but only larger so the length of chain is at least `lr`.
      return;
    }
    size_t child = get_nonempty_child(lr, index);
    if (!check_sibling_empty(lr-1, child)){
      kick_out(lr-1, get_sibling(child));
    }
    cnt_array[lr][index] = 1;
  }
  cnt_array[lr][index] += 1;
  uint8_t obj_val = (lr==0) ? 62:3;
  if(cnt_array[lr][index]==obj_val){
    cnt_array[lr][index] = 1;
    updateOne(lr+1, index/2);
  }

}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::update(size_t ori_index, T val){
  assert(val>=0);
  original_cnt[ori_index]+=val;
  size_t index = ori_index;
  while (cnt_array[0][index] == KICK_TAG) {
    index = (index + pseed)%cNum;
  }
  for(T i = 0; i<val; ++i){
    updateOne(0, index);
  }
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::clear_cnt(size_t index){
  original_cnt[index] = 0;
  for (int32_t lr = 0; lr < no_layer; lr++){
    if (cnt_array[lr][index]==NULL_VAL){
      break;
    } else {
      cnt_array[lr][index] = NULL_VAL;
      index /= 2;
    }
  }
}

template <int32_t no_layer, typename T>
std::pair<T, size_t> Stingy<no_layer, T>::query_with_size(size_t ori_index){
  size_t index = ori_index;
  while (cnt_array[0][index] == KICK_TAG) {
    index = (index + pseed)%cNum;
  }
  if (cnt_array[0][index]==NULL_VAL){
    return std::make_pair(0, 6);
  }
  T cur_times = 62;
  T result = cnt_array[0][index]-1;
  int32_t lr;
  for(lr = 1;lr<no_layer;++lr){
    index /= 2;
    if(cnt_array[lr][index]==NULL_VAL){break;} // no overflow to this layer
    result+=(cnt_array[lr][index]-1)*cur_times;
    cur_times *= 3;
  }
  return std::make_pair(result, size_t(2*lr+4));
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::decode(){
#ifdef TEST_DECODE_TIME
  auto MY_TIMER = std::chrono::microseconds::zero();
  auto MY_TICK = std::chrono::steady_clock::now();
  auto MY_TOCK = std::chrono::steady_clock::now();
#endif
  for(size_t i = 0;i<cNum;++i){
    auto pr = query_with_size(i);
    decoded_cnt[i] = pr.first;
    size_cnt[i] = pr.second;
  }
  for(int32_t lr = 1;lr<no_layer;++lr){
    rsz+=2*getEmptyNum(lr);
  }
#ifdef TEST_DECODE_TIME
  MY_TOCK = std::chrono::steady_clock::now();
  MY_TIMER = std::chrono::duration_cast<std::chrono::microseconds>(MY_TOCK -
                                                                   MY_TICK);
  printf("\nDECODE COST %jdms\n", static_cast<intmax_t>(MY_TIMER.count()));
#endif
}

template <int32_t no_layer, typename T>
size_t Stingy<no_layer, T>::getEmptyNum(int32_t lr) const{
  size_t num = 0;
  for(size_t i = 0;i<no_cnt[lr];++i){
    if(cnt_array[lr][i]==NULL_VAL || cnt_array[lr][i]==KICK_TAG){
      num++;
    }
  }
  return num;
}

template <int32_t no_layer, typename T>
size_t Stingy<no_layer, T>::csize(const std::vector<size_t>& idxs) const{
  size_t num = idxs.size();
  size_t result = num*rsz/cNum;
  for(auto ori_index:idxs){
    result += size_cnt[ori_index];
  }
  return result;
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::clear(){
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<uint8_t>(no_cnt[i], 0);
  }
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
  rsz = 0;
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::dumpCnt(std::ostream& os) const{
  for(auto i: decoded_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T>
void Stingy<no_layer, T>::dumpOri(std::ostream& os) const{
  for(auto i: original_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

}
/**
 * @file Pyramid.h
 * @author hc (you@domain.com)
 * @brief Counter type for Pyramid sketch
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
#define TEST_DECODE_TIME

#include <common/layer.h>
#include <common/utils.h>
#include <iomanip>
#include <iostream>
#include <chrono>

namespace OmniSketch::Counter{

/**
 * @brief Counter layers used in Pyramid Sketch
 * 
 * @tparam no_layer Number of Layers
 * @tparam T Counter Type (which should be numerical types)
 */
template <int32_t no_layer, typename T>
class Pyramid : public LayerCounter<no_layer, T>{
#ifdef TEST_PYRAMID
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
   * @brief Width of counters on each layer, from low to high.
   *
   */
  std::vector<size_t> width_cnt;
  /**
   * @brief Counters in each layer
   *
   */
  std::vector<Util::DynamicIntX<T>> cnt_array[no_layer];
  /**
   * @brief Status bits, corresponding to left tag and right tag
   *
   */
  std::vector<bool> status_bits[no_layer];
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
   * @brief Size of unused higher-layer counters and status-arrays (in bits)
   * 
   */
  size_t rsz;
  /**
   * @brief Number of counters
   * 
   */
  size_t cNum;
  /**
   * @brief Seed used for random permutation. The formula is: true_idx = (original_idx*pseed)%cNum.
   * Therefore, for inversibility, `pseed` needs to be coprime with `cNum`. Also, we need to spread
   * the counters of each sketch evenly in the buckets, so `pseed` should be much greater than the number
   * of sketch instances.
   * 
   */
  size_t pseed;
  /**
   * @brief The inverse of `pseed` modula `cNum`
   * 
   */
  size_t iseed;

  Pyramid(const Pyramid &) = delete;
  Pyramid(Pyramid &&) = delete;

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

public:
  size_t update_num;
  size_t update_mem_access;
  size_t max_update_mem_access;
  size_t query_mem_access;
  size_t max_query_mem_access;
  /**
   * @brief Construct Pyramid and initialize inner counters.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param width_cnt Counter width of each layer.
   * 
   */
  Pyramid(size_t counter_num, const std::vector<size_t> &width_cnt){
    initPyramid(counter_num, width_cnt);
  }
  /**
   * @brief Construct without initialize, need to initialize later. 
   * 
   */
  Pyramid(){}
  
  /**
   * @brief Release Pyramid
   * 
   */
  ~Pyramid(){}

  /**
   * @brief Initialize Pyramid
   * 
   * @param counter_num Number of counters you wish to use.
   * @param width_cnt Counter width of each layer.
   * 
   */
  void initPyramid(size_t counter_num, const std::vector<size_t> &width_cnt_);
  
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
  void clear_cnt(size_t ori_index) override;
  /**
   * @brief Query a counter online
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T query(size_t ori_index) override;
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
  size_t getOfNum(int32_t lr) const;
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
      if(original_cnt[i]!=decoded_cnt[i]){
        //std::cout << i << ' ' << original_cnt[i] << ' ' << decoded_cnt[i] << std::endl;
        num++;
        err += std::abs(original_cnt[i]-decoded_cnt[i]);
      }
    }
    std::cout << "#Inconsistency: " << num << ", which may due to clear_cnt" << std::endl;
    std::cout << "Inconsistency ratio: " << (double)num/cNum << std::endl;
    std::cout << "Counter ARE: " << (double)err/cNum << std::endl;
    std::cout << "Tag size: " << tagsize() << " ,"  << "empty counter size: " << rsz/8 << std::endl;
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

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::initPyramid(size_t counter_num, const std::vector<size_t> &width_cnt_){
  // validity check
  if (no_layer <= 1) {
    throw std::invalid_argument(
        "Invalid Template Argument: `no_layer` must > 1, got " +
        std::to_string(no_layer) + ".");
  }
  if (width_cnt_.size() != no_layer) {
    throw std::invalid_argument(
        "Invalid Argument: `width_cnt` should be of size " +
        std::to_string(no_layer) + ", but got size " +
        std::to_string(width_cnt_.size()) + ".");
  }
  for (auto i : width_cnt_) {
    if (i == 0) {
      throw std::invalid_argument(
          "Invalid Argument: There is a zero in `width_cnt`.");
    }
  }
  size_t length = 0;
  for (auto i : width_cnt_) {
    size_t tmp = length + i;
    if (tmp < length || tmp > sizeof(T) * 8) {
      throw std::invalid_argument(
          "Invalid Argument: Aggregate length of `width_cnt` is too large.");
    }
    length = tmp;
  }
  // initialize permutation seeds
  cNum = counter_num;
  int32_t candidate = 31;
  int32_t cNum32 = static_cast<int32_t>(cNum);
  while(true){
    if(Util::IsCoprime(candidate, cNum32)){
      pseed = static_cast<size_t>(candidate);
      iseed = static_cast<size_t>(Util::MulInverse(candidate, cNum32));
      break;
    }else{candidate++;}
  }
  // initialize counter array
  width_cnt = width_cnt_;
  no_cnt.resize(size_t(no_layer));
  no_cnt[0] = cNum;
  for (int32_t i = 1; i< no_layer; ++i) {
    no_cnt[i] = (no_cnt[i-1]+1)/2;
  }
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<Util::DynamicIntX<T>>(no_cnt[i], {width_cnt[i]});
  }
  for (int32_t i = 0; i < no_layer; ++i) {
    status_bits[i] = std::vector<bool>(no_cnt[i], false);
  }
  // original counters, value initialized
  original_cnt.resize(no_cnt[0]);
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  decoded_cnt.resize(no_cnt[0]);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  size_cnt.resize(no_cnt[0]);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
  rsz = 0;
  update_num = 0;
  update_mem_access = 0;
  query_mem_access = 0;
  max_update_mem_access = 0;
  max_query_mem_access = 0;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::update(size_t ori_index, T val){
  update_num += 1;
  size_t tmp_access = 0;
  original_cnt[ori_index]+=val;
  size_t index = (ori_index*pseed)%cNum;
  for(int32_t lr = 0;lr<no_layer;++lr){
    tmp_access += 1;
    T of_val = cnt_array[lr][index] + val;
    if(of_val==0){break;}
    else{ // update upper levels
      if(lr==no_layer-1){
        if(of_val==1 && status_bits[lr][index]){
          status_bits[lr][index] = false;
          break;
        } else if(of_val==-1 && !status_bits[lr][index]){
          status_bits[lr][index] = true;
          break;
        } else {
          throw std::overflow_error(
              "Counter overflow at the last layer in Bucket, overflow by " +
              std::to_string(of_val) + ".");
        }
      }
      status_bits[lr][index] = true;
      val = of_val;
      index = index/2;
    }
  }
  update_mem_access += tmp_access;
  max_update_mem_access = std::max(max_update_mem_access, tmp_access);
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::clear_cnt(size_t ori_index){
  original_cnt[ori_index] = 0;
  size_t index = (ori_index*pseed)%cNum;
  for(int32_t lr = 0;lr<no_layer;++lr){
    cnt_array[lr][index].reset();
    if(status_bits[lr][index]){
      status_bits[lr][index] = false;
      index = index/2;
    }else{break;}
  }
}

template <int32_t no_layer, typename T>
T Pyramid<no_layer, T>::query(size_t ori_index){
  size_t index = (ori_index*pseed)%cNum;
  size_t cur_bits = 0;
  size_t tmp_mem_access = 1;
  T result = cnt_array[0][index].getVal();
  bool flag = false;
  for(int32_t lr = 1;lr<no_layer;++lr){
    if(!status_bits[lr-1][index]){flag=true;break;} // no overflow to this layer
    cur_bits+=width_cnt[lr-1];
    tmp_mem_access+=1;
    T cur_val = cnt_array[lr][index/2].getVal(); // the value from parent node
    //if(status_bits[lr-1][get_sibling(index)]){ // the sibling also overflowed
    //  cur_val -= 1;
    //  if(cur_val<0){cur_val = 0;}
    //}
    result+=cur_val<<cur_bits;
    index/=2;
  }
  cur_bits+=width_cnt[no_layer-1];
  if(!flag){
    //std::cout << no_cnt[no_layer-1] << ' ' << index << std::endl;
    //std::cout << status_bits[no_layer-1][index] << std::endl;
    if(status_bits[no_layer-1][index]){
      result -= 1<<cur_bits;
    }
  }
  max_query_mem_access = std::max(max_query_mem_access, tmp_mem_access);
  query_mem_access += tmp_mem_access;
  return result;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::decode(){
  query_mem_access = 0;
  max_query_mem_access = 0;
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
  std::cout <<"update_access: average " << 1.0*update_mem_access/update_num << ", max " << max_update_mem_access << std::endl;
  std::cout <<"query_access: average " << 1.0*query_mem_access/cNum << ", max " << max_query_mem_access << std::endl;
  std::vector<size_t> tmp_cnt(cNum, 0);
  std::fill_n(size_cnt.begin(), no_cnt[no_layer-1], width_cnt[no_layer-1]);
  for(int32_t lr = no_layer-1; lr > 0; --lr){
    std::fill_n(tmp_cnt.begin(), no_cnt[lr-1], 1+width_cnt[lr-1]);
    for(size_t i = 0;i<no_cnt[lr];++i){
      if(status_bits[lr-1][2*i] && status_bits[lr-1][2*i+1]){
        tmp_cnt[2*i] += size_cnt[i]/2;
        tmp_cnt[2*i+1] += size_cnt[i]/2;
      }else if(status_bits[lr-1][2*i]){
        tmp_cnt[2*i] += size_cnt[i];
      }else if(status_bits[lr-1][2*i+1]){
        tmp_cnt[2*i+1] += size_cnt[i];
      }else{
        rsz+=size_cnt[i];
      }
    }
    tmp_cnt.swap(size_cnt);
  }
}

template <int32_t no_layer, typename T>
size_t Pyramid<no_layer, T>::getOfNum(int32_t lr) const{
  return std::accumulate(status_bits[lr].begin(), status_bits[lr].end(), 0);
}

template <int32_t no_layer, typename T>
size_t Pyramid<no_layer, T>::csize(const std::vector<size_t>& idxs) const{
  size_t num = idxs.size();
  size_t result = num*(rsz+tagsize())/cNum;
  for(auto ori_index:idxs){
    size_t index = (ori_index*pseed)%cNum;
    result += size_cnt[index];
  }
  return result;
}

template <int32_t no_layer, typename T>
size_t Pyramid<no_layer, T>::tagsize() const{
  size_t result = 0;
  for (int32_t i = 0; i < no_layer-1; ++i) {
    result+=no_cnt[i];
  }
  return result/8;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::clear(){
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<Util::DynamicIntX<T>>(no_cnt[i], {width_cnt[i]});
  }
  for (int32_t i = 0; i < no_layer; ++i) {
    status_bits[i] = std::vector<bool>(no_cnt[i], false);
  }
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
  rsz = 0;
  update_num = 0;
  update_mem_access = 0;
  query_mem_access = 0;
  max_update_mem_access = 0;
  max_query_mem_access = 0;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::dumpCnt(std::ostream& os) const{
  for(auto i: decoded_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::dumpOri(std::ostream& os) const{
  for(auto i: original_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T>
void Pyramid<no_layer, T>::dumpCntSize(std::ostream& os) const{
  std::cout << "average rsize: " << double(rsz)/cNum << std::endl;
  os << std::setprecision(3);
  for(size_t i = 0;i<cNum;++i){
    size_t idx = (i*pseed)%cNum;
    double cz = size_cnt[idx] + double(rsz)/cNum;
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
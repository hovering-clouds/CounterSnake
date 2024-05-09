/**
 * @file Brick.h
 * @author hc (you@domain.com)
 * @brief Counter type for counter layer sharing
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#define DEBUG_BRICK
#include "utils.h"
#include <numeric>
#include <vector>
#include <set>
#define BTAG_INVALID 0
typedef unsigned short btag_t;

namespace OmniSketch::Counter{

/**
 * @brief Buckets used in Brick. One Bucket holds layers of counters
 * 
 * @tparam no_layer Number of Layers
 * @tparam T Counter Type (which should be numerical types)
 */
template <int32_t no_layer, typename T>
class Bucket{
#ifdef TEST_BRICK
public:
#else
private:
#endif
  /**
   * @brief Number of counters on each layer, from low to high.
   *
   */
  const std::vector<size_t> no_cnt;
  /**
   * @brief Width of counters on each layer, from low to high.
   *
   */
  const std::vector<size_t> width_cnt;
  /**
   * @brief Counters in each layer
   *
   */
  std::vector<Util::DynamicIntX<T>> cnt_array[no_layer];
  /**
   * @brief Each tag in this array corresponds to a segment
   *
   */
  std::vector<btag_t> tag_array[no_layer];

public:
  /**
   * @brief Construct by specifying detailed architectural parameters
   *
   * @param no_cnt      number of counters on each layer, from low to high
   * @param width_cnt   width of counters on each layer, from low to high
   * 
   * @details The meaning of the three parameters stipulates the following
   * requirements:
   * - size of `no_cnt` should equal `no_layer`.
   * - size of `width_cnt` should equal `no_layer`.
   *
   * If any of these is violated, an exception would be thrown. Other
   * conditions that trigger an exception:
   * - Items in these vectors contains a 0.
   * - `no_layer <= 1`
   * - Sum of `width_cnt` exceeds `sizeof(T) * 8`. This constraint is imposed to
   * guarantee proper shifting of counters when decoding.
   */
  Bucket(const std::vector<size_t> &no_cnt, const std::vector<size_t> &width_cnt);
  /**
   * @brief Destructor
   *
   */
  ~Bucket(){
  }
  /**
   * @brief Update a segment
   *
   * @param layer The layer of the segment
   * @param index The index of the segment in that layer
   * @param val Value to be updated
   * 
   * 
   * @return The overflow value
   */
  T updateSegment(const int32_t layer, const size_t index, const T val){
    T c_overflow = cnt_array[layer][index] + val;
    return c_overflow;
  }
  /**
   * @brief Reset the segment to be 0
   * 
   */
  void resetSegment(const int32_t layer, const size_t index){
    cnt_array[layer][index].reset();
  }
  /**
   * @brief Get the value of a segment
   * 
   * @param layer The layer of the segment
   * @param index The index of the segment in that layer
   * 
   * @return The value of this segment
   */
  T getSegment(const int32_t layer, const size_t index){
    return cnt_array[layer][index].getVal();
  }
  /**
   * @brief Set the tag of a segment
   * 
   * @param layer The layer of the tag
   * @param index The index of the tag in that layer
   * @param tag The tag value
   */
  void setTag(const int32_t layer, const size_t index, const btag_t tag){
    tag_array[layer][index] = tag;
  }
  /**
   * @brief Get the tag of a segment
   * 
   * @param layer The layer of the tag
   * @param index The index of the tag in that layer
   * 
   * @return The tag value
   */
  btag_t getTag(const int32_t layer, const size_t index){
    return tag_array[layer][index];
  }
  /**
   * @brief Get the number of unused counters of the given layer
   * 
   */
  size_t getUnusedNum(int32_t layer) const;
  /**
   * @brief Clear the counters
   * 
   */
  void clearAll();
  /**
   * @brief Get memory consumption of the layers.
   * Consisting of two parts, the counters and the tags.
   * 
   * @return Memory consumption in bits
   */
  size_t bits_num(size_t tag_len) const;
};

/**
 * @brief This class will take care of the addressing process and the random
 * prmutation needed for multi-sketch use case.
 * 
 * @tparam T Inner counter type (which should be numerical types).
 */
template <int32_t no_layer, typename T>
class Brick {
#ifdef TEST_BRICK
public:
#else
private:
#endif
  /**
   * @brief Number of counters
   * 
   */
  size_t cNum;
  /**
   * @brief Number of segments in a group
   * 
   */
  size_t gNum;
  /**
   * @brief Size of unused higher-layer counters and status-arrays (in bits)
   * 
   */
  size_t rsz;
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
  /**
   * @brief Buckets used to store the counters
   * 
   */
  std::unique_ptr<Bucket<no_layer, T>> bucket_ptr;
  /**
   * @brief The number of shared segments in each layer
   *
   */
  std::vector<size_t> di;
  /**
   * @brief Width of counters on each layer, from low to high.
   *
   */
  std::vector<size_t> width_cnt;
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
  std::vector<size_t> cnt_size;
  
  typedef std::pair<int32_t, size_t> seg_idx;
  /**
   * @brief Reported overflow, in the form (layer, index)
   * 
   */
  std::vector<std::pair<seg_idx, T>> report_ofl;

  Brick(const Brick &) = delete;
  Brick(Brick &&) = delete;
  /**
   * @brief Report overflow to control plane
   * 
   * @note The index is the counter index (layer0 index), not the segment index
   * 
   */
  void report(int32_t layer, size_t index, T val){
    seg_idx sidx = std::make_pair(layer, index);
    report_ofl.push_back(std::make_pair(sidx, val));
    std::cout << "report overflow at layer "<< layer << ", index "  << index << ", value " << val << std::endl;
  }
  /**
   * @brief Query a counter and its layer, should be used offline in `decode`
   * 
   * @param ori_index 
   * @return counter value, layer
   */
  std::pair<T, int32_t> query_with_layer(size_t ori_index);
public:
  /**
   * @brief Construct Brick and initialize inner Buckets.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param dway Number shared segments in each layer.
   * @param width_cnt Counter width of each layer.
   * 
   */
  Brick(size_t counter_num, size_t group_num, const std::vector<size_t> &dway,
        const std::vector<size_t> &width_cnt){
    initBucket(counter_num, group_num, dway, width_cnt);
  }

  /**
   * @brief Construct without initialize Buckets, need to initialize later. 
   * 
   */
  Brick(){}
  
  /**
   * @brief Release Bricks
   * 
   */
  ~Brick(){}

  /**
   * @brief Initialize Buckets
   * 
   * @param counter_num Number of counters you wish to use.
   * @param dway Number shared segments in each layer.
   * @param width_cnt Counter width of each layer.
   * 
   */
  void initBucket(size_t counter_num, size_t group_num,
      const std::vector<size_t> &dway, const std::vector<size_t> &width_cnt);

  /**
   * @brief Update a counter
   * 
   * @param ori_index Counter index
   * @param val Value to be added
   */
  void update(size_t ori_index, T val);
  /**
   * @brief Query a counter online
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T query(size_t ori_index);
  /**
   * @brief Reset the counter, will clear the tag
   * 
   * @param ori_index Counter index
   */
  void clear_cnt(size_t ori_index);

  /**
   * @brief Decode all the counters into `decode_cnt` and their sizes into `cnt_size`.
   * Note we need to merge the results in `reported_ofl`
   * 
   */
  void decode();

  /**
   * @brief Get the value of a counter offline. Should be used after decoding
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T getCnt(size_t ori_index) const{
    return decoded_cnt.at(ori_index);
  }

  /**
   * @brief Get the value of the original counter, i.e. the ground truth.
   * 
   */
  T getOriCnt(size_t ori_index) const{
    return original_cnt.at(ori_index);
  }
  /**
   * @brief Get the reference of a decoded counter. Should be used after decoding
   * 
   * @param ori_index Counter index
   * @return T& The counter reference
   */
  T& operator[](size_t ori_index){
    return decoded_cnt[ori_index];
  }
  /**
   * @brief Get memory consumption of this Brick in bytes
   * 
   */
  size_t bsize() const;
  /**
   * @brief Get the memory usage of the given counters, return in bits
   * 
   */
  size_t csize(const std::vector<size_t>& idxs) const;
  /**
   * @brief Get the redundant memory in bits. (Size of unused higher-layer counters and status-arrays)
   * 
   */
  size_t rsize() const{
    return rsz;
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
    for(size_t i = 0; i<decoded_cnt.size(); ++i){
      os << decoded_cnt[i] << ' ';
      if((i+1)%gNum==0){
        os << std::endl;
      }
    }
  }
  /**
   * @brief Dump original counters to ostream
   * 
   */
  void dumpOri(std::ostream& os) const{
    for(size_t i = 0; i<decoded_cnt.size(); ++i){
      os << original_cnt[i] << ' ';
      if((i+1)%gNum==0){
        os << std::endl;
      }
    }
  }
  /**
   * @brief Dump the index of unhandled overflows in each layer
   * 
   */
  void dumpOfIdx(std::ostream& os) const;
  /**
   * @brief Dump the number of free counters of each layer
   * 
   */
  void dumpFreeCnt(std::ostream& os) const;
  /**
   * @brief Check the consistency between counters and ori_counters
   * 
   */
  void validate() const{
    size_t num = 0;
    for (size_t i = 0; i < cNum; i++){
      if(original_cnt[i]!=decoded_cnt[i]){
        num++;
      }
    }
    std::cout << "#Inconsistency: " << num << std::endl;
  }
  /**
   * @brief Clear the counters
   * 
   */
  void clear(){
    bucket_ptr->clearAll();
    std::fill_n(original_cnt.begin(), cNum, 0);
    rsz = 0;
  }
};

} //namespace Ominisketch
namespace OmniSketch::Counter {

template <int32_t no_layer, typename T>
Bucket<no_layer, T>::Bucket(
      const std::vector<size_t> &no_cnt,
      const std::vector<size_t> &width_cnt)
      : no_cnt(no_cnt), width_cnt(width_cnt){
  // validity check
  if (no_layer <= 1) {
    throw std::invalid_argument(
        "Invalid Template Argument: `no_layer` must > 1, got " +
        std::to_string(no_layer) + ".");
  }
  if (no_cnt.size() != no_layer) {
    throw std::invalid_argument(
        "Invalid Argument: `no_cnt` should be of size " +
        std::to_string(no_layer) + ", but got size " +
        std::to_string(no_cnt.size()) + ".");
  }
  if (width_cnt.size() != no_layer) {
    throw std::invalid_argument(
        "Invalid Argument: `width_cnt` should be of size " +
        std::to_string(no_layer) + ", but got size " +
        std::to_string(width_cnt.size()) + ".");
  }
  for (auto i : no_cnt) {
    if (i == 0) {
      throw std::invalid_argument(
          "Invalid Argument: There is a zero in `no_cnt`.");
    }
  }
  for (auto i : width_cnt) {
    if (i == 0) {
      throw std::invalid_argument(
          "Invalid Argument: There is a zero in `width_cnt`.");
    }
  }
  size_t length = 0;
  for (auto i : width_cnt) {
    size_t tmp = length + i;
    if (tmp < length || tmp > sizeof(T) * 8) {
      throw std::invalid_argument(
          "Invalid Argument: Aggregate length of `width_cnt` is too large.");
    }
    length = tmp;
  }
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<Util::DynamicIntX<T>>(no_cnt[i], {width_cnt[i]});
  }
  for (int32_t i = 1; i < no_layer; ++i) {
    // initialize status_array of layer i with number of counters in layer i-1
    tag_array[i] = std::vector<btag_t>(no_cnt[i], BTAG_INVALID);
  }
}

template <int32_t no_layer, typename T>
size_t Bucket<no_layer, T>::getUnusedNum(int32_t layer) const{
  if(layer==0){return 0;}
  size_t result = 0;
  for(auto idx:tag_array[layer]){
    if(idx==BTAG_INVALID){result++;}
  }
  return result;
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::clearAll(){
  for (int32_t lr = 0;lr<no_layer;++lr){
    for (auto& seg:cnt_array[lr]){
      seg.reset();
    }
  }
  for (int32_t lr = 1;lr<no_layer;++lr){
    std::fill_n(tag_array[lr].begin(), no_cnt[lr], BTAG_INVALID);
  }
}

template <int32_t no_layer, typename T>
size_t Bucket<no_layer, T>::bits_num(size_t tag_len) const{
  size_t bits = no_cnt[0]*width_cnt[0];
  for(int32_t lr = 1; lr<no_layer; ++lr){
    bits+=no_cnt[lr]*(width_cnt[lr]+tag_len);
  }
  return bits;
}


template <int32_t no_layer, typename T>
void Brick<no_layer, T>::initBucket( size_t counter_num,
    size_t group_num, const std::vector<size_t> &dway,
    const std::vector<size_t> &width_cnt_){
  cNum = counter_num;
  gNum = group_num;
  rsz = 0;
  di = dway;
  width_cnt = width_cnt_;
  // initialize permutation seeds
  int32_t candidate = 31;
  int32_t cNum32 = static_cast<int32_t>(cNum);
  while(true){
    if(Util::IsCoprime(candidate, cNum32)){
      pseed = static_cast<size_t>(candidate);
      iseed = static_cast<size_t>(Util::MulInverse(candidate, cNum32));
      break;
    }else{candidate++;}
  }
  // initialize bucket_ptr
  std::vector<size_t> no_cnt(no_layer);
  no_cnt[0] = cNum;
  for(int32_t lr = 1;lr<no_layer;++lr){
    size_t no_grp = (no_cnt[lr-1]+gNum-1)/gNum;
    no_cnt[lr] = no_grp*di[lr];
  }
  bucket_ptr = std::make_unique<Bucket<no_layer, T>>(no_cnt, width_cnt);
  // original counters, value initialized
  original_cnt.resize(no_cnt[0]);
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  decoded_cnt.resize(no_cnt[0]);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  cnt_size.resize(no_cnt[0]);
  std::fill_n(cnt_size.begin(), no_cnt[0], 0);
}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::update(size_t ori_index, T val){
  original_cnt[ori_index]+=val;
  size_t index = (ori_index*pseed)%cNum;
  for(int32_t lr = 0;lr<no_layer;++lr){
    T of_val = bucket_ptr->updateSegment(lr, index, val);
    //std::cout << lr << ' ' << index << ' ' <<of_val << std::endl;
    if(of_val!=0){
      if(lr==no_layer-1){ // last layer should not overflow
        throw std::overflow_error(
            "Counter overflow at the last layer in Bucket, overflow by " +
            std::to_string(of_val) + ".");
      }
      val = of_val;
      size_t gid = index/gNum;
      btag_t tag = gNum+(btag_t)index%gNum; // set valid bit as 1
      bool matched = false;
      // Case 1: a matched segment
      for(size_t nextId = gid*di[lr+1]; nextId<(gid+1)*di[lr+1];++nextId){
        if(bucket_ptr->getTag(lr+1, nextId)==tag){
          index = nextId;
          matched = true;
          break;
        }
      }
      if(matched){continue;}
      // Case 2: no match, allocate a new one
      for(size_t nextId = gid*di[lr+1]; nextId<(gid+1)*di[lr+1];++nextId){
        if(bucket_ptr->getTag(lr+1, nextId)==BTAG_INVALID){
          bucket_ptr->setTag(lr+1, nextId, tag);
          index = nextId;
          matched = true;
          break;
        }
      }
      // Case 3: unhandled overflow, report to control plane
      if(!matched){report(lr, ori_index, of_val);break;}
    } else {
      break;
    }
  }
}

template <int32_t no_layer, typename T>
std::pair<T, int32_t> Brick<no_layer, T>::query_with_layer(size_t ori_index){
  size_t index = (ori_index*pseed)%cNum;
  size_t cur_bits = width_cnt[0];
  T result = bucket_ptr->getSegment(0, index);
  int32_t lr;
  for(lr = 1;lr<no_layer;++lr){
    size_t gid = index/gNum;
    btag_t tag = gNum+(btag_t)index%gNum; // set valid bit as 1
    bool matched = false;
    for(size_t nextId = gid*di[lr]; nextId<(gid+1)*di[lr];++nextId){
      if(bucket_ptr->getTag(lr, nextId)==tag){
        index = nextId;
        result+=bucket_ptr->getSegment(lr, index)<<cur_bits;
        cur_bits+=width_cnt[lr];
        matched = true;
        break;
      }
    }
    if(!matched){
      break;
    }
  }
  return std::make_pair(result, lr);
}

template <int32_t no_layer, typename T>
T Brick<no_layer, T>::query(size_t ori_index){
  return query_with_layer(ori_index).first;
}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::clear_cnt(size_t ori_index){
  original_cnt[ori_index] = 0;
  size_t index = (ori_index*pseed)%cNum;
  bucket_ptr->resetSegment(0, index);
  for(int32_t lr = 1;lr<no_layer;++lr){
    size_t gid = index/gNum;
    btag_t tag = gNum+(btag_t)index%gNum; // set valid bit as 1
    bool matched = false;
    for(size_t nextId = gid*di[lr]; nextId<(gid+1)*di[lr];++nextId){
      if(bucket_ptr->getTag(lr, nextId)==tag){
        index = nextId;
        bucket_ptr->resetSegment(lr, index);
        bucket_ptr->setTag(lr, index, BTAG_INVALID);
        matched = true;
        break;
      }
    }
    if(!matched){
      break;
    }
  }

}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::decode(){
  std::vector<size_t> accum_bits(no_layer);
  accum_bits[0] = width_cnt[0];
  for(int32_t lr=1;lr<no_layer;++lr){
    accum_bits[lr] = accum_bits[lr-1]+width_cnt[lr];
  }
  size_t tag_len = ceil(log2(gNum))+1;
  // decoded values
  for(size_t i = 0;i<cNum;++i){
    std::pair<T, size_t> pr = query_with_layer(i);
    decoded_cnt[i] = pr.first;
    cnt_size[i] = accum_bits[pr.second-1]+(pr.second-1)*tag_len;
  }
  // reported values
  for(auto kv: report_ofl){
    int32_t lr = kv.first.first;
    size_t idx = kv.first.second;
    T val = kv.second;
    decoded_cnt[idx]+=val<<accum_bits[lr];
  }
  // get rsz
  for(int32_t lr=1;lr<no_layer;++lr){
    rsz += bucket_ptr->getUnusedNum(lr)*(width_cnt[lr]+tag_len);
  }
}

template <int32_t no_layer, typename T>
size_t Brick<no_layer, T>::bsize() const{
  size_t tag_len = ceil(log2(gNum))+1;
  return bucket_ptr->bits_num(tag_len)/8;
}

template <int32_t no_layer, typename T>
size_t Brick<no_layer, T>::csize(const std::vector<size_t>& idxs) const{
  size_t result = 0;
  for(auto ori_index:idxs){
    result+=cnt_size[ori_index];
  }
  return result;
}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::dumpOfIdx(std::ostream& os) const{
  std::vector<size_t> ofNum(no_layer, 0);
  std::vector<std::set<size_t>> index_sets(no_layer);
  for(auto kv: report_ofl){
    int32_t lr = kv.first.first;
    size_t idx = kv.first.second;
    index_sets[lr].insert(idx);
  }
  for(int32_t lr = 0;lr<no_layer;++lr){
    os << "layer " << lr << ": ";
    for(auto idx:index_sets[lr]){
      os << idx << ' ';
    }
    os << std::endl;
  }
}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::dumpFreeCnt(std::ostream& os) const{
  for(int32_t lr = 0;lr<no_layer;++lr){
    os << "Unused segments in layer " << lr << ": " << bucket_ptr->getUnusedNum(lr) << std::endl;
  }
}

}// end of namespace Counter

#undef DEBUG_BRICK
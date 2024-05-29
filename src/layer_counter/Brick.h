/**
 * @file Brick.h
 * @author hc (you@domain.com)
 * @brief Counter type for Brick
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#define DEBUG_BRICK
#include <common/utils.h>
#include <common/layer.h>
#include <numeric>
#include <vector>
#include <iostream>

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
   * @brief counters in BS
   *
   */
  std::vector<Util::DynamicIntX<T>> cnt_array[no_layer];
  /**
   * @brief Status bits
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
   * @brief The pointer to corresponding full-box. (Valid when `overflow` is true)
   * 
   * @note Hardware implementation should use a few bits as index to full-box array.
   * Also here we do not implement copy on update.
   * 
   */
  T* full_box;
  /**
   * @brief Whether the bucket has overflowed
   * 
   */
  bool overflow;

  /**
   * @brief Update an inner counter
   *
   * @param layer The current layer
   * @param index The index of the updated counter in that layer
   * @param val Value to be updated
   * 
   * @return The overflow value not handled in migration
   */
  T updateSegment(const int32_t layer, const size_t index, const T val);

  /**
   * @brief Migrate bucket to full_box
   * 
   */
  void migrate();
  /**
   * @brief Query a counter and the number of layers of this counter
   * 
   * @note Even if a full_box is used, this function will still query the original bucket
   * 
   * @return std::pair<T, int32_t> pr.first is the value, pr.second is the layer number
   */
  std::pair<T, int32_t> query_with_layer(size_t index);

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
    if(full_box){
      delete[] full_box;
    }
  }
  /**
   * @brief Update a counter
   *
   * @param index Serialized index of a counter in this bucket
   * @param val   Value to be updated
   *
   * @note
   * - An out-of-range exception would be thrown if `index` is out of range.
   */
  void update(size_t index, T val);
  /**
   * @brief Clear the counter
   * 
   */
  void clear_cnt(size_t index);
  /**
   * @brief Query a counter online
   * 
   * @param index Serialized index of a counter in this bucket
   * 
   */
  T query(size_t index);
  /**
   * @brief Decode the counters and store the results into inner vector
   * 
   */
  void decode();
  /**
   * @brief Get the value of a counter offline. This should be called after decoding.
   * 
   */
  T getCnt(size_t index) const{
    return decoded_cnt.at(index);
  }

  /**
   * @brief Get the value of the original counter, i.e. the ground truth.
   * 
   */
  T getOriCnt(size_t index) const{
    return original_cnt.at(index);
  }

  /**
   * @brief Check `overflow`
   * 
   */
  bool isOverflow() const{
    return overflow;
  }

  /**
   * @brief Get the reference of a decoded counter. Should be used after decoding.
   * 
   * @param index Counter index
   * @return T& The counter reference
   */
  T& operator[](size_t index){
    return decoded_cnt[index];
  }

  /**
   * @brief Get the memory usage of this bucket in bytes.
   * 
   * @details There are four data structures that are taken into count:
   * 1. `cnt_array`: Bits are calculated as sum of no_cnt[i]*width_cnt[i] and then ceil to bytes. Because in 
   *     hardware implementation, we can arrange the counters in the same layer of all buckets as a register array. 
   *     What's more, we usually set no_cnt and width_cnt such that they can fit into several registers exactly.
   * 2. `status_bits`: Ceil to bytes.
   * 3. `overflow`: Count together with `status_bits`
   * 4. `full_box`: If used, add the size of this full counter array, because `cnt_array` already consumes memory.
   *(5.) Index bits used to point to the full_box array, cauculated as log(number of fullbox)*number of buckets.
   *     This can be only calculated by outer users of Buckets(ex. Brick) though.
   */
  size_t bsize() const;

  /**
   * @brief Get the memory usage of a specific counter, return in bits
   * 
   */
  size_t csize(size_t index) const{
    return size_cnt[index];
  }

  /**
   * @brief Get the redundant memory in bits. (Size of unused higher-layer counters and status-arrays)
   * 
   */
  size_t rsize() const{
    return rsz;
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
   * @brief Check the consistency between counters and ori_counters
   * 
   */
  bool validate() const{
    for(size_t i = 0; i < no_cnt[0]; ++i){
      if(getCnt(i)!=getOriCnt(i)){return false;}
    }
    return true;
  }
  /**
   * @brief Clear the counters
   * 
   */
  void clear();
};

/**
 * @brief Bucketized rank-indexed counter.
 * 
 * @tparam T Inner counter type (which should be numerical types).
 */
template <int32_t no_layer, typename T>
class Brick : public LayerCounter<no_layer, T> {
  /**
   * @brief Number of counters
   * 
   */
  size_t cNum;
  /**
   * @brief Number of Buckets
   * 
   */
  size_t bNum;
  /**
   * @brief Number of counters per bucket
   * 
   */
  size_t perBkt;
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
  std::vector<Bucket<no_layer, T>> buckets;

  Brick(const Brick &) = delete;
  Brick(Brick &&) = delete;
public:
  /**
   * @brief Construct Brick and initialize inner Buckets.
   * 
   * @param counter_num Number of counters you wish to use.
   * @param no_cnt Number of counters in each layer.
   * @param width_cnt Counter width of each layer.
   * 
   */
  Brick(size_t counter_num, const std::vector<size_t> &no_cnt,
        const std::vector<size_t> &width_cnt){
    initBucket(counter_num, no_cnt, width_cnt);
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
   * @param no_cnt Number of counters in each layer.
   * @param width_cnt Counter width of each layer.
   * 
   */
  void initBucket(size_t counter_num, const std::vector<size_t> &no_cnt,
        const std::vector<size_t> &width_cnt);

  /**
   * @brief Update a counter
   * 
   * @param ori_index Counter index
   * @param val Value to be added
   */
  void update(size_t ori_index, T val) override{
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    buckets.at(bktIdx).update(cIdx, val);
  }

  /**
   * @brief Clear counter
   * 
   */
  void clear_cnt(size_t ori_index) override{
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    buckets.at(bktIdx).clear_cnt(cIdx);
  }

  /**
   * @brief Decode all Buckets
   * 
   */
  void decode();
  /**
   * @brief Query a counter online
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T query(size_t ori_index) override{
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).query(cIdx);
  }
  /**
   * @brief Get the value of a counter offline. Should be used after decoding
   * 
   * @param ori_index Counter index
   * @return The counter value
   */
  T getCnt(size_t ori_index) const override{
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).getCnt(cIdx);
  }

  /**
   * @brief Get the value of the original counter, i.e. the ground truth.
   * 
   */
  T getOriCnt(size_t ori_index) const{
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).getOriCnt(cIdx);
  }
  /**
   * @brief Get the reference of a decoded counter. Should be used after decoding
   * 
   * @param ori_index Counter index
   * @return T& The counter reference
   */
  T& operator[](size_t ori_index){
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx)[cIdx];
  }
  /**
   * @brief Get memory consumption of this Brick in bytes
   * 
   * @details Need to add index bits used to point to the full_box array.
   * 
   */
  size_t bsize() const;

  size_t rsize() const{
    return rsz;
  }

  size_t csize(const std::vector<size_t>& idxs) const override;

  /**
   * @brief Get the number of overflow buckets
   * 
   */
  size_t getOfNum() const;
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
    for (size_t i = 0; i < bNum; i++){
      buckets[i].dumpCnt(os);
    }
  }
  /**
   * @brief Dump original counters to ostream
   * 
   */
  void dumpOri(std::ostream& os) const{
    for (size_t i = 0; i < bNum; i++){
      buckets[i].dumpOri(os);
    }
  }
  /**
   * @brief Check the consistency between counters and ori_counters
   * 
   */
  void validate() const{
    for (size_t i = 0; i < bNum; i++){
      if(!buckets[i].validate()){
        std::cerr << "Inconsistency of bucket counters found!" << std::endl;
        break;
      }
    }
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
  }
};

} //namespace Ominisketch
namespace OmniSketch::Counter {

template <int32_t no_layer, typename T>
Bucket<no_layer, T>::Bucket(
      const std::vector<size_t> &no_cnt,
      const std::vector<size_t> &width_cnt)
      : no_cnt(no_cnt), width_cnt(width_cnt), full_box(0), overflow(false){
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
  for (int32_t i = 0; i < no_layer-1; ++i) {
    status_bits[i] = std::vector<bool>(no_cnt[i], false);
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
T Bucket<no_layer, T>::updateSegment(const int32_t layer, const size_t index, const T val){
  assert(!overflow);
  T c_overflow = cnt_array[layer][index] + val;
  if (c_overflow) {
    if(layer==no_layer-1){ // last layer should not overflow
      throw std::overflow_error(
          "Counter overflow at the last layer in Bucket, overflow by " +
          std::to_string(c_overflow) + ".");
    }
    if(status_bits[layer][index]){
      // get index at next layer
      size_t next_index = 0;
      for(size_t i = 0;i<index;++i){
        next_index+=status_bits[layer][i];
      }
      T u_overflow = updateSegment(layer+1, next_index, c_overflow);
      return u_overflow << width_cnt[layer];
    } else {
      // 1. get index at next layer
      size_t next_index = 0;
      for(size_t i = 0;i<index;++i){
        next_index+=status_bits[layer][i];
      }
      // 2. check if overflowed
      size_t num_used = next_index;
      for(size_t i = index+1;i<no_cnt[layer];++i){
        num_used+=status_bits[layer][i];
      }
      if(num_used==no_cnt[layer+1]){ // bucket overflow
        overflow = true;
        migrate();
        return c_overflow << width_cnt[layer];
      }
      // 3. allocate a next-layer counter by moving counters backwards
      status_bits[layer][index] = true;
      for(size_t i = no_cnt[layer+1]-1;i>next_index;--i){
        cnt_array[layer+1][i] = cnt_array[layer+1][i-1];
      }
      cnt_array[layer+1][next_index].reset();
      // 4. Also need to move the status bits. So in hardware inplmentation, we can 
      // consider putting status bits and counters together, so one shift is enough.
      // However, this technique violates the hypothesis that status bits are in one word.
      // Also, popcount operation needs that status bits are stored continiously.
      if(layer+1<no_layer-1){
        for(size_t i = no_cnt[layer+1]-1;i>next_index;--i){
          status_bits[layer+1][i] = status_bits[layer+1][i-1];
        }
        status_bits[layer+1][next_index] = false;
      }
      // 5. update next layer
      T u_overflow = updateSegment(layer+1, next_index, c_overflow);
      return u_overflow << width_cnt[layer];
    }
  }
  return static_cast<T>(0);
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::migrate(){
  assert(overflow && !full_box);
  full_box = new T[no_cnt[0]];
  T* tmp_box = new T[no_cnt[0]];
  std::fill_n(full_box, no_cnt[0], 0);
  std::fill_n(tmp_box, no_cnt[0], 0);
  // decode all the counters layer by layer
  for (size_t j = 0;j<no_cnt[no_layer-1];++j){
    full_box[j] = cnt_array[no_layer-1][j].getVal();
  }
  for (int32_t i = no_layer-2; i >= 0; --i){
    size_t cur_idx = 0;
    for (size_t j = 0;j<no_cnt[i];++j){
      tmp_box[j] = cnt_array[i][j].getVal();
      if(status_bits[i][j]){
        tmp_box[j] += full_box[cur_idx]<<width_cnt[i];
        cur_idx+=1;
      }
    }
    std::copy_n(tmp_box, no_cnt[i], full_box);
  }
  delete[] tmp_box;
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::update(size_t index, T val){
  if (index >= no_cnt[0]) {
    throw std::out_of_range("Index Out of Range: Should be in [0, " +
                            std::to_string(no_cnt[0] - 1) + "], but got " +
                            std::to_string(index) + " instead.");
  }
  original_cnt[index]+=val;
  if(overflow){
    full_box[index]+=val;
  }else{
    T ret = updateSegment(0, index, val);
    if(ret){
      assert(overflow && full_box);
      full_box[index]+=ret;
    }
  }
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::clear_cnt(size_t index){
  if (index >= no_cnt[0]) {
    throw std::out_of_range("Index Out of Range: Should be in [0, " +
                            std::to_string(no_cnt[0] - 1) + "], but got " +
                            std::to_string(index) + " instead.");
  }
  original_cnt[index] = 0;
  if(overflow){
    full_box[index] = 0;
  }else{
    size_t next_index = index;
    cnt_array[0][next_index].reset();
    for(int32_t lr = 0;lr<no_layer-1;++lr){
      if(status_bits[lr][next_index]){ // overflow
        status_bits[lr][next_index] = false;
        // get index at next layer
        next_index = 0;
        for(size_t i = 0;i<index;++i){
          next_index+=status_bits[lr][i];
        }
        // shift the counters
        for(size_t i = next_index;i<no_cnt[lr+1]-1;++i){
          cnt_array[lr+1][i] = cnt_array[lr+1][i+1];
        }
        cnt_array[lr+1][no_cnt[lr+1]-1].reset();
      } else {
        break;
      }
    }
  }
}

template <int32_t no_layer, typename T>
T Bucket<no_layer, T>::query(size_t index){
  if(overflow){
    return full_box[index];
  } else {
    return query_with_layer(index).first;
  }
}

template <int32_t no_layer, typename T>
std::pair<T, int32_t> Bucket<no_layer, T>::query_with_layer(size_t index){
  size_t cur_bits = 0;
  T result = cnt_array[0][index].getVal();
  int32_t lr;
  for (lr = 1; lr < no_layer; lr++){
    if(!status_bits[lr-1][index]){break;} // no overflow to this layer
    cur_bits+=width_cnt[lr-1];
    T next_index = 0;
    for (size_t j = 0; j < index; j++){
      next_index+=status_bits[lr-1][j];
    }
    index = next_index;
    result+=cnt_array[lr][index].getVal()<<cur_bits;
  }
  return std::make_pair(result, lr);
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::decode(){
  std::vector<size_t> accum_bits(no_layer);
  accum_bits[0] = width_cnt[0];
  for(int32_t lr=1;lr<no_layer;++lr){
    accum_bits[lr] = accum_bits[lr-1]+width_cnt[lr];
  }
  // decode values and get size_cnt
  if(overflow){
    std::copy_n(full_box, no_cnt[0], decoded_cnt.begin());
    std::fill_n(size_cnt.begin(), no_cnt[0], accum_bits[no_layer-1]);
  }
  for(size_t i = 0;i<no_cnt[0];++i){
    std::pair<T, int32_t> pr = query_with_layer(i);
    if(!overflow){
      decoded_cnt[i] = pr.first;
    }
    size_cnt[i] += accum_bits[pr.second-1] + std::min(pr.second, no_layer-1);
  }
  // get rsz
  rsz = 1; // overflow bits
  for(int32_t lr=0;lr<no_layer-1;++lr){
    size_t used_num = 0;
    for(size_t i = 0;i<no_cnt[lr];++i){
      used_num+=status_bits[lr][i];
    }
    rsz += (no_cnt[lr+1]-used_num)*(width_cnt[lr+1]+1);
  }
}

template <int32_t no_layer, typename T>
size_t Bucket<no_layer, T>::bsize() const{
  size_t cbits = 0; // counter size in bits
  size_t sbits = 0; // status bits
  constexpr size_t obits = 1; // overflow
  size_t fbits = 0; // full_box bits
  size_t max_width = 0;
  for (size_t i = 0; i < no_layer; i++){
    cbits+=no_cnt[i]*width_cnt[i];
    sbits+=no_cnt[i];
    max_width+=width_cnt[i];
  }
  sbits-=no_cnt[no_layer-1];
  if(overflow){
    fbits = no_cnt[0]*max_width;
  }
  return (cbits+7)/8+(sbits+obits+7)/8+(fbits+7)/8;
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::dumpCnt(std::ostream& os) const{
  for(auto i: decoded_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::dumpOri(std::ostream& os) const{
  for(auto i: original_cnt){
    os << i << ' ';
  }
  os << std::endl;
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::clear(){
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
  if(overflow){
    delete[] full_box;
    full_box = nullptr;
    overflow = false;
  }
}



template <int32_t no_layer, typename T>
void Brick<no_layer, T>::initBucket(
    size_t counter_num, const std::vector<size_t> &no_cnt,
    const std::vector<size_t> &width_cnt){
  cNum = counter_num;
  perBkt = no_cnt[0];
  bNum = (counter_num+perBkt-1)/perBkt;
  rsz = 0;
  int32_t candidate = 31;
  int32_t cNum32 = static_cast<int32_t>(cNum);
  while(true){
    if(Util::IsCoprime(candidate, cNum32)){
      pseed = static_cast<size_t>(candidate);
      iseed = static_cast<size_t>(Util::MulInverse(candidate, cNum32));
      break;
    }else{candidate++;}
  }
  for(size_t i = 0;i<bNum;++i){
    buckets.push_back(Bucket<no_layer, T>{no_cnt, width_cnt});
  }
}

template <int32_t no_layer, typename T>
size_t Brick<no_layer, T>::getOfNum() const{
  size_t ofNum = 0;
  for (size_t i = 0; i < bNum; i++){
    ofNum+=buckets[i].isOverflow();
  }
  return ofNum;
}

template <int32_t no_layer, typename T>
void Brick<no_layer, T>::decode(){
  rsz = 0;
  for (size_t i = 0; i < bNum; i++){
    buckets[i].decode();
    rsz+=buckets[i].rsize();
  }
  // count the number of overflow buckets to get the minimum bits needed for full_box index.
  size_t ofNum = getOfNum();
  double ofbits_d = log2(static_cast<double>(ofNum+1));
  size_t ofbits = static_cast<size_t>(ceil(ofbits_d));
  rsz += bNum*ofbits;
}

template <int32_t no_layer, typename T>
size_t Brick<no_layer, T>::bsize() const{
  size_t bytes = 0;
  for (size_t i = 0; i < bNum; i++){
    bytes+=buckets[i].bsize();
  }
  // count the number of overflow buckets to get the minimum bits needed for full_box index.
  size_t ofNum = getOfNum();
  double ofbits_d = log2(static_cast<double>(ofNum+1));
  size_t ofbits = static_cast<size_t>(ceil(ofbits_d));
  bytes += (bNum*ofbits+7)/8;
  return bytes;
}

template <int32_t no_layer, typename T>
size_t Brick<no_layer, T>::csize(const std::vector<size_t>& idxs) const{
  size_t num = idxs.size();
  size_t result = num*rsz/cNum;
  for(auto ori_index:idxs){
    size_t index = (ori_index*pseed)%cNum;
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    result += buckets.at(bktIdx).csize(cIdx);
  }
  return result;
}


}// end of namespace Counter

#undef DEBUG_BRICK
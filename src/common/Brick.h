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
#include "utils.h"
#include <numeric>
#include <vector>

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
   * @brief Status array
   *
   */
  std::vector<size_t> status_array[no_layer];
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
   * @brief Query a counter online
   * 
   * @param index Serialized index of a counter in this bucket
   * 
   */
  T query(size_t index);
  /**
   * @brief Decode the counters and store the results into inner vector. 
   * Also get the true length of each counter and fill it into `size_cnt`. 
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
class Brick {
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
   * @param index Counter index
   * @param val Value to be added
   */
  void update(size_t index, T val){
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    buckets.at(bktIdx).update(cIdx, val);
  }

  /**
   * @brief Decode all Buckets
   * 
   */
  void decode(){
    for (size_t i = 0; i < bNum; i++){
      buckets[i].decode();
      rsz+=buckets[i].rsize();
    }
  }

  /**
   * @brief Query a counter online
   * 
   * @param index Counter index
   * @return The counter value
   */
  T query(size_t index){
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).query(cIdx);
  }
  /**
   * @brief Get the value of a counter offline. Should be used after decoding
   * 
   * @param index Counter index
   * @return The counter value
   */
  T getCnt(size_t index) const{
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).getCnt(cIdx);
  }

  /**
   * @brief Get the value of the original counter, i.e. the ground truth.
   * 
   */
  T getOriCnt(size_t index) const{
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    return buckets.at(bktIdx).getOriCnt(cIdx);
  }
  /**
   * @brief Get the reference of a decoded counter. Should be used after decoding
   * 
   * @param index Counter index
   * @return T& The counter reference
   */
  T& operator[](size_t index){
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
  /**
   * @brief Get the memory usage of a specific counter, return in bits
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
      : no_cnt(no_cnt), width_cnt(width_cnt), full_box(0), rsz(0), overflow(false){
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
    status_array[i] = std::vector<size_t>(no_cnt[i], no_cnt[i-1]);
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
    // 1. find an previously allocated counter
    for(size_t nxt = 0;nxt<no_cnt[layer+1];++nxt){
      if(status_array[layer+1][nxt]==index){
        T u_overflow = updateSegment(layer+1, nxt, c_overflow);
        return u_overflow << width_cnt[layer];
      }
    }
    // 2. allocate a counter
    for(size_t nxt = 0;nxt<no_cnt[layer+1];++nxt){
      if(status_array[layer+1][nxt]==no_cnt[layer]){
        status_array[layer+1][nxt] = index;
        T u_overflow = updateSegment(layer+1, nxt, c_overflow);
        return u_overflow << width_cnt[layer];
      }
    }
    // 3. no counter available, overflow
    overflow = true;
    migrate();
    return c_overflow << width_cnt[layer];
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
  for (int32_t i = no_layer-1; i > 0; --i){
    // copy the values of lower layer counter
    for (size_t j = 0;j<no_cnt[i-1];++j){
      tmp_box[j] = cnt_array[i-1][j].getVal();
    }
    // add the values from the upper layer to the lower layer
    for (size_t j = 0;j<no_cnt[i];++j){
      if(status_array[i][j]<no_cnt[i-1]){
        tmp_box[status_array[i][j]] += full_box[j]<<width_cnt[i-1];
      }
    }
    std::copy_n(tmp_box, no_cnt[i-1], full_box);
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
T Bucket<no_layer, T>::query(size_t index){
  if(overflow){
    return full_box[index];
  } else {
    size_t cur_bits = 0;
    size_t cur_idx = index;
    T result = cnt_array[0][index].getVal();
    for (size_t i = 1; i < no_layer; i++){
      cur_bits+=width_cnt[i-1];
      bool oflw = false;
      for (size_t j = 0;j < no_cnt[i]; j++){
        if(status_array[i][j]==cur_idx){
          oflw = true;
          cur_idx = j;
          result+=cnt_array[i][j].getVal() << cur_bits;
          break;
        }
      }
      if(!oflw){break;}
    }
    return result;
  }
}

template <int32_t no_layer, typename T>
void Bucket<no_layer, T>::decode(){
  // scan the counters layer by layer(bottom-up)
  for(size_t i = 0;i<no_cnt[0];++i){
    decoded_cnt[i] = cnt_array[0][i].getVal();
    size_cnt[i] = width_cnt[0];
  }
  size_t cur_bits = width_cnt[0];// we'll need to shift this much bits in the end
  for(size_t lr = 1;lr<no_layer;++lr){
    for(size_t j = 0;j<no_cnt[lr];++j){
      if(status_array[lr][j]<no_cnt[lr-1]){// a counter at layer `lr` has been allocated
        size_t idx = status_array[lr][j];
        for(size_t dw = lr-1;dw!=0;--dw){// down to the lowest layer to get the index
          idx = status_array[dw][idx]; // lower counters must have been allocated
        }
        decoded_cnt[idx] += cnt_array[lr][j].getVal() << cur_bits;
        size_cnt[idx] += width_cnt[lr]+static_cast<size_t>(ceil(log2(no_cnt[lr-1]+1)));
      } else {
        rsz += width_cnt[lr]+static_cast<size_t>(ceil(log2(no_cnt[lr-1]+1)));
      }
    }
    cur_bits+=width_cnt[lr];
  }
  if(overflow){
    std::copy_n(full_box, no_cnt[0], decoded_cnt.begin());
    for(size_t i = 0;i<no_cnt[0];++i){
      size_cnt[i]+=cur_bits;
    }
  }
}

template <int32_t no_layer, typename T>
size_t Bucket<no_layer, T>::bsize() const{
  size_t cbits = 0; // counter size in bits
  size_t sbits = 0; // status bits
  constexpr size_t obits = 1; // overflow
  size_t fbits = 0; // full_box bits
  size_t max_width = 0;
  cbits += no_cnt[0]*width_cnt[0];
  max_width += width_cnt[0];
  for (size_t i = 1; i < no_layer; i++){
    cbits+=no_cnt[i]*width_cnt[i];
    sbits+=no_cnt[i]*static_cast<size_t>(ceil(log2(no_cnt[i-1]+1)));
    max_width+=width_cnt[i];
  }
  if(overflow){
    fbits = no_cnt[0]*max_width;
  }
  //std::cout << cbits << ' '<< sbits << ' ' << fbits << std::endl;
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
  rsz = 0;
  for (int32_t i = 0; i < no_layer; ++i) {
    cnt_array[i] = std::vector<Util::DynamicIntX<T>>(no_cnt[i], {width_cnt[i]});
  }
  for (int32_t i = 1; i < no_layer; ++i) {
    status_array[i] = std::vector<size_t>(no_cnt[i], no_cnt[i-1]);
  }
  std::fill_n(original_cnt.begin(), no_cnt[0], 0);
  std::fill_n(decoded_cnt.begin(), no_cnt[0], 0);
  std::fill_n(size_cnt.begin(), no_cnt[0], 0);
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
  size_t result = 0;
  for(auto index:idxs){
    size_t bktIdx = index/perBkt;
    size_t cIdx = index%perBkt;
    result+=buckets[bktIdx].csize(cIdx);
  }
  return result;
}

}// end of namespace Counter

#undef DEBUG_BRICK
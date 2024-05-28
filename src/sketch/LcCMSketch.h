/**
 * @file LcCMSketch.h
 * @author hc (you@domain.com)
 * @brief Implementation of Count Min Sketch with Layer counters
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "common/hash.h"
#include <common/sketch.h>
#include <common/layer.h>

namespace OmniSketch::Sketch {
/**
 * @brief Count Min Sketch
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class LcCMSketch : public SketchBase<key_len, T> {
private:
  int32_t depth;
  int32_t width;
  const int32_t offset;
  hash_t *hash_fns;
  Counter::LayerCounter<no_layer, T>& counter;

  LcCMSketch(const LcCMSketch &) = delete;
  LcCMSketch(LcCMSketch &&) = delete;

public:
  /**
   * @brief Construct by specifying depth and width
   * @param width_ should be prime number to reduce hash collision
   *
   */
  LcCMSketch(int32_t depth_, int32_t width_, int32_t _offset, Counter::LayerCounter<no_layer, T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~LcCMSketch();
  /**
   * @brief Update a flowkey with certain value
   *
   */
  void update(const FlowKey<key_len> &flowkey, T val) override;
  /**
   * @brief Query a flowkey
   *
   */
  T query(const FlowKey<key_len> &flowkey) const override;
  /**
   * @brief Get the size of the sketch
   *
   */
  size_t size() const override;
  size_t cntNum() const override;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
LcCMSketch<key_len, no_layer, T, hash_t>::LcCMSketch(int32_t depth_, int32_t width_, int32_t _offset, Counter::LayerCounter<no_layer, T>& counter_)
    : depth(depth_), width(Util::NextPrime(width_)), counter(counter_), offset(_offset){
  hash_fns = new hash_t[depth];
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
LcCMSketch<key_len, no_layer, T, hash_t>::~LcCMSketch()
{
    delete[] hash_fns;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void LcCMSketch<key_len, no_layer, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                          T val) {
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    counter.update(index, val);
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T LcCMSketch<key_len, no_layer, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    min_val = std::min(min_val, counter.getCnt(index));
  }
  return min_val;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LcCMSketch<key_len, no_layer, T, hash_t>::size() const {
  std::vector<size_t> idxs(depth * width);
  for(size_t i = 0;i < depth * width;++i){
    idxs[i]=i+offset;
  }
  return sizeof(*this)                // instance
         + sizeof(hash_t) * depth     // hashing class
         + counter.csize(idxs)/8;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LcCMSketch<key_len, no_layer, T, hash_t>::cntNum() const {
  return depth * width;
}

} // namespace OmniSketch::Sketch

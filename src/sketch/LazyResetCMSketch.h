/**
 * @file LazyResetCMSketch.h
 * @author hc (you@domain.com)
 * @brief Implementation of Count Min Sketch with Lazy Reset Layer counters
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
class LazyResetCMSketch : public SketchBase<key_len, T> {
private:
  short sketch_idx;
  int32_t chunk_size;
  int32_t depth;
  int32_t width;
  const int32_t offset;
  hash_t *hash_fns;
  std::vector<unsigned short>& chunk_tags;
  Counter::LayerCounter<no_layer, T>& counter;

  LazyResetCMSketch(const LazyResetCMSketch &) = delete;
  LazyResetCMSketch(LazyResetCMSketch &&) = delete;

public:
  /**
   * @brief Construct by specifying depth and width
   * @param width_ should be prime number to reduce hash collision
   *
   */
  LazyResetCMSketch(short sketch_idx, int32_t chunk_size, int32_t depth_, int32_t width_, int32_t _offset, std::vector<unsigned short>& _chunk_tags, Counter::LayerCounter<no_layer, T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~LazyResetCMSketch();
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
LazyResetCMSketch<key_len, no_layer, T, hash_t>::LazyResetCMSketch(short sketch_idx, int32_t chunk_size,
  int32_t depth_, int32_t width_, int32_t _offset, std::vector<unsigned short>& _chunk_tags, Counter::LayerCounter<no_layer, T>& counter_)
    : sketch_idx(sketch_idx), chunk_size(chunk_size), depth(depth_), width(Util::NextPrime(width_)), 
      counter(counter_), chunk_tags(_chunk_tags), offset(_offset){
  hash_fns = new hash_t[depth];
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
LazyResetCMSketch<key_len, no_layer, T, hash_t>::~LazyResetCMSketch()
{
    delete[] hash_fns;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void LazyResetCMSketch<key_len, no_layer, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                          T val) {
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    // lazy reset
    int32_t chunk_idx = index/chunk_size;
    if (chunk_tags[chunk_idx] != sketch_idx) {
      chunk_tags[chunk_idx] = sketch_idx;
      for (int32_t j = chunk_idx*chunk_size; j < (chunk_idx+1)*chunk_size; ++j) {
        counter.clear_cnt(j);
      }
    }
    counter.update(index, val);
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T LazyResetCMSketch<key_len, no_layer, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < depth; ++i) {
    int32_t index = hash_fns[i](flowkey) % width + i*width + offset;
    // lazy reset
    int32_t chunk_idx = index/chunk_size;
    if (chunk_tags[chunk_idx] != sketch_idx) {
      min_val = std::min(min_val, (T)0);
    } else {
      min_val = std::min(min_val, counter.getCnt(index));
    }
  }
  return min_val;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LazyResetCMSketch<key_len, no_layer, T, hash_t>::size() const {
  std::vector<size_t> idxs(depth * width);
  for(size_t i = 0;i < depth * width;++i){
    idxs[i]=i+offset;
  }
  return sizeof(*this)                // instance
         + sizeof(hash_t) * depth     // hashing class
         + counter.csize(idxs)/8;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LazyResetCMSketch<key_len, no_layer, T, hash_t>::cntNum() const {
  return depth * width;
}

} // namespace OmniSketch::Sketch

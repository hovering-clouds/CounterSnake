/**
 * @file BrickMVSketch.h
 * @author hc
 * @brief Implementation of MVSketch with Brick
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <algorithm>
#include <map>
#include <vector>

#include <common/hash.h>
#include <common/sketch.h>
#include <common/utils.h>
#include <common/Brick.h>

namespace OmniSketch::Sketch {
template <int32_t key_len, int32_t no_layer, typename T, typename hash_t> 
class BrickMVSketch : public SketchBase<key_len, T> {
  int32_t depth_;
  int32_t width_;
  const int32_t offset;
  Counter::Brick<no_layer, T>& counter;


  hash_t *hash_fns_;

  struct Bounds {
    T lower;
    T upper;
  };

  struct Bucket {
    //T V;
    FlowKey<key_len> K;
    //T C;
  };

  Bucket **bkt;

public:
  BrickMVSketch(int32_t depth, int32_t width, int32_t _offset, Counter::Brick<no_layer, T>& counter_);
  BrickMVSketch(BrickMVSketch &&) = delete;
  ~BrickMVSketch();
  BrickMVSketch &operator=(const BrickMVSketch &) = delete;
  BrickMVSketch &operator=(BrickMVSketch &&) = delete;

  void update(const FlowKey<key_len> &flow_key, T val);
  T query(const FlowKey<key_len> &flow_key) const;

  void clear();
  size_t size() const override;
  size_t cntNum() const override;

  Bounds queryBounds(const FlowKey<key_len> &flow_key) const;
  Data::Estimation<key_len, T>  getHeavyHitter(double threshold) const;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
BrickMVSketch<key_len, no_layer, T, hash_t>::BrickMVSketch(int32_t depth,
    int32_t width, int32_t _offset, Counter::Brick<no_layer, T>& counter_)
    : depth_(depth), width_(Util::NextPrime(width)), counter(counter_), offset(_offset) {
  hash_fns_ = new hash_t[depth_];

  // Allocate continuous memory
  bkt = new Bucket *[depth_];
  bkt[0] = new Bucket[depth_ * width_]();
  for (int i = 1; i < depth_; ++i)
    bkt[i] = bkt[i - 1] + width_;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
BrickMVSketch<key_len, no_layer, T, hash_t>::~BrickMVSketch() {
  delete[] hash_fns_;

  delete[] bkt[0];
  delete[] bkt;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickMVSketch<key_len, no_layer, T, hash_t>::update(const FlowKey<key_len> &flow_key,
                                          T val) {
  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    size_t v_idx = 2*(i*width_+index)+offset;
    counter.update(v_idx, val); //bkt[i][index].V += val;
    if (bkt[i][index].K == flow_key)
      counter.update(v_idx+1, val); //bkt[i][index].C += val;
    else {
      T cur_val = counter.query(v_idx+1); //bkt[i][index].C -= val;
      if (cur_val < val) {
        bkt[i][index].K = flow_key;
        counter.update(v_idx+1, val-2*cur_val); //bkt[i][index].C *= -1;
      } else {
        counter.update(v_idx+1, -val);
      }
    }
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T BrickMVSketch<key_len, no_layer, T, hash_t>::query(const FlowKey<key_len> &flow_key) const {
  std::vector<T> S_cap(depth_);

  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    size_t v_idx = 2*(i*width_+index)+offset;
    T v_val = counter.query(v_idx);
    T c_val = counter.query(v_idx+1);
    if (bkt[i][index].K == flow_key)
      S_cap[i] = (v_val + c_val) / 2;
    else
      S_cap[i] = (v_val - c_val) / 2;
  }

  return *min_element(S_cap.begin(), S_cap.end());
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickMVSketch<key_len, no_layer, T, hash_t>::clear() {
  std::fill(bkt[0], bkt[0] + depth_ * width_, 0);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t BrickMVSketch<key_len, no_layer, T, hash_t>::size() const {
  std::vector<size_t> idxs(2*depth_*width_);
  for(size_t i = 0;i < 2*depth_*width_;++i){
    idxs[i]=i+offset;
  }
  return sizeof(*this) + // Instance
         depth_ * sizeof(hash_t) +              // hash_fns
         sizeof(Bucket *) * depth_ +            // counter
         sizeof(FlowKey<key_len>) * depth_ * width_ +
         + counter.rsize()*(cntNum())/(8*counter.getcNum()) // redundant bits
         + counter.csize(idxs)/8;   
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t BrickMVSketch<key_len, no_layer, T, hash_t>::cntNum() const {
  return 2 * depth_ * width_;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
typename BrickMVSketch<key_len, no_layer, T, hash_t>::Bounds
BrickMVSketch<key_len, no_layer, T, hash_t>::queryBounds(
    const FlowKey<key_len> &flow_key) const {
  std::vector<T> L(depth_);

  for (int i = 0; i < depth_; ++i) {
    int index = hash_fns_[i](flow_key) % width_;
    size_t v_idx = 2*(i*width_+index)+offset;
    L[i] = bkt[i][index].K == flow_key ? counter.query(v_idx+1) : 0;
  }

  return {*max_element(L.begin(), L.end()), query(flow_key)};
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
Data::Estimation<key_len, T> 
BrickMVSketch<key_len, no_layer, T, hash_t>::getHeavyHitter(double threshold) const {
  Data::Estimation<key_len, T> heavy_hitters;
  std::set<FlowKey<key_len>> heavy_set;

  for (int i = 0; i < depth_; ++i)
    for (int j = 0; j < width_; ++j) {
      size_t v_idx = 2*(i*width_+j)+offset;
      if (counter.query(v_idx) < threshold)
        continue;

      const FlowKey<key_len> &flow_key = bkt[i][j].K;
      if (heavy_set.count(flow_key))
        continue;

      T S_cap = query(flow_key);
      if (S_cap >= threshold){
        heavy_set.insert(flow_key);
        heavy_hitters[flow_key] = S_cap;
      }
    }

  return heavy_hitters;
}

} // namespace OmniSketch::Sketch

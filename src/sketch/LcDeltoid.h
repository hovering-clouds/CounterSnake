/**
 * @file LcDeltoid.h
 * @author deadlycat <lsmfttb@gmail.com>
 * XierLabber<yangshibo@stu.pku.edu.cn>(modified)
 * hc
 * @brief Implementation of Deltoid with Layer counters
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <common/hash.h>
#include <common/sketch.h>
#include <common/layer.h>
#include <iostream>
#include <vector>

namespace OmniSketch::Sketch {
/**
 * @brief LcDeltoid
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class LcDeltoid : public SketchBase<key_len, T> {
private:
  T sum_;
  int32_t num_hash_;
  int32_t num_group_;
  int32_t nbits_;
  const int32_t offset;
  Counter::LayerCounter<no_layer, T>& counter;
  hash_t *hash_fns_; // hash funcs

public:
  /**
   * @brief Construct by specifying hash number and group number
   *
   */
  LcDeltoid(int32_t num_hash, int32_t num_group, int32_t offset_, Counter::LayerCounter<no_layer, T>& counter_);
  /**
   * @brief Release the pointer
   *
   */
  ~LcDeltoid();
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
   * @brief Get all the heavy hitters
   *
   */
  Data::Estimation<key_len, T> getHeavyHitter(
      double threshold) const override; /**
                                         * @brief Get the size of the sketch
                                         *
                                         */
  size_t size() const override;
  /**
   * @brief Reset the sketch
   *
   */
  void clear();
  size_t cntNum() const override;
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of template methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
LcDeltoid<key_len, no_layer, T, hash_t>::LcDeltoid(int32_t num_hash, int32_t num_group, int32_t offset_, Counter::LayerCounter<no_layer, T>& counter_)
    : num_hash_(num_hash), num_group_(Util::NextPrime(num_group)),
      nbits_(key_len * 8), sum_(0), offset(offset_), counter(counter_) {
  hash_fns_ = new hash_t[num_hash_];
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
LcDeltoid<key_len, no_layer, T, hash_t>::~LcDeltoid() {
  delete[] hash_fns_;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void LcDeltoid<key_len, no_layer, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                         T val) {
  sum_ += val;
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    int32_t uidx = offset+(i*num_group_+idx)*(nbits_+1);
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        counter.update(uidx+j, val);
      }
    }
    counter.update(uidx+nbits_, val);
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T LcDeltoid<key_len, no_layer, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {

  static bool cnt_distrib = true;

  T min_val = std::numeric_limits<T>::max();
  for (int32_t i = 0; i < num_hash_; ++i) {
    int32_t idx = hash_fns_[i](flowkey) % num_group_;
    int32_t uidx = offset+(i*num_group_+idx)*(nbits_+1);
    for (int32_t j = 0; j < nbits_; ++j) {
      if (flowkey.getBit(j)) {
        min_val = std::min(min_val, counter.getCnt(uidx+j));
      } else {
        min_val = std::min(min_val, counter.getCnt(uidx+nbits_)-counter.query(uidx+j));
      }
    }
  }
  return min_val;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
Data::Estimation<key_len, T>
LcDeltoid<key_len, no_layer, T, hash_t>::getHeavyHitter(double threshold) const {
  T thresh = threshold;
  double val1 = 0;
  double val0 = 0;
  Data::Estimation<key_len, T> heavy_hitters;
  for (int32_t i = 0; i < num_hash_; i++) {
    for (int32_t j = 0; j < num_group_; j++) {
      int32_t uidx = offset+(i*num_group_+j)*(nbits_+1);
      if (counter.getCnt(uidx+nbits_) <= thresh) { // no heavy hitter in this group
        continue;
      }
      FlowKey<key_len> fk{}; // create a flowkey with full 0
      bool reject = false;
      for (int32_t k = 0; k < nbits_; k++) {
        T cnt1 = counter.getCnt(uidx+k);
        T cnt0 = counter.getCnt(uidx+nbits_)-cnt1;
        bool t1 = (cnt1 > thresh);
        bool t0 = (cnt0 > thresh);
        if (t1 == t0) {
          reject = true;
          break;
        }
        if (t1) {
          fk.setBit(k, true);
        }
      }
      if (reject) {
        continue;
      } else if (!heavy_hitters.count(fk)) {
        T esti_val = query(fk);
        heavy_hitters[fk] = esti_val;
      }
    }
  }
  return heavy_hitters;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LcDeltoid<key_len, no_layer, T, hash_t>::size() const {
  size_t cnt_num = cntNum();
  std::vector<size_t> idxs(cnt_num);
  for(size_t i = 0;i < cnt_num;++i){
    idxs[i]=i+offset;
  }
  return sizeof(LcDeltoid<key_len, no_layer, T, hash_t>) +
         num_hash_ * sizeof(hash_t) + 
         + counter.csize(idxs)/8;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void LcDeltoid<key_len, no_layer, T, hash_t>::clear() {
  sum_ = 0;
  //std::fill(arr0_[0][0], arr0_[0][0] + num_hash_ * num_group_ * nbits_, 0);
  //std::fill(arr1_[0][0], arr1_[0][0] + num_hash_ * num_group_ * (nbits_ + 1),
  //          0);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t LcDeltoid<key_len, no_layer, T, hash_t>::cntNum() const {
  return num_group_ * num_hash_ * (nbits_+1);
}


} // namespace OmniSketch::Sketch
/**
 * @file BrickElasticSketch.h
 * @author dromniscience XierLabber hc (you@domain.com)
 * @brief Implementation of Elastic Sketch with Brick
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <common/hash.h>
#include <common/sketch.h>
#include <common/Brick.h>

#include <sketch/BrickCMSketch.h>

#define JUDGE_IF_SWAP(min_val, guard_val)                                      \
  ((guard_val) > ((min_val) << 3)) // delta = 8

namespace OmniSketch::Sketch {
/**
 * @brief Elastic Sketch
 *
 * @tparam key_len  length of flowkey
 * @tparam T        type of the counter
 * @tparam hash_t   hashing class
 */
template <int32_t key_len, int32_t no_layer, typename T, typename hash_t = Hash::AwareHash>
class BrickElasticSketch : public SketchBase<key_len, T> {
private:
  struct Entry {
    FlowKey<key_len> flowkey_;
    bool flag_;
    Entry() : flowkey_(), flag_(false) {}
    bool isEmpty() {
      FlowKey<key_len> emptyKey{};
      return flowkey_ == emptyKey;
    }
  };
  // heavy part
  Entry **buckets_;
  int32_t num_buckets_;
  int32_t num_per_bucket_; // each bucket has num_per_bucket_ entries
  const int32_t offset;
  Counter::Brick<no_layer, T>& counter;

  hash_t hash_h_;
  // light part
  BrickCMSketch<key_len, no_layer, T, hash_t> cm_;

public:
  BrickElasticSketch(int32_t num_buckets, int32_t num_per_bucket, int32_t l_depth,
                int32_t l_width, int32_t _offset, Counter::Brick<no_layer, T>& counter_);
  ~BrickElasticSketch();

  int heavypartInsert(const FlowKey<key_len> &flowkey, T val,
                      FlowKey<key_len> &swap_key, T &swap_val);

  void lightpartInsert(const FlowKey<key_len> &flowkey, T val);
  void update(const FlowKey<key_len> &flowkey, T val);
  T heavypartQuery(const FlowKey<key_len> &flowkey, bool &flag) const;
  T lightpartQuery(const FlowKey<key_len> &flowkey) const;
  T query(const FlowKey<key_len> &flowkey) const;
  size_t size() const override;
  size_t cntNum() const override;
  void clear();
};

} // namespace OmniSketch::Sketch

//-----------------------------------------------------------------------------
//
///                        Implementation of templated methods
//
//-----------------------------------------------------------------------------

namespace OmniSketch::Sketch {

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
BrickElasticSketch<key_len, no_layer, T, hash_t>::BrickElasticSketch(int32_t num_buckets,
    int32_t num_per_bucket, int32_t l_depth, int32_t l_width, int32_t _offset, Counter::Brick<no_layer, T>& counter_)
    : num_buckets_(Util::NextPrime(num_buckets)), num_per_bucket_(num_per_bucket+1), offset(_offset), 
      cm_(l_depth, l_width, _offset+num_buckets_*num_per_bucket_, counter_), counter(counter_) {
  buckets_ = new Entry *[num_buckets_];
  buckets_[0] = new Entry[num_buckets_ * num_per_bucket_]();
  for (int i = 1; i < num_buckets_; ++i) {
    buckets_[i] = buckets_[i - 1] + num_per_bucket_;
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t BrickElasticSketch<key_len, no_layer, T, hash_t>::size() const {
  return sizeof(*this) 
         + (key_len + sizeof(T) + 0.125) * num_buckets_ * num_per_bucket_ 
         + cm_.size();
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
size_t BrickElasticSketch<key_len, no_layer, T, hash_t>::cntNum() const {
  return num_buckets_*num_per_bucket_ + cm_.cntNum();
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickElasticSketch<key_len, no_layer, T, hash_t>::clear() {
  cm_.clear();
  std::fill(buckets_[0], buckets_[0] + num_buckets_ * num_per_bucket_, 0);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
BrickElasticSketch<key_len, no_layer, T, hash_t>::~BrickElasticSketch() {
  delete[] buckets_[0];
  delete[] buckets_;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
int BrickElasticSketch<key_len, no_layer, T, hash_t>::heavypartInsert(
    const FlowKey<key_len> &flowkey, T val, FlowKey<key_len> &swap_key,
    T &swap_val) {

  int32_t index = hash_h_(flowkey) % num_buckets_;
  int32_t matched = -1;
  int32_t empty = -1;
  int32_t min_counter = -1;
  T min_counter_val = std::numeric_limits<T>::max();//buckets_[index][0].val_;
  for (int32_t i = 0; i < num_per_bucket_ - 1; ++i) {
    if (buckets_[index][i].flowkey_ == flowkey) { // flowkey is in the bucket
      matched = i;
      counter.update(index*num_per_bucket_+i+offset, val);//buckets_[index][i].val_ += val;
      return 0;
    }
    if (buckets_[index][i].isEmpty()) {
      buckets_[index][i].flowkey_ = flowkey;
      counter.update(index*num_per_bucket_+i+offset, val);//buckets_[index][i].val_ = val;
      return 0;
    }
    T new_val = counter.query(index*num_per_bucket_+i+offset);
    if (min_counter_val > new_val) {
      min_counter = i;
      min_counter_val = new_val;
    }
  }
  /* update guard val and comparison */
  size_t guard_idx = (index+1)*num_per_bucket_-1+offset;
  size_t min_idx = index*num_per_bucket_+min_counter+offset;
  //std::cout << guard_idx << ' ' << min_idx << std::endl;
  T guard_val = counter.query(guard_idx);//buckets_[index][num_per_bucket_ - 1].val_;
  guard_val += 1;

  if (!JUDGE_IF_SWAP(min_counter_val, guard_val)) {
    counter.update(guard_idx, 1); //buckets_[index][num_per_bucket_ - 1].val_ = guard_val;
    return 2;
  } else {
    swap_key = buckets_[index][min_counter].flowkey_;
    swap_val = counter.query(min_idx); //buckets_[index][min_counter].val_;
    counter.clear_cnt(guard_idx); //buckets_[index][num_per_bucket_ - 1].val_ = 0;
    buckets_[index][min_counter].flowkey_ = flowkey;
    counter.update(min_idx, val-swap_val); //buckets_[index][min_counter].val_ = val;
    buckets_[index][min_counter].flag_ = true;
    return 1;
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickElasticSketch<key_len, no_layer, T, hash_t>::lightpartInsert(
    const FlowKey<key_len> &flowkey, T val) {
  cm_.update(flowkey, val);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
void BrickElasticSketch<key_len, no_layer, T, hash_t>::update(const FlowKey<key_len> &flowkey,
                                          T val) {

  FlowKey<key_len> swap_key;
  T swap_val;
  int result = heavypartInsert(flowkey, val, swap_key, swap_val);
  switch (result) {
  case 0:
    return;
  case 1: {
    lightpartInsert(swap_key, swap_val);
    return;
  }
  case 2: {
    lightpartInsert(flowkey, val);
    return;
  }
  default:
    printf("error return value !\n");
    exit(1);
  }
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T BrickElasticSketch<key_len, no_layer, T, hash_t>::heavypartQuery(
    const FlowKey<key_len> &flowkey, bool &flag) const {
  int index = hash_h_(flowkey) % num_buckets_;
  for (int i = 0; i < num_per_bucket_ - 1; ++i) {
    if (buckets_[index][i].flowkey_ == flowkey) {
      flag = buckets_[index][i].flag_;
      return counter.query(index*num_per_bucket_+i+offset);//buckets_[index][i].val_;
    }
  }
  return 0;
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T BrickElasticSketch<key_len, no_layer, T, hash_t>::lightpartQuery(
    const FlowKey<key_len> &flowkey) const {
  return cm_.query(flowkey);
}

template <int32_t key_len, int32_t no_layer, typename T, typename hash_t>
T BrickElasticSketch<key_len, no_layer, T, hash_t>::query(const FlowKey<key_len> &flowkey) const {
  
  bool flag = false;
  T heavy_result = heavypartQuery(flowkey, flag);
  T light_result = 0;
  if (heavy_result == 0 || flag == true) {
    light_result = lightpartQuery(flowkey);
  }
  return heavy_result + light_result;
}

} // namespace OmniSketch::Sketch

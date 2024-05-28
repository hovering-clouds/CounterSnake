/**
 * @file layer.h
 * @author hc (you@domain.com)
 * @brief Abstact class for counter layer sharing
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "utils.h"
#include <numeric>
#include <vector>
#include <set>

namespace OmniSketch::Counter{
template <int32_t no_layer, typename T>
class LayerCounter {
public:
    /**
     * @brief Update a counter
     * 
     */
    virtual void update(size_t index, T val) = 0;
    /**
     * @brief Clear the counter (Reset to 0)
     * 
     */
    virtual void clear_cnt(size_t index) = 0;
    /**
     * @brief Query a counter online
     * 
     */
    virtual T query(size_t index) = 0;
    /**
     * @brief Get the value of a counter offline (after decoding)
     * 
     */
    virtual T getCnt(size_t index) const = 0;
    /**
     * @brief Get the memory usage of the given counters, return in bits
     * (This will count redundant bits in average)
     * 
     */
    virtual size_t csize(const std::vector<size_t>& idxs) const = 0;
};

}
#!/bin/bash

cd '/home/liu_xunpeng/CounterSnake/build'

# 遍历配置
suffixes=("all" "cm" "dt" "es" "fr" "hp" "mv" "pr" "sl")
for suffix in "${suffixes[@]}"; do
  config_file="../exp/vldb/exp6-multi/exp4-${suffix}.toml"

  # 检查配置文件是否存在
  if [[ -f "$config_file" ]]; then
    echo "Running: ./Dway -c $config_file"
    "./Dway" -c "$config_file"
  else
    echo "Warning: Config file '$config_file' not found, skipping."
  fi
done
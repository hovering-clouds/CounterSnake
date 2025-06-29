#!/bin/bash

cd '/home/liu_xunpeng/CounterSnake/exp/vldb/exp7-layer'

# 遍历配置
suffixes=("1" "2" "3" "4" "5" "6")
for suffix in "${suffixes[@]}"; do
  exec_path="./Dway_${suffix}"
  config_file="./dway_${suffix}.toml"

  # 检查配置文件是否存在
  if [[ -f "$config_file" ]]; then
    echo "Running: $exec_path -c $config_file"
    "$exec_path" -c "$config_file"
  else
    echo "Warning: Config file '$config_file' not found, skipping."
  fi
done
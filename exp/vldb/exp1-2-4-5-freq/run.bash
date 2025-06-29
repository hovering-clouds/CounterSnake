#!/bin/bash

# 参数检查
if [[ $# -ne 2 ]]; then
  echo "Usage: $0 /path/to/exefilename config_prefix"
  exit 1
fi

exe_path="$1"
config_prefix="$2"

# 检查可执行文件是否有效
if [[ ! -x "$exe_path" || ! -f "$exe_path" ]]; then
  echo "Error: '$exe_path' is not a valid executable file."
  exit 1
fi

# 遍历 1M 到 4M 的配置
suffixes=("100K" "200K" "400K" "800K" "1600K")
for suffix in "${suffixes[@]}"; do
  config_dir="../exp/vldb/exp1-2-4-5-freq/config_${suffix}"
  config_file="${config_dir}/${config_prefix}_${suffix}.toml"

  # 检查配置文件是否存在
  if [[ -f "$config_file" ]]; then
    echo "Running: $exe_path -c $config_file"
    "$exe_path" -c "$config_file"
  else
    echo "Warning: Config file '$config_file' not found, skipping."
  fi
done
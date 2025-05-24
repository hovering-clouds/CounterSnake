#!/bin/bash
mkdir -p config_8M
for f in config_4M/*; do
  if [[ -f "$f" ]]; then
    filename="${f##*/}"  # 提取文件名（去掉路径）
    if [[ "${filename%.*}" == *4M ]]; then
      # 构建新文件名（替换1M为2M并保留扩展名）
      newname="${filename%.*}"
      newname="${newname%4M}8M"
      [[ "$filename" == *.* ]] && newname+=".${filename##*.}"
      # 复制到目标目录
      cp -- "$f" "config_8M/$newname"
    fi
  fi
done
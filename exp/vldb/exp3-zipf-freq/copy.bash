#!/bin/bash
old_pfx="175"
new_pfx="200"

mkdir -p config_${new_pfx}
for f in config_${old_pfx}/*; do
  if [[ -f "$f" ]]; then
    filename="${f##*/}"  # 提取文件名（去掉路径）
    if [[ "${filename%.*}" == *${old_pfx} ]]; then
      # 构建新文件名（替换1M为2M并保留扩展名）
      newname="${filename%.*}"
      newname="${newname%"$old_pfx"}${new_pfx}"
      [[ "$filename" == *.* ]] && newname+=".${filename##*.}"
      # 复制到目标目录
      cp -- "$f" "config_${new_pfx}/$newname"
    fi
  fi
done
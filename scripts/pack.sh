#!/bin/bash

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_DIR=$(dirname "$SCRIPT_DIR")

if [ ! -d "$PROJECT_DIR/scripts" ] || [ ! -d "$PROJECT_DIR/release" ]; then
  echo "错误：缺少打包文件" >&2
  exit 1
fi

tmp_dir=$(mktemp -d -t ohno_pack_XXXXXX)
sudo mkdir -p "$tmp_dir/ohno"
sudo mkdir -p "$tmp_dir/ohno/configs"
sudo mkdir -p "$tmp_dir/ohno/scripts"
sudo mkdir -p "$PROJECT_DIR/packages"

# 获取默认 CNI 配置文件
sudo $PROJECT_DIR/build/ohno --get-conf

# 复制必需文件到临时目录
sudo mv -v -f ./ohno.json $tmp_dir/ohno/configs/
sudo cp -v -f "$PROJECT_DIR/build/ohno" "$tmp_dir/ohno"
sudo cp -v -f "$PROJECT_DIR/scripts/install.sh" "$tmp_dir/ohno/scripts/"
sudo cp -v -f "$PROJECT_DIR/scripts/launch.sh" "$tmp_dir/ohno/scripts/"
sudo cp -v -f "$PROJECT_DIR/scripts/uninstall.sh" "$tmp_dir/ohno/scripts/"

sudo chmod +x $tmp_dir/ohno/scripts/*.sh
sudo tar -czf "$PROJECT_DIR/packages/ohno.tar.gz" -C "$tmp_dir" .
sudo rm -rf "$tmp_dir"

echo "✅ 打包完成：$PROJECT_DIR/packages/ohno.tar.gz"
echo "包含文件："
sudo tar -ztvf "$PROJECT_DIR/packages/ohno.tar.gz" | awk '{print $6}'

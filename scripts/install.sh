#!/bin/bash

set -e

# 目前仅支持 Ubuntu
if ! [[ $(cat /etc/os-release | grep "^NAME=" | cut -d'=' -f2 | tr -d '"') == "Ubuntu" ]]; then
  echo "当前系统不是 Ubuntu"
  exit 1
elif [[ $(cat /etc/os-release | grep "^VERSION=" | cut -d'=' -f2 | tr -d '"' | awk '{print $1}') < "22.04" ]]; then
  echo "当前系统版本低于 Ubuntu 22.04"
  exit 1
fi

# 查看是否安装了 iproute2 以及 etcd-client 两个软件包
if ! dpkg -l | grep -q "iproute2"; then
  apt-get update
  apt-get install -y iproute2
fi
if ! dpkg -l | grep -q "etcd-client"; then
  apt-get update
  apt-get install -y etcd-client
fi

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_DIR=$(dirname "$SCRIPT_DIR")
INSTALL_BIN_DIR="/opt/cni/bin"
INSTALL_CONF_DIR="/etc/cni/net.d"
mkdir -p /opt/cni
mkdir -p $INSTALL_BIN_DIR
mkdir -p /etc/cni
mkdir -p $INSTALL_CONF_DIR

TARGET_BIN="$PROJECT_DIR/ohno"
TARGET_DAEMON="$PROJECT_DIR/ohnod"
TARGET_CONF="$PROJECT_DIR/configs/ohno.json"
if [[ ! -f "$TARGET_BIN" ]]; then
  echo "Error: 压缩包损坏, 不存在 ohno CNI 插件"
  exit 1
fi
if [[ ! -f "$TARGET_DAEMON" ]]; then
  echo "Error: 压缩包损坏, 不存在 ohnod 守护进程"
  exit 1
fi
if [[ ! -f "$TARGET_CONF" ]]; then
  echo "Error: 压缩包损坏, 不存在 ohno CNI 配置"
  exit 1
fi

sudo cp -v "$TARGET_BIN" "$INSTALL_BIN_DIR/"
sudo cp -v "$TARGET_DAEMON" "$INSTALL_BIN_DIR/"
sudo cp -v "$TARGET_CONF" "$INSTALL_CONF_DIR/"
sudo chmod +x "$INSTALL_BIN_DIR/ohno"

echo "✅ 安装 $INSTALL_CONF_DIR/ohno.json、$INSTALL_BIN_DIR/ohno 和 \
$INSTALL_BIN_DIR/ohnod 完成"

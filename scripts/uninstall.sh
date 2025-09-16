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

output_redirect="> /dev/null"
INSTALL_BIN_DIR="/opt/cni/bin"
INSTALL_CONF_DIR="/etc/cni/net.d"

if [[ -f "$INSTALL_BIN_DIR/ohno" ]]; then
  sudo rm -v "$INSTALL_BIN_DIR/ohno"
fi

if [[ -f "$INSTALL_CONF_DIR/ohno.json" ]]; then
  sudo rm -v "$INSTALL_CONF_DIR/ohno.json"
fi

# 输出卸载完成信息
echo "✅ 卸载完成: $INSTALL_BIN_DIR/ohno 和 $INSTALL_CONF_DIR/ohno.json 已被删除"

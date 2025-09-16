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

usage() {
  echo "Usage: $0 {add namespace_name|del namespace_name|version}"
}

if [ "$#" -lt 1 ]; then
  echo "至少需要一个参数"
  usage
  exit 1
fi

command=$1
if [ "$command" != "add" ] && [ "$command" != "del" ] && [ "$command" != "version" ]; then
  echo "未知 CNI 命令: $command"
  exit 1
fi
if [ "$command" != "version " ]; then
  if [ $# -ne 2 ]; then
    echo "add / del 需要带上 namespace 名称"
    usage
    exit 1
  fi
fi

namespace_path=/var/run/netns
ns=$2
if ! [ -f "$namespace_path/$ns" ]; then
  echo "网络空间 $ns 不存在"
  exit 1
fi

export CNI_COMMAND=$(echo "$command" | tr '[:lower:]' '[:upper:]')
export CNI_CONTAINERID=ohno-cid
export CNI_IFNAME=eth0
export CNI_NETNS=/var/run/netns/$ns

echo "执行 CNI 插件"
/opt/cni/bin/ohno < /etc/cni/net.d/ohno.json
if [[ $? -ne 0 ]]; then
  echo "CNI 插件执行失败"
else
  echo "CNI 插件执行成功"
fi

# 检查结果
echo
echo
echo "查看最后一次日志"
tail -n 26 /var/run/log/ohno.log
echo "查看 Linux namespace"
sudo ip netns show

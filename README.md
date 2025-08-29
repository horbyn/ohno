# Ohno

用 C++ 实现一个 CNI 插件

## 项目依赖

仅在 `Ubuntu 22.04` 验证过

静态检查、火焰图依赖、内存检查、单元测试相关

```shell
apt-get install clang-tidy linux-tools-common linux-tools-generic valgrind gcovr
```

项目使用 ETCD 实现 IPAM

```shell
apt-get install etcd-client
```

## 项目构建

```shell
cmake -S . -B build
cmake --build build -j $(getconf _NPROCESSORS_ONLN)
```

## 部署开发环境

项目基于 ETCD 实现了 IPAM，CNI 插件会运行一个 ETCD 客户端来访问 ETCD 节点。如果你已经在一个 Kubernetes 集群中，意味着已经部署好了 ETCD 集群，此时直接可以使用 `myclusters/Dockerfile-dev` 创建 `Docker` 镜像和容器；如果你像我一样是单机环境做开发，可以通过 `docker` 容器部署 ETCD 集群，参考 [Run etcd clusters inside containers](https://etcd.io/docs/v3.5/op-guide/container/)，项目提供了一个 `docker compose` 部署选项。在部署 ETCD 集群之前要生成证书，`myclusters/compose.yaml` 配置会生成一个拥有 3 个 ETCD 节点的集群，如果你想增加或减少，按需修改 `myclusters/compose.yaml` 配置文件

在执行之前先设置一些 **环境变量**，`myclusters/compose.yaml` 配置文件会用到：

- **OHNO_CERT_DIR**: 保存证书的目录
- **OHNO_ETCD1_DATA_DIR**: 第一个 ETCD 节点（ETCD1）的数据目录
- **OHNO_ETCD2_DATA_DIR**: 第二个 ETCD 节点（ETCD2）的数据目录
- **OHNO_ETCD3_DATA_DIR**: 第三个 ETCD 节点（ETCD3）的数据目录
- **OHNO_REPO**: 代码仓库目录
- **OHNO_ARCH**: CPU 架构，仅支持 aarch64 或 x86_64 两种

导出环境变量之后执行：

```shell
# 跳转到项目根目录
docker compose -f myclusters/compose.yaml up -d
```

这会创建 `ohno-cert:latest` 和 `ohno-dev:latest` 两个镜像，会创建 `etcd1`、`etcd2`、`etcd3`、`cert` 和 `dev-env` 五个容器，其中 `etcd1`、`etcd2`、`etcd3` 是 ETCD 集群，`cert` 是证书生成容器（生成证书之后就可以不用管了），`dev-env` 是开发环境容器

并且，所有容器都会将相关证书保存到容器自己的 `/etc/etcd/pki` 目录，所有 ETCD 操作无论是 ETCD 服务端、peer 还是 ETCD 客户端都使用同一份证书（公钥 etcd.pem 和私钥 etcd-key.pem）：

```shell
➜ ~ ls /etc/etcd/pki 
ca-config.json  ca.csr  ca-csr.json  ca-key.pem  ca.pem  etcd.csr  etcd-csr.json  etcd-key.pem  etcd.pem
```

利用证书执行一些健康检查：

```shell
PS D:\DockerCompose\etcd> docker exec etcd1 etcdctl --endpoints="https://etcd1:2379,https://etcd2:2379,https://etcd3:2379" --cacert=/etc/etcd/pki/ca.pem --cert=/etc/etcd/pki/etcd.pem --key=/etc/etcd/pki/etcd-key.pem --write-out=table endpoint health
+--------------------+--------+-------------+-------+
|      ENDPOINT      | HEALTH |    TOOK     | ERROR |
+--------------------+--------+-------------+-------+
| https://etcd1:2379 |   true | 14.238492ms |       |
| https://etcd3:2379 |   true | 34.598323ms |       |
| https://etcd2:2379 |   true | 35.096794ms |       |
+--------------------+--------+-------------+-------+
```

## 参考

- [cni](https://github.com/amoghgarg/cni)
- [simple-k8s-cni](https://github.com/y805939188/simple-k8s-cni)

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

## TODO

- `src/ipam/subnet.{h,cc}` 不支持 IPv6 子网计算
- `src/netlink` 实现 C++ 对 Netlink 编程接口的封装
- `src/util/env_std.{h,cc}` 使用的环境变量相关操作不是线程安全的
- `src/cni` 目录下 `cluster`、`node` 对象由于需要向外部暴露 setter，即外部可以修改它们的内部成员，所以 getter 返回值使用了 `std::shared_ptr<T>`，可能会导致滥用
- 线程应该用一个单独的类来封装而不是直接用 `std::thread`
- `src/backend/backend.{h,cc}` 用线程池来管理线程

## 参考

- [cni](https://github.com/amoghgarg/cni)
- [simple-k8s-cni](https://github.com/y805939188/simple-k8s-cni)

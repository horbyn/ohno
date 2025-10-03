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
# debug 版本
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DOHNO_STATIC_ANALYSIS=ON -DOHNO_MEMCHECK=ON -DOHNO_TEST=ON -DOHNO_BENCHMARK=ON -DOHNO_FLAMEGRAPH=ON -DENABLE_ETCDCTL_TEST=ON
cmake --build build -j $(getconf _NPROCESSORS_ONLN)

# release 版本
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j $(getconf _NPROCESSORS_ONLN)
```

项目以压缩包形式交付，执行 `scripts/pack.sh` 进行打包，然后会在 `packages` 目录生成压缩包

```shell
➜  /ohno git:(dev) ✗ scripts/pack.sh                                     
renamed './ohno.json' -> '/tmp/ohno_pack_sTEmtY/ohno/configs/ohno.json'
'/ohno/release/ohno' -> '/tmp/ohno_pack_sTEmtY/ohno/ohno'
'/ohno/release/src/ohnod/ohnod' -> '/tmp/ohno_pack_sTEmtY/ohno/ohnod'
'/ohno/scripts/install.sh' -> '/tmp/ohno_pack_sTEmtY/ohno/scripts/install.sh'
'/ohno/scripts/uninstall.sh' -> '/tmp/ohno_pack_sTEmtY/ohno/scripts/uninstall.sh'
✅ 打包完成：/ohno/packages/ohno.tar.gz
包含文件：
./
./ohno/
./ohno/scripts/
./ohno/scripts/uninstall.sh
./ohno/scripts/install.sh
./ohno/ohno
./ohno/configs/
./ohno/configs/ohno.json
./ohno/ohnod
😯 清理临时目录 /tmp/ohno_pack_sTEmtY
```

## 部署

项目由两部分组成，均会输出日志，默认目录是 `/var/run/log/ohno.log`：

- CNI 插件二进制文件：负责创建 Pod 网络，日志仅会输出文件
- DaemonSet：负责为实现跨节点通信，目前已实现 host-gw 静态路由方式，日志既会输出文件，也会输出 stdout

需要提前在所有工作节点创建 Deamon Set 所需镜像（镜像来自 [项目 myclusters 目录 Daemon Set 子目录](./myclusters/daemon_set/Dockerfile)）

```shell
# 假设两个工作节点为 worker1 和 worker2，当前在开发环境中
➜  /ohno git:(dev) ✗ scp packages/ohno.tar.gz root@worker1:/root
root@worker1's password: 
ohno.tar.gz                                                                            100% 6029KB  32.1MB/s   00:00    
➜  /ohno git:(dev) ✗ scp packages/ohno.tar.gz root@worker2:/root  
root@worker2's password: 
ohno.tar.gz                                                                            100% 6029KB  33.9MB/s   00:00
```

登录工作节点，创建 containerd 镜像：

```shell
# crictl rmi docker.io/ohno/ohnod:latest # 第一次不用执行，第二次之后执行以便清理过期镜像
cd /root && rm -rf ohno && tar -zxf ohno.tar.gz && cd ohno && scripts/install.sh
cd /root
docker build --load --platform linux/amd64 --build-arg ARCH=x86_64 -t ohno/ohnod:latest . # 需要提前在 /root 目录保存上面的 Daemon Set Dockerfile 文件

# 通过 docker 创建镜像，然后导入 containerd（如果你的 CRI 也是 containerd）
docker save -o ohnod.tar ohno/ohnod:latest
ctr -n k8s.io images import ohnod.tar
docker rmi ohno/ohnod:latest
rm ohnod.tar
echo "" > /var/run/log/ohno.log # 清空 CNI 二进制文件日志，kubelet 调用 CNI 过程中产生的日志都会保存到这里
```

登录 master 节点先创建 Daemon Set，使用 [DaemonSet 配置文件](./scripts/ohno.yaml) 来定义 DaemonSet 资源：

```shell
➜  ~ kubectl get pods -n ohno -o wide 
No resources found in ohno namespace.
# 通过环境变量的方式导入证书
➜  ~ export CA_CRT=$(cat /etc/kubernetes/pki/etcd/ca.crt | base64 -w 0)
export CLIENT_CRT=$(cat /etc/kubernetes/pki/etcd/healthcheck-client.crt | base64 -w 0)
export CLIENT_KEY=$(cat /etc/kubernetes/pki/etcd/healthcheck-client.key | base64 -w 0)
➜  ~ envsubst < ohno.yaml | kubectl apply -f -                         
namespace/ohno created
secret/etcd-certs created
serviceaccount/ohnod created
clusterrole.rbac.authorization.k8s.io/ohnod created
clusterrolebinding.rbac.authorization.k8s.io/ohnod created
daemonset.apps/ohnod created
➜  ~ kubectl get pods -n ohno -o wide
NAME          READY   STATUS    RESTARTS   AGE   IP             NODE       NOMINATED NODE   READINESS GATES
ohnod-wdrhx   1/1     Running   0          3s    192.168.0.2    worker1    <none>           <none>
ohnod-z4mq6   1/1     Running   0          3s    192.168.0.3    worker2    <none>           <none>
```

可以查看 Daemon Set 执行过程的日志：

```shell
➜  ~ kubectl logs -n ohno daemonset/ohnod
Found 2 pods, using pod/ohnod-z4mq6
[2025-09-28 12:41:32.455] [ohno] [info] [ohnod.cc:144] Launch ohnod v0.1.0
[2025-09-28 12:41:32.456] [ohno] [info] [ohnod.cc:145] API Server:       https://10.96.0.1:443
[2025-09-28 12:41:32.456] [ohno] [info] [ohnod.cc:146] SSL verification: enabled
[2025-09-28 12:41:32.456] [ohno] [info] [ohnod.cc:147] Refresh interval: 5
[2025-09-28 12:41:32.639] [etcd] [info] [etcd_client_shell.cc:46] ETCD cluster init successfully, addr:https://192.168.0.1:2379, ca_cert:/etc/etcd-certs/ca.crt, cert:/etc/etcd-certs/client.crt, key:/etc/etcd-certs/client.key
[2025-09-28 12:41:32.728] [backend] [info] [center.cc:65] Successfully accesses api server: https://10.96.0.1:443, ca_path: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt, token_path: /var/run/secrets/kubernetes.io/serviceaccount/token, ssl: enable
```

最后就可以通过 Deployment 资源来创建 Pod 了：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: bb
  namespace: ohno
spec:
  replicas: 2
  selector:
    matchLabels:
      app: busybox
  template:
    metadata:
      labels:
        app: busybox
    spec:
      containers:
      - name: busybox
        image: library/busybox
        imagePullPolicy: Never
        command:
          - sleep
          - "36000"
```

同节点通信和跨节点通信：

```shell
➜  ~ kubectl apply -f test-cni.yaml  
deployment.apps/bb created
➜  ~ kubectl get pods -n ohno -o wide
NAME                  READY   STATUS    RESTARTS   AGE     IP                NODE      NOMINATED NODE   READINESS GATES
bb-7c5f6bf8d9-ft8s5   1/1     Running   0          4s      10.244.1.4        worker1   <none>           <none>
bb-7c5f6bf8d9-s54td   1/1     Running   0          4s      10.244.2.2        worker2   <none>           <none>
ohnod-wdrhx           1/1     Running   0          3m26s   192.168.0.2       worker1   <none>           <none>
ohnod-z4mq6           1/1     Running   0          3m26s   192.168.0.3       worker2   <none>           <none>
➜  ~ kubectl exec -it bb-7c5f6bf8d9-ft8s5 -n ohno -- sh
/ # ping -c5 10.244.1.2
PING 10.244.1.2 (10.244.1.2): 56 data bytes
64 bytes from 10.244.1.2: seq=0 ttl=64 time=0.433 ms
64 bytes from 10.244.1.2: seq=1 ttl=64 time=0.139 ms
64 bytes from 10.244.1.2: seq=2 ttl=64 time=0.139 ms
64 bytes from 10.244.1.2: seq=3 ttl=64 time=0.152 ms
64 bytes from 10.244.1.2: seq=4 ttl=64 time=0.146 ms

--- 10.244.1.2 ping statistics ---
5 packets transmitted, 5 packets received, 0% packet loss
round-trip min/avg/max = 0.139/0.201/0.433 ms
/ # ping -c5 10.244.2.2
PING 10.244.2.2 (10.244.2.2): 56 data bytes
64 bytes from 10.244.2.2: seq=0 ttl=62 time=1.416 ms
64 bytes from 10.244.2.2: seq=1 ttl=62 time=0.693 ms
64 bytes from 10.244.2.2: seq=2 ttl=62 time=0.733 ms
64 bytes from 10.244.2.2: seq=3 ttl=62 time=0.726 ms
64 bytes from 10.244.2.2: seq=4 ttl=62 time=0.695 ms

--- 10.244.2.2 ping statistics ---
5 packets transmitted, 5 packets received, 0% packet loss
round-trip min/avg/max = 0.693/0.852/1.416 ms
```

## TODO

- Daemon Set 路由模式：VxLAN、动态路由
- `src/ipam/subnet.{h,cc}` 不支持 IPv6 子网计算
- `src/netlink` 实现 C++ 对 Netlink 编程接口的封装
- `src/util/env_std.{h,cc}` 使用的环境变量相关操作不是线程安全的
- `src/cni` 目录下 `cluster`、`node` 对象由于需要向外部暴露 setter，即外部可以修改它们的内部成员，所以 getter 返回值使用了 `std::shared_ptr<T>`，可能会导致滥用
- 线程应该用一个单独的类来封装而不是直接用 `std::thread`
- `src/backend/backend.{h,cc}` 用线程池来管理线程

## 参考

- [cni](https://github.com/amoghgarg/cni)
- [simple-k8s-cni](https://github.com/y805939188/simple-k8s-cni)

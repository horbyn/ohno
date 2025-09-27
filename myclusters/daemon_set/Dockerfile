FROM ubuntu:jammy

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai
ARG ARCH

RUN apt-get update && apt install -y --no-install-recommends ca-certificates && \
  cp /etc/apt/sources.list /etc/apt/sources.list.bak && \
  if [ "$ARCH" = "aarch64" ]; then \
    sed -i -e 's@//ports.ubuntu.com/\? @//ports.ubuntu.com/ubuntu-ports @g' \
      -e 's@//ports.ubuntu.com@//mirrors.ustc.edu.cn@g' \
      /etc/apt/sources.list; \
  elif [ "$ARCH" = "x86_64" ]; then \
    sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list; \
    sed -i 's/security.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list; \
    sed -i 's/http:/https:/g' /etc/apt/sources.list; \
  else \
    echo "未知架构，无法设置 apt 代理"; \
    exit 1; \
  fi && \
  apt-get update && apt install -y --no-install-recommends \
    sudo vim tzdata wget curl \
    iputils-ping iproute2 tcpdump lsof iptables etcd-client && \
  rm -rf /var/lib/apt/lists/* && \
  apt-get clean

COPY ./ohno.tar.gz /app/
WORKDIR /app
RUN tar -zxf ohno.tar.gz

CMD ["/bin/bash"]

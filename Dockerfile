FROM centos:centos8

RUN cd /etc/yum.repos.d/
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*


RUN yum update -y

RUN yum install -y \
    gcc \
    clang \
    llvm \
    elfutils-libelf-devel \
    kernel-devel \
    kernel-headers \
    make \
    iproute \
    iputils \
    git \
    vim-enhanced \
    which \
    python3-pip \
    cmake \
    llvm-devel.aarch64 \
    mpich-devel.aarch64

RUN dnf update -y

RUN dnf group install -y "Development Tools"
RUN dnf install -y clang-devel.aarch64 \
    gcc-toolset-11 \
    jq
RUN scl enable gcc-toolset-11 bash
RUN git clone --recurse-submodules https://github.com/libbpf/bpftool.git /workspaces/bpftool
RUN cd /workspaces/bpftool && \
    git checkout tags/v7.5.0 && \
    cd libbpf && \
    git checkout tags/v1.5.0 && \
    cd /workspaces/bpftool/libbpf/src && \
    make && \
    make install && \
    cd /workspaces/bpftool/src && \
    make && \
    make install

RUN git clone https://github.com/jbeder/yaml-cpp.git /workspaces/yaml-cpp
RUN cd /workspaces/yaml-cpp && \
    git checkout tags/yaml-cpp-0.7.0 && \
    mkdir build && \
    cd build && \
    cmake -DYAML_BUILD_SHARED_LIBS=ON -DYAML_CPP_BUILD_TESTS=OFF .. && \
    make -j && \
    make install

RUN git clone https://github.com/json-c/json-c.git /workspaces/json-c
RUN cd /workspaces/json-c && \
    git checkout tags/json-c-0.15-20200726 && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j && \
    make install
ENV CC=/usr/lib64/mpich/bin/mpicc
ENV CXX=/usr/lib64/mpich/bin/mpic++
# Set working directory
WORKDIR /workspace

# By default, run bash. eBPF programs require --privileged and --cap-add=ALL when running the container.
CMD ["/bin/bash"]
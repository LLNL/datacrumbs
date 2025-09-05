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
    mpich-devel.aarch64 \
    time

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

# # Set working directory
# WORKDIR /workspace

# # By default, run bash. eBPF programs require --privileged and --cap-add=ALL when running the container.
# CMD ["/bin/bash"]

COPY ../../ /workspaces/datacrumbs

# Enable devtoolset-11 for subsequent commands in the Dockerfile
# and set it as the default for interactive shells
ENV PATH="/opt/rh/gcc-toolset-11/root/usr/bin/:${PATH}"
ENV LD_LIBRARY_PATH="/opt/rh/gcc-toolset-11/root/usr/lib64:${LD_LIBRARY_PATH}"
# Verify GCC version
RUN gcc --version

RUN cd /workspaces && \
    mkdir -p build && \
    rm -rf build/* && \
    cd build && \
    cmake -DDATACRUMBS_HOST=docker -DDATACRUMBS_USER=root /workspaces/datacrumbs/ && \
    make

RUN mkdir -p /workspaces/logs

# Set up the default command to run the datacrumbs process, wait, run dd, send SIGINT, and wait for exit
CMD ["/bin/bash", "-c", "/workspaces/build/bin/datacrumbs \"docker\" \"--user\" \"root\" \"--config_path\" \"/workspaces/datacrumbs/etc/datacrumbs/configs\" \"--data_dir\" \"/workspaces/datacrumbs/etc/datacrumbs/data\" \"--trace_log_dir\" \"/workspaces/logs\" & DATACRUMBS_PID=$!; sleep 60; dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16; kill -SIGINT $DATACRUMBS_PID; wait $DATACRUMBS_PID"]
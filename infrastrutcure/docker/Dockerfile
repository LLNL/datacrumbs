FROM hdevarajan92/datacrumbs:latest

RUN mkdir -p /opt/datacrumbs
COPY . /opt/datacrumbs/
RUN . /opt/rh/gcc-toolset-11/enable && \
    mkdir -p datacrumbs-build && \
    rm -rf datacrumbs-build/* && \
    cd datacrumbs-build && \
    cmake -DDATACRUMBS_HOST=docker \
          -DDATACRUMBS_USER=root \
          -DDATACRUMBS_INSTALL_USER=docker \
          -DDATACRUMBS_KERNEL_HEADERS_PATH=/usr/src/kernels/4.18.0-348.7.1.el8_5.x86_64 \
          -DCMAKE_INSTALL_PREFIX=/opt/datacrumbs-install \
          /opt/datacrumbs/ && \
    make -j $(nproc) && \
    make install

ENV PATH=/opt/datacrumbs/datacrumbs-build/bin:${PATH}
ENV LD_LIBRARY_PATH="/opt/datacrumbs/datacrumbs-build/lib:${LD_LIBRARY_PATH}":${LD_LIBRARY_PATH}

# Create a non-root user 'docker' with passwordless sudo access and install tools
RUN (dnf -y install epel-release || yum -y install epel-release) && \
    (dnf -y install sudo || yum -y install sudo) && \
    (dnf -y install patchelf || yum -y install patchelf) && \
    (dnf clean all || yum clean all) && \
    groupadd -f wheel && \
    useradd -m -s /bin/bash docker && \
    usermod -aG wheel docker && \
    echo 'docker ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/docker && \
    chmod 0440 /etc/sudoers.d/docker

# Ensure docker user can access relevant directories under /opt
RUN mkdir -p /opt/datacrumbs /opt/datacrumbs-install && \
    chown -R docker:docker /opt/datacrumbs /opt/datacrumbs-install && \
    chmod 0755 /opt

USER docker

WORKDIR /opt/datacrumbs

# By default, run bash. eBPF programs require --privileged and --cap-add=ALL when running the container.
CMD ["/bin/bash"]
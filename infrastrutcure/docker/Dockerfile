FROM hdevarajan92/datacrumbs:latest

RUN mkdir -p /opt/datacrumbs
COPY . /opt/datacrumbs/
RUN . /opt/rh/gcc-toolset-11/enable && \
    mkdir -p datacrumbs-build && \
    rm -rf datacrumbs-build/* && \
    cd datacrumbs-build && \
    cmake -DDATACRUMBS_HOST=docker -DDATACRUMBS_USER=root -DDATACRUMBS_INSTALL_USER=root -DDATACRUMBS_KERNEL_PATH=4.18.0-348.7.1.el8_5.x86_64 /opt/datacrumbs/ && \
    make -j

ENV PATH=/opt/datacrumbs/datacrumbs-build/bin:${PATH}
ENV LD_LIBRARY_PATH="/opt/datacrumbs/datacrumbs-build/lib:${LD_LIBRARY_PATH}":${LD_LIBRARY_PATH}

WORKDIR /opt/datacrumbs

# By default, run bash. eBPF programs require --privileged and --cap-add=ALL when running the container.
CMD ["/bin/bash"]
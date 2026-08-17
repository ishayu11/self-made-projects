FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Install ns-3 dependencies
RUN apt-get update && apt-get install -y \
    g++ python3 python3-dev pkg-config sqlite3 cmake ninja-build git nano \
    && rm -rf /var/lib/apt/lists/*

# Clone and build ns-3.40
WORKDIR /usr/ns3
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git . && \
    git checkout ns-3.40 && \
    ./ns3 configure --build-profile=default --enable-examples && \
    ./ns3 build

CMD ["/bin/bash"]
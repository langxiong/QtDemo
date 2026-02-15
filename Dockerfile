# medical_robot_control_demo - Build container
# Standard open-source style: Ubuntu base, multi-stage build
# Usage (from demo/):
#   docker build -t mrc-demo .
#   docker run --rm -v $(pwd):/work -w /work mrc-demo ./scripts/build.sh --release
#
# From repo root:
#   docker build -t mrc-demo -f demo/Dockerfile .
#   docker run --rm -v $(pwd)/demo:/work -w /work mrc-demo ./scripts/build.sh --release

# Stage 1: Fast DDS ecosystem (Fast-CDR, Fast DDS, Fast-DDS-Gen)
FROM ubuntu:22.04 AS fastdds
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git \
    openjdk-17-jdk gradle \
    && rm -rf /var/lib/apt/lists/*
ENV INSTALL_PREFIX=/opt/fastdds
RUN git clone --depth 1 https://github.com/eProsima/Fast-CDR.git /tmp/fastcdr \
    && cmake -S /tmp/fastcdr -B /tmp/fastcdr/build -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
    && cmake --build /tmp/fastcdr/build && cmake --install /tmp/fastcdr/build
RUN git clone --depth 1 https://github.com/eProsima/Fast-DDS.git /tmp/fastdds \
    && cmake -S /tmp/fastdds -B /tmp/fastdds/build -DCMAKE_PREFIX_PATH=$INSTALL_PREFIX \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX && cmake --build /tmp/fastdds/build \
    && cmake --install /tmp/fastdds/build
RUN git clone --depth 1 --recurse-submodules https://github.com/eProsima/Fast-DDS-Gen.git /opt/fastddsgen \
    && cd /opt/fastddsgen && gradle assemble \
    && ln -sf /opt/fastddsgen/scripts/fastddsgen /usr/local/bin/fastddsgen

# Stage 2: Demo build
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git \
    libpoco-dev qt6-base-dev qt6-charts-dev qt6-tools-dev \
    openjdk-17-jre \
    && rm -rf /var/lib/apt/lists/*

COPY --from=fastdds /opt/fastdds /opt/fastdds
COPY --from=fastdds /opt/fastddsgen /opt/fastddsgen
COPY --from=fastdds /usr/local/bin/fastddsgen /usr/local/bin/fastddsgen

ENV PATH="/opt/fastdds/bin:$PATH"
ENV MRCD_FASTDDS_ROOT=/opt/fastdds
ENV CMAKE_PREFIX_PATH="/usr;/opt/fastdds"

WORKDIR /work
CMD ["./scripts/build.sh", "--release"]

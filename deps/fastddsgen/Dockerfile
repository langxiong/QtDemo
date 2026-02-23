# Fast-DDS-Gen Docker image for IDL code generation.
# Build: docker build -t fastddsgen-mrcd -f demo/deps/fastddsgen/Dockerfile demo/deps/fastddsgen
# Use:   -DMRCD_FASTDDSGEN_DOCKER_IMAGE=fastddsgen-mrcd

FROM eclipse-temurin:17-jdk AS builder
RUN apt-get update && apt-get install -y --no-install-recommends git ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /build
ARG FASTDDSGEN_VERSION=v2.4.0
RUN git clone --depth 1 --branch ${FASTDDSGEN_VERSION} https://github.com/eProsima/Fast-DDS-Gen.git . \
    && ./gradlew assemble --no-daemon

# Minimal runtime image (layout matches fastddsgen script: bin/fastddsgen -> ../share/fastddsgen/java/fastddsgen.jar)
FROM eclipse-temurin:17-jre
COPY --from=builder /build/share/fastddsgen/java/fastddsgen.jar /opt/fastddsgen/share/fastddsgen/java/
COPY --from=builder /build/scripts/fastddsgen /opt/fastddsgen/bin/
RUN chmod +x /opt/fastddsgen/bin/fastddsgen
ENV PATH="/opt/fastddsgen/bin:${PATH}"
ENTRYPOINT ["/opt/fastddsgen/bin/fastddsgen"]

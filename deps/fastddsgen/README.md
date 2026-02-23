# Fast-DDS-Gen Docker Image

Dockerfile for building [eProsima Fast-DDS-Gen](https://github.com/eProsima/Fast-DDS-Gen) (IDL code generator).

**Build:**
```bash
docker build -t fastddsgen-mrcd -f demo/deps/fastddsgen/Dockerfile demo/deps/fastddsgen
```

Or use the convenience script:
```bash
./demo/deps/scripts/build_fastddsgen_docker.sh
```

**Use with project:**
```bash
cmake -B build -S demo -DMRCD_FASTDDSGEN_DOCKER_IMAGE=fastddsgen-mrcd ...
```

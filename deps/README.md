# Dependencies (Git Submodules)

This directory contains bundled dependencies built from source: Poco, Fast-CDR, foonathan_memory, and Fast-DDS.

## Submodules / Vendored Deps

| Dir | Purpose |
|-----|---------|
| `poco/` | C++ libraries: Foundation, Util, Net |
| `fastcdr/` | CDR serialization (Fast-DDS dependency) |
| `foonathan_memory_vendor/` | Memory allocator (Fast-DDS dependency) |
| `fastdds/` | DDS middleware |
| `duilib/` | DuiLib_Ultimate (Windows); clone into deps if missing |

## Build Order

1. **foonathan_memory** – memory allocator (fetched and built via foonathan_memory_vendor)
2. **Fast-CDR** – serialization library
3. **Fast-DDS** – depends on Fast-CDR and foonathan_memory; uses thirdparty for asio, tinyxml2
4. **Poco** – Foundation, Util, Net only (minimal build)
5. **DuiLib** (Windows only) – host window UI for controller_app; source in `duilib/` from [DuiLib_Ultimate](https://github.com/qdtroy/DuiLib_Ultimate); builds library only (no Demos). cef_host does not use DuiLib.

## Building Dependencies

If `duilib/` is missing, clone it: `git clone https://github.com/qdtroy/DuiLib_Ultimate.git demo/deps/duilib`

The main project can build deps automatically when `MRCD_USE_DEPS_SUBMODULES` is ON. When enabled, CEF prebuilt is also fetched from CDN (if not present) unless `-DMRCD_FETCH_CEF=OFF`, and DuiLib is built (Windows only) for controller_app.

**Recommended build (root-level `build_debug`/`build_release`, single-config):** Uses Ninja with flat `bin/`/`lib/` layout for compatibility with vcpkg, Conan, and other third-party libraries.

```bash
# From project root (directory containing demo/)
cmake --preset build_debug -S demo
cmake --build --preset build_debug

# Or Release
cmake --preset build_release -S demo
cmake --build --preset build_release
```

Build and install layout:
- Main project: `build_debug/`, `build_release/` at project root (flat `bin/`, `lib/`)
- Deps: `install/Debug`, `install/Release` under `demo/deps/install`

To build deps manually as a standalone project:

```bash
# Initialize submodules first (or let main project do it)
git submodule update --init --recursive

# Build Debug
cmake -B build_deps_Debug -S demo/deps \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDEPS_INSTALL_CONFIG=Debug
cmake --build build_deps_Debug
cmake --install build_deps_Debug --prefix demo/deps/install/Debug

# Build Release
cmake -B build_deps_Release -S demo/deps \
  -DCMAKE_BUILD_TYPE=Release \
  -DDEPS_INSTALL_CONFIG=Release
cmake --build build_deps_Release
cmake --install build_deps_Release --prefix demo/deps/install/Release
```

## Using Built Deps in the Main Project

Use `build_debug` or `build_release` presets (they set `MRCD_USE_DEPS_SUBMODULES=ON` and `CMAKE_PREFIX_PATH`):

```bash
cmake --preset build_debug -S demo
cmake --build --preset build_debug
```

Or configure manually with `-DCMAKE_PREFIX_PATH=demo/deps/install/Debug` or `demo/deps/install/Release` for the matching config.

**vcpkg:** Use `-DCMAKE_TOOLCHAIN_FILE` with a single-config build. Set `CMAKE_PREFIX_PATH` to `demo/deps/install/Debug` or `install/Release` for the corresponding triplet.

## Fast-DDS-Gen (Docker)

Fast-DDS-Gen (IDL code generator) is Java/Gradle-based and **not** in submodules. The preferred way is **Docker**:

```bash
# Build the Fast-DDS-Gen Docker image (from project root)
./demo/deps/scripts/build_fastddsgen_docker.sh

# Configure main project with Docker image
cmake -B build -S demo -DMRCD_USE_DEPS_SUBMODULES=ON
# Or set image explicitly: -DMRCD_FASTDDSGEN_DOCKER_IMAGE=fastddsgen-mrcd
```

Dockerfile: `demo/deps/fastddsgen/Dockerfile` (builds from eProsima Fast-DDS-Gen v2.4.0).

Other options: install native fastddsgen and set `-DMRCD_FASTDDSGEN=/path/to/fastddsgen`, or use another Docker image name. See `docs/FASTDDS_SETUP.md`.

## Patches

`patches/duilib-no-flash.patch` – Excludes UIFlash from DuiLib (flash11.tlh unavailable; Flash deprecated). Applied to `duilib/` before build. Requires `patch` (e.g. from Git for Windows).

`patches/fastdds-msvc-utf8.patch` – On MSVC, patches Fast-DDS CMakeLists to add `/utf-8 /WX-`, fixing C4819 on CP936 (Chinese) systems. Requires `patch` (e.g. from Git for Windows).

## Fallback: Pre-built Fast-DDS

If building Fast-DDS from source fails (e.g. patch not found or other errors on Windows with non-UTF-8 locale), use pre-built Fast-DDS instead:

- Set `MRCD_USE_DEPS_SUBMODULES=OFF`
- Set `MRCD_FASTDDS_ROOT` to your Fast-DDS install prefix
- Ensure Poco is available (vcpkg or system)

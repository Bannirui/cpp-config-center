# AGENTS.md — Config Center

## Build & Run

```bash
# Quick configure + build (most common workflow)
./configer.sh          # cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j # or ./build.sh

# Cross-platform presets (auto-detect macOS/Linux, set Homebrew paths on Darwin)
cmake --preset debug            # Debug + tests + ASAN/UBSAN
cmake --preset debug-no-san     # Debug + tests, NO sanitizers (faster iteration)
cmake --preset release          # Release, no tests
cmake --preset release-with-tests

cmake --build build/<presetName> --parallel

# Machine-specific dep paths — create from template, it's gitignored
cp CMakeUserPresets.json.template CMakeUserPresets.json

# Run the server
./build/<preset>/src/config-center-server config-server.yaml
```

- Build output: `build/<presetName>/` (e.g. `build/debug/`, `build/release/`)
- `configer.sh` and `build.sh` are shortcuts for a plain Release build into `build/`
- Use `-DBUILD_TESTS=OFF` to skip test targets. Tests are ON in `debug` and `debug-no-san` presets.
- Homebrew paths hardcoded in CMakePresets.json for `/opt/homebrew`. Intel Macs or custom installs need `CMakeUserPresets.json`.
- CI uses `-G Ninja`; presets use the platform default generator (Make on Linux). Either works.

## Testing

```bash
# Run all tests (from build dir)
cd build/debug && ctest --output-on-failure

# Run a single test by regex
cd build/debug && ctest -R test_name_pattern --output-on-failure
```

- Every `.cpp` in `tests/` is auto-discovered (GLOB_RECURSE) and compiled as its own GTest executable.
- Tests link both `config-center-core` and `config-center-sdk`.
- CI runs tests only in Debug builds, with ASAN/UBSAN variants on Ubuntu.

## Lint & Format

```bash
# clang-format check (Google style, 4-space indent, 120 cols)
find src include sdk tests -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror || true

# clang-tidy check (uses compile_commands.json from build)
find src include sdk tests -name '*.cpp' -o -name '*.h' | xargs clang-tidy -p build/debug --warnings-as-errors='*' || true
```

- CI runs both checks but does NOT block on failure (`|| true`). Run locally before PR.
- `compile_commands.json` is auto-generated (`CMAKE_EXPORT_COMPILE_COMMANDS: ON` in all presets).

## Code Conventions

- **Naming** (enforced by `.clang-tidy`):
  - Classes/Structs/Enums: `CamelCase`
  - Functions/Methods: `camelBack`
  - Variables/Params/Members: `lower_case`
  - Private/Protected members: trailing `_` suffix
  - Constants: `UPPER_CASE`
- **Formatting**: Google-based `.clang-format`, 4-space indent, no tabs, 120 char limit, pointer/reference align left.
- C++17, no extensions. `CMAKE_POSITION_INDEPENDENT_CODE` is ON (static libs linked into shared/server).

## Architecture

```
config-center-core (header-only lib) — include/
config-center-sdk (static lib)       — sdk/src/, depends on core
config-center-proto (static lib)     — generated from proto/config_service.proto
config-center-server (executable)    — src/main.cpp, links core + gRPC + Beast
```

- `include/config-center/` mirrors `src/` subdirectories (api/, auth/, config/, publish/, server/, storage/, subscription/).
- `config-center-core` is a header-only library (no compiled sources). Its interface headers are in `include/`.
- `config-center-sdk` contains the actual implementation sources in `sdk/src/` and links to the core header-only library.
- `proto/` generates C++ gRPC + protobuf code at build time.
- Headers for `jwt-cpp` are from FetchContent source tree — included directly from `config-center-core`.
- `cmake/` contains custom find modules: `boost.cmake`, `openssl.cmake`, `protobuf_grpc.cmake` for system packages; `nlohmann_json.cmake`, `yaml-cpp.cmake`, `jwt-cpp.cmake`, `sqlite_orm.cmake`, `googletest.cmake` for FetchContent.

## Dependencies

**System packages** (must be pre-installed):
- Boost (system + beast), OpenSSL, Protobuf + gRPC

**FetchContent** (auto-downloaded by CMake):
- nlohmann/json, yaml-cpp, jwt-cpp, sqlite_orm, GTest

## OpenSpec

This project uses OpenSpec for change planning. Commands live in `.opencode/commands/` (opsx-apply, opsx-archive, opsx-explore, opsx-propose). When proposing or implementing changes, follow the OpenSpec workflow via those commands.

# AGENTS.md — Config Center

## Build & Run

```bash
# Configure + build (debug, tests, sanitizers on macOS)
cmake --preset macos-debug
cmake --build build/macos-debug --parallel

# Configure + build (release, no tests, macOS)
cmake --preset macos-release
cmake --build build/macos-release --parallel

# Quick rebuild after editing source (if already configured)
cmake --build build/debug -j      # or ./build.sh for Release

# Machine-specific dep paths — create from template, it's gitignored
cp CMakeUserPresets.json.template CMakeUserPresets.json

# Run the server
./build/<preset>/src/config-center-server config-server.yaml
```

- Cross-platform presets: `debug`, `release`, `release-with-tests` auto-detect macOS/Linux and set Homebrew paths on Darwin.
- Use `-DBUILD_TESTS=OFF` to skip test targets. Tests are ON in `debug` and `release-with-tests` presets.
- Homebrew paths are hardcoded in CMakePresets.json for `/opt/homebrew`. Intel Macs or custom installs need `CMakeUserPresets.json`.

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
find src include sdk tests -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror

# clang-tidy check
find src include sdk tests -name '*.cpp' -o -name '*.h' | xargs clang-tidy -p build/debug
```

- CI runs both checks but does NOT block on failure (`|| true`). Run locally before PR.
- `compile_commands.json` is auto-generated (`CMAKE_EXPORT_COMPILE_COMMANDS: ON` in all presets).

## Code Conventions

- **Naming** (enforced by `.clang-tidy`):
  - Classes/Structs/Enums: `CamelCase`
  - Functions/Methods: `camelBack`
  - Variables/Params/Members: `lower_case`
  - Private/Protected members: `trailing_underscore_`
  - Constants: `UPPER_CASE`
- **Formatting**: Google-based, 4-space indent, no tabs, 120 char limit, pointer/reference align left.
- C++17, no extensions.

## Architecture

```
config-center-core (static lib)  — include/, built from src/ dirs
config-center-sdk (static lib)   — sdk/src/, depends on core
config-center-proto (static lib) — generated from proto/config_service.proto
config-center-server (executable)— src/main.cpp, links core + gRPC + Beast
```

- `include/config-center/` mirrors `src/` subdirectories (api/, auth/, config/, publish/, server/, storage/, subscription/).
- `config-center-core` is the shared library. The server and SDK both link it.
- `proto/` generates C++ gRPC + protobuf code at build time.
- Headers for `jwt-cpp` are from FetchContent source tree — included directly from `config-center-core`.

## Dependencies

**System packages** (must be pre-installed):
- Boost (system + beast), OpenSSL, Protobuf + gRPC

**FetchContent** (auto-downloaded by CMake):
- nlohmann/json, yaml-cpp, jwt-cpp, sqlite_orm, GTest

## OpenSpec

This project uses OpenSpec for change planning. Commands live in `.opencode/commands/` (opsx-apply, opsx-archive, opsx-explore, opsx-propose). When proposing or implementing changes, follow the OpenSpec workflow via those commands.

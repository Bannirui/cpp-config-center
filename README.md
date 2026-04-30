# Config Center

A C++ native remote configuration center inspired by Apollo and Nacos.

## Features

- **Versioned Config Storage**: Every config change creates a new version with full audit trail
- **Multi-Tenant Isolation**: Namespace model (application + environment + cluster) for organizing configs
- **Real-time Push**: Long-polling (HTTP) and gRPC streaming for instant config updates
- **Draft/Publish Workflow**: Draft editing, optional approval gating, and grayscale releases
- **Dual Protocol**: REST API (port 8080) for management, gRPC (port 9090) for SDK communication
- **C++ Client SDK**: Cache-first reads, hot-reload callbacks, local file cache failover
- **Multiple Backends**: SQLite for development, MySQL for production

## Quick Start

### Prerequisites

- C++17 compiler (GCC 12+, Clang 15+)
- CMake 3.20+
- **System packages:** Boost (system), OpenSSL, Protobuf, gRPC

**macOS:**
```bash
brew install cmake boost openssl protobuf grpc
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake libboost-system-dev libssl-dev protobuf-compiler libprotobuf-dev libgrpc++-dev
```

### Build

All other dependencies (nlohmann/json, yaml-cpp, jwt-cpp, sqlite_orm, GTest) are automatically fetched by CMake via `FetchContent` — no extra package manager needed.

```bash
# Clone and build
git clone <repo-url> config-center
cd config-center

# Configure (dependencies auto-download on first build)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run
./build/src/config-center-server config-server.yaml
```

To skip tests:
```bash
cmake -B build -DBUILD_TESTS=OFF
cmake --build build --parallel
```

### Configuration

Edit `config-server.yaml`:

```yaml
server:
  http:
    port: 8080
  grpc:
    port: 9090

storage:
  backend: sqlite  # or "mysql"
  sqlite:
    path: data/config-center.db
  mysql:
    host: 127.0.0.1
    port: 3306
    user: root
    password: ""
    database: config_center

auth:
  enabled: true
  jwt:
    secret: "your-secret-key"
    expiry_hours: 24
```

### Quick Test

```bash
# Health check
curl http://localhost:8080/health

# Create application
curl -X POST http://localhost:8080/api/v1/apps \
  -H "Content-Type: application/json" \
  -d '{"app_id": "my-service", "name": "My Service"}'

# Create config
curl -X PUT http://localhost:8080/api/v1/namespaces/my-service/DEV/default/application/keys/timeout \
  -H "Content-Type: application/json" \
  -d '{"value": "30"}'

# Get config
curl http://localhost:8080/api/v1/namespaces/my-service/DEV/default/application/keys/timeout

# Publish
curl -X POST http://localhost:8080/api/v1/namespaces/my-service/DEV/default/application/publish \
  -H "Content-Type: application/json" \
  -d '{"release_note": "Initial release"}'
```

## Architecture

```
┌──────────────────────────────────────────────┐
│                  Clients                     │
│  ┌──────────────────┐ ┌──────────────────┐  │
│  │   C++ SDK        │ │   HTTP Client    │  │
│  │ (gRPC streaming) │ │ (REST + Polling) │  │
│  └────────┬─────────┘ └────────┬─────────┘  │
└───────────┼────────────────────┼────────────┘
            │                    │
     ┌──────▼────────────────────▼──────┐
     │         Config Center Server      │
     │  ┌──────────┐  ┌──────────────┐  │
     │  │ HTTP API  │  │  gRPC API    │  │
     │  │ (Beast)   │  │  (Protobuf)  │  │
     │  └────┬──────┘  └──────┬───────┘  │
     │       └───────┬────────┘          │
     │         ┌─────▼─────┐             │
     │         │   Logic   │             │
     │         │ (Publish, │             │
     │         │ Auth, etc)│             │
     │         └─────┬─────┘             │
     │         ┌─────▼─────┐             │
     │         │ ConfigStore│            │
     │         │ (Abstract) │            │
     │         └─────┬─────┘             │
     └───────────────┼───────────────────┘
                     │
            ┌────────┴────────┐
            │                 │
       ┌────▼────┐     ┌─────▼────┐
       │  SQLite  │     │   MySQL  │
       └─────────┘     └──────────┘
```

## Project Structure

```
cpp-config-center/
├── CMakeLists.txt
├── vcpkg.json
├── config-server.yaml
├── proto/          # Protobuf definitions
├── include/        # Public headers
│   └── config-center/
│       ├── server/     # HTTP/gRPC servers
│       ├── storage/    # Data models, ConfigStore
│       ├── api/        # API handlers
│       ├── publish/    # Publish engine
│       ├── subscription/ # Notification channels
│       ├── auth/       # JWT, RBAC
│       └── config/     # Configuration parser
├── src/            # Source implementations
├── sdk/            # Client SDK
├── tests/          # Integration tests
└── docs/           # Documentation
```

## License

MIT

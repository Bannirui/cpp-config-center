## Context

This is a greenfield C++ project implementing a remote configuration center inspired by Apollo (Ctrip) and Nacos (Alibaba). The server manages configuration key-value pairs across multiple tenants, versions changes, and pushes updates to connected clients in real time. Target users operate latency-sensitive or resource-constrained C++ services that want centralized config without a JVM dependency.

**Constraints:**
- C++17 minimum (C++20 preferred where available)
- Single binary deployable (embedded SQLite), optionally backed by external MySQL
- HTTP API must be self-contained — no external gateway required
- Client SDK must be header-only or linkable static/dynamic library

## Goals / Non-Goals

**Goals:**
- C++ native config server with REST + gRPC APIs
- Versioned config storage with rollback and change audit trail
- Real-time config push via long-polling and gRPC streaming
- Namespace isolation (application/environment/cluster)
- Config publish lifecycle: draft → review → release
- C++ client SDK with local cache, hot-reload, and failover
- Grayscale (canary) release support

**Non-Goals:**
- Service discovery or DNS (this is a config center, not a full Nacos clone)
- Web admin UI (use Apollo Portal or Nacos Console; API is compatible)
- Horizontal scaling with Raft/Paxos consensus (single-node authoritative; HA via MySQL replication)
- Lua/Python/JS client SDKs (C++ only for this change)
- Docker image or k8s operator (out of scope; documented deployment instead)

## Decisions

### 1. Single-process, multi-threaded server with Boost.ASIO

**Choice:** ASIO event loop with `io_context` pool (1 thread per core). HTTP via Boost.Beast, gRPC via gRPC C++ library.
**Alternative considered:** libuv + uWebSockets. Rejected — gRPC C++ natively integrates with its own completion queue; mixing event loops adds complexity.
**Rationale:** Boost.ASIO is the de facto C++ async I/O library, integrates well with Beast (HTTP/WS), and a thread-per-core model keeps the architecture simple.

### 2. Dual protocol: HTTP REST + gRPC

**Choice:** HTTP on port 8080 for admin/config CRUD + long-polling; gRPC on port 9090 for subscription streaming and SDK communication.
**Alternative considered:** gRPC-only. Rejected — HTTP REST is required for web UI compatibility (Apollo Portal style) and simpler debugging/curl usage.
**Rationale:** HTTP handles the "management plane" (CRUD, approval), gRPC handles the "data plane" (high-throughput config push to clients). Both share the same backend storage layer.

### 3. Storage layer with repository pattern

**Choice:** Abstract `ConfigStore` interface with MySQL and SQLite implementations. MySQL uses `mysql-connector-cpp`; SQLite uses `sqlite_orm`.
**Alternative considered:** LevelDB or RocksDB. Rejected — relational data (namespaces, versions, approvals) benefits from SQL; relational DBs provide transactional integrity for publish operations.
**Rationale:** Repository pattern lets users swap backends. SQLite for dev/embedded, MySQL for production with replication.

### 4. Namespace model: App + Environment + Cluster

**Choice:** Three-level namespace hierarchy matching Apollo's model:
```
{appId}/{env}/{cluster}/{namespace}/{key} → value (with version)
```
- `appId`: service identifier (e.g., `order-service`)
- `env`: DEV / FAT / UAT / PROD
- `cluster`: `default` or regional cluster name
- `namespace`: logical config group (e.g., `application`, `database`)
**Rationale:** This model is proven in production at scale (Apollo manages thousands of apps at Ctrip). Direct compatibility with Apollo client concepts.

### 5. Push mechanism: Long-polling + gRPC streaming

**Choice:** HTTP long-polling for simple clients, gRPC server-streaming for SDK clients. Server maintains a per-namespace notification channel. On config publish, all subscribers watching that namespace receive the change notification.
**Alternative considered:** WebSocket. Rejected — gRPC streaming provides stronger typing (Protobuf) and backpressure semantics.
**Rationale:** Long-polling is universally compatible (curl, any HTTP client). gRPC streaming offers lower latency and efficient multiplexing for SDK clients.

### 6. Grayscale release via IP/app-instance targeting

**Choice:** On publish, specify release rules: percentage rollout or explicit IP/instance-id whitelist. Server tracks which instances have received which version.
**Alternative considered:** Canary by cluster only. Rejected — too coarse; same cluster may need partial rollout.
**Rationale:** Matches Apollo's grayscale model. Implementation complexity is moderate (rule engine in publish path).

### 7. Client SDK: cache-first with file-based failover

**Choice:** SDK maintains in-memory LRU cache. On startup, loads from local file cache. Connects to server, subscribes to watched namespaces. On config change notification, fetches new config and invokes user callbacks. If server unreachable, falls back to stale cached values.
**Alternative considered:** Client-side polling. Rejected — adds unnecessary server load; push-based is more efficient.
**Rationale:** Cache-first ensures zero-latency reads. File failover ensures service can start even if config server is down.

### 8. Auth: JWT Bearer tokens

**Choice:** Server issues JWT tokens via login endpoint. API requests include `Authorization: Bearer <token>`. Tokens carry role claims (admin/operator/viewer).
**Alternative considered:** Basic auth. Rejected — JWT enables stateless auth across potential future horizontal scaling.
**Rationale:** JWT is standard, widely supported, and integrates with existing identity systems.

## Risks / Trade-offs

- **[Single-node write authority]**: Writes go to one MySQL primary or one SQLite file. Not horizontally scalable for writes. → Mitigation: Config write volume is inherently low (human-triggered publishes). Read path scales via replicas.
- **[C++ memory safety]**: Risk of use-after-free, buffer overflow in a network server. → Mitigation: ASAN/TSAN/UBSAN in CI; use smart pointers; fuzz test HTTP/gRPC endpoints.
- **[MySQL dependency for production]**: Adds operational complexity. → Mitigation: SQLite works for single-node; documented MySQL setup for HA.
- **[Client SDK ABI stability]**: C++ ABI fragility across compilers. → Mitigation: Provide C API wrapper in SDK for stable ABI; header-only option for template users.

## Migration Plan

Not applicable — this is a new project. Deployment steps:

1. Build the server binary (`cmake && make`)
2. Configure `config-server.yaml` (storage backend, ports, JWT secret)
3. Run binary; it self-initializes the database schema
4. Create applications/namespaces via API or import from Apollo/Nacos export
5. Deploy clients with SDK, pointing to server address

Rollback: Not applicable for initial deployment.

## Open Questions

- Should we support configuration inheritance (namespace inherits from parent app defaults)? → Defer to v2.
- Encryption at rest for sensitive config values? → Defer; users can encrypt values before storing.
- Plugin system for custom config value validation? → Defer to v2.

## 1. Project Setup

- [x] 1.1 Create CMake project structure with top-level CMakeLists.txt, src/, include/, tests/, proto/ directories
- [x] 1.2 Add vcpkg manifest (vcpkg.json) with dependencies: Boost (ASIO, Beast, JSON), gRPC/Protobuf, nlohmann/json, jwt-cpp, SQLite ORM, mysql-connector-cpp
- [x] 1.3 Create .clang-format, .clang-tidy, and CI workflow (GitHub Actions) with ASAN/UBSAN builds
- [x] 1.4 Create config-server.yaml schema and YAML config parser using yaml-cpp

## 2. Data Model & Storage Layer (config-persistence)

- [x] 2.1 Define SQL schema (DDL): applications, namespaces, configs, config_versions, change_logs, releases, approvals, users, roles tables
- [x] 2.2 Implement `ConfigStore` abstract interface with CRUD methods for all entities
- [x] 2.3 Implement `SqliteConfigStore` with auto-migration (table creation on startup)
- [x] 2.4 Implement `MysqlConfigStore` with auto-migration and connection pool
- [x] 2.5 Implement config versioning logic: create new version on write, preserve history
- [x] 2.6 Implement rollback feature: create new version from historical version value
- [x] 2.7 Implement change history query with pagination and filtering by key/namespace/time range
- [x] 2.8 Implement transactional publish: wrap release creation and version updates in DB transaction

## 3. Namespace Management (namespace-management)

- [x] 3.1 Implement application registration: create, list, delete applications with unique ID constraint
- [x] 3.2 Implement environment enumeration: predefined DEV/FAT/UAT/PROD with optional custom environments
- [x] 3.3 Implement cluster management: create/lookup clusters per app+environment, enforce default cluster
- [x] 3.4 Implement namespace CRUD with types (private, public, common) scoped to app/env/cluster
- [x] 3.5 Implement namespace inheritance: parent-child relationship with config key fallback resolution
- [x] 3.6 Implement config key validation (name pattern `[a-zA-Z0-9._-]+`, max 128 chars) and value size limit (1 MB)

## 4. HTTP API Layer (config-api)

- [x] 4.1 Implement HTTP server with Boost.Beast on port 8080, routing with URL parameter extraction
- [x] 4.2 Implement config CRUD endpoints: `GET/PUT/DELETE /api/v1/namespaces/{ns}/keys/{key}`
- [x] 4.3 Implement namespace management endpoints: `POST/GET/DELETE /api/v1/apps`, `GET/POST /api/v1/namespaces`
- [x] 4.4 Implement publish/release endpoints: `POST /api/v1/namespaces/{ns}/publish`, `GET /api/v1/namespaces/{ns}/releases`
- [x] 4.5 Implement long-polling endpoint: `GET /api/v1/notifications?namespace={ns}&release_id={id}&timeout={t}`
- [x] 4.6 Implement health check: `GET /health` returning JSON status, uptime, DB connectivity
- [x] 4.7 Add JSON request/response serialization with validation and error response format (`{"error": "...", "code": ...}`)

## 5. gRPC Service (config-api)

- [x] 5.1 Define Protobuf schema: `config_service.proto` with messages for Config, Namespace, Release, Notification and RPC methods (GetConfig, SetConfig, DeleteConfig, WatchConfig, PublishRelease, GetReleaseHistory)
- [x] 5.2 Implement gRPC server on port 9090 with thread pool integration
- [x] 5.3 Implement unary RPC handlers: GetConfig, SetConfig, DeleteConfig, PublishRelease, GetReleaseHistory
- [x] 5.4 Implement server-streaming RPC handler: WatchConfig with per-client notification channel

## 6. Configuration Publishing (config-publish)

- [x] 6.1 Implement draft editing layer: isolate draft changes per namespace, return draft status on reads
- [x] 6.2 Implement publish flow: atomically move draft changes to live, create release record with release note
- [x] 6.3 Implement approval workflow: namespace-level approval toggle, draft submission, approver review, approve/reject actions
- [x] 6.4 Implement grayscale release engine: percentage-based routing with consistent hashing and IP/instance-id whitelist rules
- [x] 6.5 Implement full rollout: transition grayscale release to full release, retire old version
- [x] 6.6 Implement release history: list past releases per namespace in reverse chronological order with pagination
- [x] 7.1 Implement per-namespace notification channel: pub/sub in-memory, notify on publish
- [x] 7.2 Implement long-polling handler: hold connection, wait for notification or timeout, return result
- [x] 7.3 Implement gRPC WatchConfig streaming: register client on notification channel, push on change
- [x] 7.4 Implement catch-up mechanism: client provides last_release_id, server returns missed releases
- [x] 8.1 Implement user management: create/list users with hashed passwords (bcrypt)
- [x] 8.2 Implement JWT token generation and validation with configurable secret and expiry
- [x] 8.3 Implement login endpoint: `POST /api/v1/auth/login` returning JWT token
- [x] 8.4 Implement auth middleware: validate Bearer token on protected routes for HTTP and gRPC
- [x] 8.5 Implement role-based access control: admin/operator/viewer roles with permission checks on write endpoints

## 9. Client SDK (client-sdk)

- [x] 9.1 Create SDK CMake project with shared/static/header-only build options
- [x] 9.2 Implement `ConfigClient` class: Init(server, appId, env), Shutdown()
- [x] 9.3 Implement config fetch and in-memory LRU cache with cache-first reads
- [x] 9.4 Implement gRPC-based subscription with auto-reconnect and long-polling fallback
- [x] 9.5 Implement hot-reload callback system: register/remove callbacks, invoke on config change
- [x] 9.6 Implement local file cache: serialize to JSON on shutdown, deserialize on startup, stale-while-revalidate
- [x] 9.7 Implement typed accessors: GetConfig (string), GetConfigInt, GetConfigDouble, GetConfigBool, GetConfigJson
- [x] 9.8 Implement thread-safe cache with shared_mutex (read-write lock)
- [x] 9.9 Implement background reconnection: exponential backoff, continue serving stale cache

## 10. Grayscale Release Client Support

- [x] 10.1 Implement server-side release rule engine: evaluate rules (percentage hash, IP whitelist) per client request
- [x] 10.2 Implement client instance identity: SDK sends IP or instance-id in subscription request
- [x] 10.3 Implement full rollout transition on the server side: upgrade all clients to new version

## 11. Integration Tests

- [x] 11.1 Write integration tests for SQLite storage: CRUD, versioning, rollback, change history
- [x] 11.2 Write integration tests for MySQL storage: same test suite against MySQL container
- [x] 11.3 Write HTTP API integration tests: all endpoints with success and error cases
- [x] 11.4 Write gRPC integration tests: unary and streaming methods
- [x] 11.5 Write publish workflow integration tests: draft → publish, approval flow, grayscale release
- [x] 11.6 Write subscription integration tests: long-polling notification delivery, gRPC streaming
- [x] 11.7 Write client SDK integration tests: init, fetch, cache, hot-reload callback, failover, reconnect
- [x] 12.1 Write README.md with build instructions, quick-start guide, and architecture overview
- [x] 12.2 Document REST API endpoints with request/response examples in docs/api.md
- [x] 12.3 Document gRPC service with proto annotations and usage examples in docs/grpc.md
- [x] 12.4 Write client SDK usage guide with code examples in docs/sdk-guide.md
- [x] 12.5 Write deployment guide: single-node (SQLite) and production (MySQL) setups

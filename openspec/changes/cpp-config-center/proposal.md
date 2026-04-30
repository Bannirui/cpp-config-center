## Why

Modern distributed systems require centralized, dynamic configuration management to eliminate the need for redeployment when configuration changes. While Apollo (Java) and Nacos (Java/Go) serve this need, there is no high-quality C++ native solution. A C++ implementation fills this gap for latency-sensitive, resource-constrained environments where Java/Go runtimes are impractical.

## What Changes

- **New**: C++ remote configuration center server with HTTP and gRPC APIs
- **New**: C++ client SDK for configuration subscription and hot-reload
- **New**: Configuration persistence with MySQL/SQLite backend and versioned change history
- **New**: Real-time config push via long-polling and gRPC streaming
- **New**: Multi-tenant isolation through namespaces (environment + application + cluster)
- **New**: Configuration publishing with optional approval workflow
- **New**: Web admin console for configuration management (optional, can reuse Apollo/Nacos UI)

## Capabilities

### New Capabilities

- `config-persistence`: Configuration storage layer with versioning, rollback, and change history. Supports MySQL and SQLite backends.
- `config-publish`: Configuration publishing flow with draft/release lifecycle, optional approval gating, and grayscale release support.
- `config-subscription`: Real-time configuration push from server to clients via long-polling and gRPC bidirectional streaming. Clients receive notifications on config change.
- `namespace-management`: Multi-level namespace isolation (application + environment + cluster) for organizing configurations across different services and deployment environments.
- `config-api`: RESTful HTTP API and gRPC service for all configuration operations — read, write, publish, subscribe, and administrative functions.
- `client-sdk`: C++ client library providing config cache, hot-reload callback, failover to local cache, and fallback to remote server.

### Modified Capabilities

<!-- None - this is a greenfield project -->

## Impact

- New C++ codebase with CMake build system
- Dependencies: Boost (ASIO, Beast), gRPC/Protobuf, MySQL client or SQLite, nlohmann/json, JWT (jwt-cpp)
- External HTTP API compatible with Apollo/Nacos client patterns for interoperability
- No impact on existing code — entirely new project

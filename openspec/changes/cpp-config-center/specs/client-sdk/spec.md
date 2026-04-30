## ADDED Requirements

### Requirement: Client initialization and connection
The C++ client SDK SHALL provide an initialization function that accepts the server address (host:port), application ID, and environment. The SDK MUST establish a connection to the server and authenticate if required.

#### Scenario: Initialize client
- **WHEN** a user calls `ConfigClient::Init("localhost:8080", "order-service", "DEV")`
- **THEN** the SDK connects to the server, authenticates (if configured), and returns success

#### Scenario: Initialization fails gracefully
- **WHEN** a user calls `ConfigClient::Init()` with an unreachable server address
- **THEN** the SDK returns an error code or throws a descriptive exception, and logs the failure

### Requirement: Local config cache
The SDK SHALL maintain an in-memory cache of all fetched config values. On config read, the SDK MUST return the cached value immediately without a network round-trip.

#### Scenario: Cache-first read
- **WHEN** a user calls `client.GetConfig("timeout")` and the config has been previously fetched
- **THEN** the SDK returns the cached value immediately (sub-millisecond latency)

#### Scenario: First read fetches from server
- **WHEN** a user calls `client.GetConfig("timeout")` and the config has never been fetched
- **THEN** the SDK fetches the value from the server, caches it, and returns it

### Requirement: Config hot-reload via callback
The SDK SHALL allow users to register callbacks that are invoked when a watched config key changes. The callback MUST receive the key name, old value, and new value.

#### Scenario: Hot-reload callback triggered
- **WHEN** a user registers callback `onConfigChange(string key, string old_val, string new_val)` for key `timeout`, and the server publishes a new value for that key
- **THEN** the callback is invoked with `key="timeout"`, `old_val="30"`, `new_val="60"`

#### Scenario: Multiple callbacks for same key
- **WHEN** two different callbacks are registered for the same config key `timeout`
- **THEN** both callbacks are invoked in registration order when the key changes

### Requirement: Subscription management
The SDK SHALL automatically subscribe to all namespaces associated with the configured application and environment. The SDK MUST use gRPC streaming (primary) with long-polling fallback (secondary) for receiving change notifications.

#### Scenario: Auto-subscribe on init
- **WHEN** the SDK initializes with application `order-service` and environment `DEV`
- **THEN** the SDK subscribes to all namespaces under `order-service/DEV/*` and begins receiving change notifications

### Requirement: Local file cache failover
The SDK SHALL persist fetched configs to a local file cache. On startup, if the server is unreachable, the SDK MUST load configs from the local file cache and operate in offline mode.

#### Scenario: Startup with unreachable server
- **WHEN** the SDK initializes and the server is unreachable, but a local cache file exists
- **THEN** the SDK loads configs from the local cache file, returns `Init()` with a warning status, and retries server connection in the background

#### Scenario: Startup with no cache and no server
- **WHEN** the SDK initializes with no server and no local cache file
- **THEN** the SDK returns an error and config reads return empty/error values

### Requirement: Stale-while-revalidate
The SDK SHALL continue serving cached config values even if the server becomes temporarily unreachable (stale-while-revalidate pattern). The SDK MUST attempt to reconnect in the background and update the cache once reconnected.

#### Scenario: Server becomes unreachable at runtime
- **WHEN** the server goes down after the SDK has an established cache
- **THEN** `GetConfig()` calls continue returning cached values, the SDK logs the disconnection, and reconnection is attempted every 5 seconds

### Requirement: Config value type support
The SDK SHALL support retrieving config values as `string`, `int`, `double`, `bool`, and `json` (parsed JSON object). Type conversion errors MUST be reported clearly.

#### Scenario: Read config as integer
- **WHEN** a user calls `client.GetConfigInt("max_connections")` and the server value is `"100"`
- **THEN** the SDK returns integer `100`

#### Scenario: Type conversion failure
- **WHEN** a user calls `client.GetConfigInt("timeout")` and the server value is `"thirty"`
- **THEN** the SDK returns an error indicating the value cannot be parsed as an integer

### Requirement: Thread safety
The SDK SHALL be thread-safe. Multiple threads MUST be able to call `GetConfig()` and register callbacks concurrently without data races or deadlocks.

#### Scenario: Concurrent reads
- **WHEN** 10 threads simultaneously call `GetConfig("timeout")`
- **THEN** all calls return the correct cached value without crashes or races

### Requirement: Graceful shutdown
The SDK SHALL provide a `Shutdown()` method that closes server connections, stops background threads, and flushes the local cache to disk.

#### Scenario: Shutdown flushes cache
- **WHEN** a user calls `client.Shutdown()`
- **THEN** all connections are closed, background threads are joined, the cache is written to disk, and the method returns

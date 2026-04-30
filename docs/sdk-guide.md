# Client SDK Usage Guide

## Overview

The C++ Client SDK provides cache-first config reads, hot-reload callbacks, and failover to local cache. It connects to the Config Center server via gRPC (primary) or HTTP long-polling (fallback).

## Installation

### CMake Integration

```cmake
find_package(config-center-sdk REQUIRED)
target_link_libraries(my_app PRIVATE config-center::sdk)
```

Or include as a subdirectory:
```cmake
add_subdirectory(path/to/config-center/sdk)
target_link_libraries(my_app PRIVATE config-center-sdk)
```

### Header-Only Mode

The SDK can also be used header-only by defining `CC_SDK_HEADER_ONLY` before including.

## Quick Start

```cpp
#include <config-center/config_client.h>
#include <iostream>

using namespace config_center::sdk;

int main() {
    ConfigClient client;

    // Initialize
    auto result = client.Init("localhost:8080", "order-service", "DEV");
    if (result == InitResult::Failed) {
        std::cerr << "Failed to initialize config client" << std::endl;
        return 1;
    }

    // Read config (cache-first)
    auto timeout = client.GetConfig("timeout");
    if (timeout) {
        std::cout << "Timeout: " << *timeout << std::endl;
    }

    // Typed access
    auto maxConn = client.GetConfigInt("max_connections");
    if (maxConn) {
        std::cout << "Max connections: " << *maxConn << std::endl;
    }

    // Cleanup
    client.Shutdown();
    return 0;
}
```

## API Reference

### Initialization

```cpp
enum class InitResult {
    Success = 0,
    ServerUnreachable = 1,
    ServerUnreachableUsingCache = 2,
    Failed = 3,
};

InitResult ConfigClient::Init(
    const std::string& serverAddress,  // "host:port"
    const std::string& appId,          // application ID
    const std::string& env             // DEV/FAT/UAT/PROD
);
```

- `Success`: Connected to server, cache loaded
- `ServerUnreachableUsingCache`: Server unreachable, loaded from local file cache
- `Failed`: No server and no cache available

### Shutdown

```cpp
void ConfigClient::Shutdown();
```

Closes connections, flushes cache to disk, joins background threads.

### Reading Config

```cpp
// String value
std::optional<std::string> GetConfig(const std::string& key);

// Typed accessors
std::optional<int> GetConfigInt(const std::string& key);
std::optional<double> GetConfigDouble(const std::string& key);
std::optional<bool> GetConfigBool(const std::string& key);
std::optional<json> GetConfigJson(const std::string& key);
```

All reads go through the local cache first. On cache miss, the SDK fetches from the server synchronously.

#### Type Conversion Rules

- `GetConfigBool`: Accepts `"true"`, `"false"`, `"1"`, `"0"` (case-insensitive)
- `GetConfigInt`: Standard `std::stoi` parsing
- `GetConfigDouble`: Standard `std::stod` parsing
- `GetConfigJson`: Standard `nlohmann::json::parse`

Returns `std::nullopt` on parse failure.

### Hot-Reload Callbacks

```cpp
using ConfigChangeCallback = std::function<void(
    const std::string& key,
    const std::string& oldValue,
    const std::string& newValue
)>;

void RegisterCallback(const std::string& key, ConfigChangeCallback callback);
void RemoveCallbacks(const std::string& key);
```

Example:
```cpp
client.RegisterCallback("timeout", [](const std::string& key,
                                       const std::string& oldVal,
                                       const std::string& newVal) {
    std::cout << key << " changed from " << oldVal << " to " << newVal << std::endl;
    // Update your application's timeout setting
});
```

Multiple callbacks per key are supported and invoked in registration order.

### Listing All Keys

```cpp
std::vector<std::string> GetAllKeys();
```

Returns all cached config keys.

## Behavior

### Cache-First Reads

Every `GetConfig*` call:
1. Checks in-memory cache
2. If cache hit: returns immediately (sub-millisecond)
3. If cache miss: fetches from server, caches, returns

### Stale-While-Revalidate

If the server becomes unreachable:
- `GetConfig()` continues returning cached values
- Background thread attempts reconnection with exponential backoff
- Logs disconnection/reconnection events

### Local File Cache

- On shutdown: cache serialized to `~/.config-center/cache/{appId}-{env}.json`
- On startup with unreachable server: loads from file cache
- Always prefers server data when available

### Thread Safety

All public methods are thread-safe (uses `std::shared_mutex` for read-write locking). Multiple threads can concurrently call `GetConfig()` and `RegisterCallback()`.

### Reconnection

Exponential backoff: 1s → 2s → 4s → ... → 60s max. Resets on successful reconnection.

## Complete Example

```cpp
#include <config-center/config_client.h>
#include <iostream>
#include <thread>

using namespace config_center::sdk;

class MyService {
public:
    void start() {
        auto result = client_.Init("config-center:8080", "my-service", "PROD");
        if (result == InitResult::Failed) {
            throw std::runtime_error("Cannot initialize config");
        }

        // Register hot-reload callbacks
        client_.RegisterCallback("timeout", [this](auto& key, auto& old, auto& newVal) {
            onTimeoutChanged(newVal);
        });
        client_.RegisterCallback("log_level", [this](auto& key, auto& old, auto& newVal) {
            onLogLevelChanged(newVal);
        });

        // Initial config load
        loadConfig();
    }

    void stop() {
        client_.Shutdown();
    }

private:
    void loadConfig() {
        if (auto val = client_.GetConfigInt("port")) port_ = *val;
        if (auto val = client_.GetConfigInt("timeout")) timeout_ = *val;
        if (auto val = client_.GetConfig("log_level")) level_ = *val;
    }

    void onTimeoutChanged(const std::string& newVal) {
        timeout_ = std::stoi(newVal);
        std::cout << "Timeout updated to: " << timeout_ << std::endl;
    }

    void onLogLevelChanged(const std::string& newVal) {
        level_ = newVal;
        std::cout << "Log level updated to: " << level_ << std::endl;
    }

    ConfigClient client_;
    int port_ = 8080;
    int timeout_ = 30;
    std::string level_ = "info";
};
```

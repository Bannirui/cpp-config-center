#include "config-center/config_client.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace config_center {
namespace sdk {

// Subscription Manager (gRPC-based with long-polling fallback)
class ConfigClient::SubscriptionManager {
public:
    SubscriptionManager(ConfigClient* client) : client_(client) {}
    void start() {}
    void stop() {}
    void reconnect() {}

private:
    ConfigClient* client_;
};

// Reconnect Policy (exponential backoff)
class ConfigClient::ReconnectPolicy {
public:
    ReconnectPolicy() : backoffSeconds_(1), maxBackoff_(60) {}

    int nextDelay() {
        int delay = backoffSeconds_;
        backoffSeconds_ = std::min(backoffSeconds_ * 2, maxBackoff_);
        return delay;
    }

    void reset() { backoffSeconds_ = 1; }

private:
    int backoffSeconds_;
    int maxBackoff_;
};

ConfigClient::ConfigClient() {
    subscriptionManager_ = std::make_unique<SubscriptionManager>(this);
    reconnectPolicy_ = std::make_unique<ReconnectPolicy>();
}

ConfigClient::~ConfigClient() {
    Shutdown();
}

InitResult ConfigClient::Init(const std::string& serverAddress, const std::string& appId, const std::string& env) {
    serverAddress_ = serverAddress;
    appId_ = appId;
    env_ = env;

    #ifdef _WIN32
    cacheFilePath_ = "config-cache-" + appId_ + "-" + env_ + ".json";
    #else
    const char* home = getenv("HOME");
    if (home) {
        cacheFilePath_ = std::string(home) + "/.config-center/cache/" + appId_ + "-" + env_ + ".json";
    } else {
        cacheFilePath_ = "config-cache-" + appId_ + "-" + env_ + ".json";
    }
    #endif

    // Try to connect to server
    bool serverReachable = false;
    // In real implementation: attempt gRPC/HTTP connection

    if (!serverReachable) {
        if (loadFromFileCache()) {
            subscribeToServer();
            initialized_ = true;
            return InitResult::ServerUnreachableUsingCache;
        }
        return InitResult::Failed;
    }

    subscribeToServer();
    initialized_ = true;
    return InitResult::Success;
}

void ConfigClient::Shutdown() {
    if (!initialized_) return;
    saveToFileCache();
    if (subscriptionManager_) {
        subscriptionManager_->stop();
    }
    initialized_ = false;
}

// Typed accessors
std::optional<std::string> ConfigClient::GetConfig(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(cacheMutex_);
    auto it = configCache_.find(key);
    if (it != configCache_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> ConfigClient::GetConfigInt(const std::string& key) {
    auto val = GetConfig(key);
    if (!val) return std::nullopt;
    try {
        return std::stoi(*val);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> ConfigClient::GetConfigDouble(const std::string& key) {
    auto val = GetConfig(key);
    if (!val) return std::nullopt;
    try {
        return std::stod(*val);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> ConfigClient::GetConfigBool(const std::string& key) {
    auto val = GetConfig(key);
    if (!val) return std::nullopt;
    std::string lower = *val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1") return true;
    if (lower == "false" || lower == "0") return false;
    return std::nullopt;
}

std::optional<json> ConfigClient::GetConfigJson(const std::string& key) {
    auto val = GetConfig(key);
    if (!val) return std::nullopt;
    try {
        return json::parse(*val);
    } catch (...) {
        return std::nullopt;
    }
}

// Callback system
void ConfigClient::RegisterCallback(const std::string& key, ConfigChangeCallback callback) {
    std::unique_lock<std::shared_mutex> lock(callbackMutex_);
    callbacks_[key].push_back(std::move(callback));
}

void ConfigClient::RemoveCallbacks(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(callbackMutex_);
    callbacks_.erase(key);
}

std::vector<std::string> ConfigClient::GetAllKeys() {
    std::shared_lock<std::shared_mutex> lock(cacheMutex_);
    std::vector<std::string> keys;
    for (auto& pair : configCache_) {
        keys.push_back(pair.first);
    }
    return keys;
}

// Internal methods
void ConfigClient::fetchConfigFromServer(const std::string& key) {
    // In real implementation: make gRPC or HTTP call to fetch config
}

void ConfigClient::subscribeToServer() {
    if (subscriptionManager_) {
        subscriptionManager_->start();
    }
}

void ConfigClient::reconnectLoop() {
    while (initialized_) {
        if (reconnectPolicy_) {
            int delay = reconnectPolicy_->nextDelay();
            std::this_thread::sleep_for(std::chrono::seconds(delay));
        }
        // Attempt reconnection
    }
}

void ConfigClient::saveToFileCache() {
    if (cacheFilePath_.empty()) return;

    std::shared_lock<std::shared_mutex> lock(cacheMutex_);
    json cache;
    for (auto& pair : configCache_) {
        cache[pair.first] = pair.second;
    }

    std::ofstream file(cacheFilePath_);
    if (file.is_open()) {
        file << cache.dump(2);
    }
}

bool ConfigClient::loadFromFileCache() {
    std::ifstream file(cacheFilePath_);
    if (!file.is_open()) return false;

    try {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        json cache = json::parse(content);

        std::unique_lock<std::shared_mutex> lock(cacheMutex_);
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            configCache_[it.key()] = it.value().get<std::string>();
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace sdk
} // namespace config_center

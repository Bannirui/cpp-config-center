#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace config_center {
namespace sdk {

using json = nlohmann::json;
using ConfigChangeCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;

enum class InitResult {
    Success = 0,
    ServerUnreachable = 1,
    ServerUnreachableUsingCache = 2,
    Failed = 3,
};

class ConfigClient {
public:
    ConfigClient();
    ~ConfigClient();

    InitResult Init(const std::string& serverAddress, const std::string& appId, const std::string& env);
    void Shutdown();

    // Typed accessors
    std::optional<std::string> GetConfig(const std::string& key);
    std::optional<int> GetConfigInt(const std::string& key);
    std::optional<double> GetConfigDouble(const std::string& key);
    std::optional<bool> GetConfigBool(const std::string& key);
    std::optional<json> GetConfigJson(const std::string& key);

    // Callback system
    void RegisterCallback(const std::string& key, ConfigChangeCallback callback);
    void RemoveCallbacks(const std::string& key);

    // Get all keys
    std::vector<std::string> GetAllKeys();

private:
    void fetchConfigFromServer(const std::string& key);
    void subscribeToServer();
    void reconnectLoop();
    void saveToFileCache();
    bool loadFromFileCache();

    std::string serverAddress_;
    std::string appId_;
    std::string env_;
    bool initialized_ = false;

    mutable std::shared_mutex cacheMutex_;
    std::unordered_map<std::string, std::string> configCache_;

    std::shared_mutex callbackMutex_;
    std::unordered_map<std::string, std::vector<ConfigChangeCallback>> callbacks_;

    class SubscriptionManager;
    std::unique_ptr<SubscriptionManager> subscriptionManager_;

    class ReconnectPolicy;
    std::unique_ptr<ReconnectPolicy> reconnectPolicy_;

    std::string cacheFilePath_;
};

} // namespace sdk
} // namespace config_center

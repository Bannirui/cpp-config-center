#pragma once

#include "config-center/storage/config_store.h"

#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace config_center {

class NamespaceManager {
public:
    explicit NamespaceManager(std::shared_ptr<ConfigStore> store);

    // Application
    bool registerApplication(const std::string& appId, const std::string& name,
                              const std::optional<std::string>& description = std::nullopt);
    bool removeApplication(const std::string& appId);
    std::vector<Application> listApplications();
    std::optional<Application> getApplication(const std::string& appId);

    // Environment
    std::vector<std::string> listEnvironments();
    bool addCustomEnvironment(const std::string& envName);

    // Cluster
    bool createCluster(const std::string& appId, const std::string& env, const std::string& cluster);
    std::vector<std::string> listClusters(const std::string& appId, const std::string& env);

    // Namespace
    bool createNamespace(const std::string& appId, const std::string& env, const std::string& cluster,
                          const std::string& name, const std::string& type = "private",
                          const std::optional<std::string>& parentNamespaceId = std::nullopt);
    bool deleteNamespace(const std::string& namespaceId);
    std::optional<Namespace> getNamespace(const std::string& namespaceId);
    std::vector<Namespace> listNamespaces(const std::string& appId, const std::string& env, const std::string& cluster);

    // Namespace inheritance - config key fallback resolution
    std::optional<ConfigKey> resolveConfig(const std::string& namespaceId, const std::string& key);

    // Config key validation
    static bool validateKeyName(const std::string& key, std::string& error);
    static bool validateValueSize(const std::string& value, size_t maxBytes, std::string& error);

private:
    std::string makeNamespaceId(const std::string& appId, const std::string& env, const std::string& cluster,
                                 const std::string& name);
    std::string getTimestamp();

    static const std::vector<std::string> kDefaultEnvironments;
    static const std::regex kKeyNamePattern;
    static const size_t kMaxValueBytes = 1048576;

    std::shared_ptr<ConfigStore> store_;
    std::vector<std::string> customEnvironments_;
};

} // namespace config_center

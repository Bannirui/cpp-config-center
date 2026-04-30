#include "config-center/storage/namespace_manager.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace config_center {

const std::vector<std::string> NamespaceManager::kDefaultEnvironments = {"DEV", "FAT", "UAT", "PROD"};
const std::regex NamespaceManager::kKeyNamePattern(R"([a-zA-Z0-9._-]+)");

NamespaceManager::NamespaceManager(std::shared_ptr<ConfigStore> store)
    : store_(std::move(store)) {
}

std::string NamespaceManager::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string NamespaceManager::makeNamespaceId(const std::string& appId, const std::string& env,
                                               const std::string& cluster, const std::string& name) {
    return appId + "/" + env + "/" + cluster + "/" + name;
}

// Application
bool NamespaceManager::registerApplication(const std::string& appId, const std::string& name,
                                            const std::optional<std::string>& description) {
    auto existing = store_->getApplication(appId);
    if (existing) return false;

    Application app;
    app.app_id = appId;
    app.name = name;
    app.description = description;
    app.created_at = getTimestamp();
    return store_->createApplication(app);
}

bool NamespaceManager::removeApplication(const std::string& appId) {
    return store_->deleteApplication(appId);
}

std::vector<Application> NamespaceManager::listApplications() {
    return store_->listApplications();
}

std::optional<Application> NamespaceManager::getApplication(const std::string& appId) {
    return store_->getApplication(appId);
}

// Environment
std::vector<std::string> NamespaceManager::listEnvironments() {
    std::vector<std::string> envs = kDefaultEnvironments;
    envs.insert(envs.end(), customEnvironments_.begin(), customEnvironments_.end());
    return envs;
}

bool NamespaceManager::addCustomEnvironment(const std::string& envName) {
    if (std::find(kDefaultEnvironments.begin(), kDefaultEnvironments.end(), envName) != kDefaultEnvironments.end()) {
        return false;
    }
    if (std::find(customEnvironments_.begin(), customEnvironments_.end(), envName) != customEnvironments_.end()) {
        return false;
    }
    customEnvironments_.push_back(envName);
    return true;
}

// Cluster
bool NamespaceManager::createCluster(const std::string& appId, const std::string& env, const std::string& cluster) {
    auto app = store_->getApplication(appId);
    if (!app) return false;

    auto existing = store_->listNamespaces(appId, env, cluster);
    if (!existing.empty()) return true; // cluster already exists

    // Create a default 'application' namespace for the cluster
    Namespace ns;
    ns.id = makeNamespaceId(appId, env, cluster, "application");
    ns.app_id = appId;
    ns.env = env;
    ns.cluster = cluster;
    ns.name = "application";
    ns.type = "private";
    ns.created_at = getTimestamp();
    return store_->createNamespace(ns);
}

std::vector<std::string> NamespaceManager::listClusters(const std::string& appId, const std::string& env) {
    auto namespaces = store_->listNamespaces(appId, env, "");
    std::vector<std::string> clusters;
    for (auto& ns : namespaces) {
        if (std::find(clusters.begin(), clusters.end(), ns.cluster) == clusters.end()) {
            clusters.push_back(ns.cluster);
        }
    }
    if (clusters.empty()) {
        clusters.push_back("default");
    }
    return clusters;
}

// Namespace
bool NamespaceManager::createNamespace(const std::string& appId, const std::string& env, const std::string& cluster,
                                        const std::string& name, const std::string& type,
                                        const std::optional<std::string>& parentNamespaceId) {
    auto app = store_->getApplication(appId);
    if (!app) return false;

    Namespace ns;
    ns.id = makeNamespaceId(appId, env, cluster, name);
    ns.app_id = appId;
    ns.env = env;
    ns.cluster = cluster;
    ns.name = name;
    ns.type = type;
    ns.parent_namespace_id = parentNamespaceId;
    ns.approval_enabled = false;
    ns.created_at = getTimestamp();
    return store_->createNamespace(ns);
}

bool NamespaceManager::deleteNamespace(const std::string& namespaceId) {
    return store_->deleteNamespace(namespaceId);
}

std::optional<Namespace> NamespaceManager::getNamespace(const std::string& namespaceId) {
    return store_->getNamespace(namespaceId);
}

std::vector<Namespace> NamespaceManager::listNamespaces(const std::string& appId, const std::string& env,
                                                         const std::string& cluster) {
    return store_->listNamespaces(appId, env, cluster);
}

// Namespace inheritance - config key fallback resolution
std::optional<ConfigKey> NamespaceManager::resolveConfig(const std::string& namespaceId, const std::string& key) {
    auto config = store_->getConfig(namespaceId, key, false);
    if (config) return config;

    auto ns = store_->getNamespace(namespaceId);
    if (!ns) return std::nullopt;

    if (ns->parent_namespace_id) {
        return resolveConfig(*ns->parent_namespace_id, key);
    }

    return std::nullopt;
}

// Config key validation
bool NamespaceManager::validateKeyName(const std::string& key, std::string& error) {
    if (key.empty()) {
        error = "Key name cannot be empty";
        return false;
    }
    if (key.length() > 128) {
        error = "Key name exceeds maximum length of 128 characters";
        return false;
    }
    if (!std::regex_match(key, kKeyNamePattern)) {
        error = "Key name contains invalid characters. Allowed: [a-zA-Z0-9._-]";
        return false;
    }
    return true;
}

bool NamespaceManager::validateValueSize(const std::string& value, size_t maxBytes, std::string& error) {
    if (value.size() > maxBytes) {
        error = "Config value exceeds maximum size of " + std::to_string(maxBytes) + " bytes";
        return false;
    }
    return true;
}

} // namespace config_center

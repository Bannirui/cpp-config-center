#pragma once

#include "config-center/storage/data_types.h"

#include <optional>
#include <string>
#include <vector>

namespace config_center {

class ConfigStore {
public:
    virtual ~ConfigStore() = default;

    // Lifecycle
    virtual bool initialize(const std::string& connectionString) = 0;
    virtual bool healthCheck() = 0;

    // Application
    virtual bool createApplication(const Application& app) = 0;
    virtual std::optional<Application> getApplication(const std::string& appId) = 0;
    virtual std::vector<Application> listApplications() = 0;
    virtual bool deleteApplication(const std::string& appId) = 0;

    // Namespace
    virtual bool createNamespace(const Namespace& ns) = 0;
    virtual std::optional<Namespace> getNamespace(const std::string& nsId) = 0;
    virtual std::vector<Namespace> listNamespaces(const std::string& appId, const std::string& env,
                                                   const std::string& cluster) = 0;
    virtual bool deleteNamespace(const std::string& nsId) = 0;

    // Config
    virtual bool createConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                               const std::string& operatorId) = 0;
    virtual bool updateConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                               const std::string& operatorId, bool draft = true) = 0;
    virtual bool deleteConfig(const std::string& namespaceId, const std::string& key, const std::string& operatorId,
                               bool draft = true) = 0;
    virtual std::optional<ConfigKey> getConfig(const std::string& namespaceId, const std::string& key,
                                                bool includeDraft = false) = 0;
    virtual std::vector<ConfigKey> listConfigs(const std::string& namespaceId, bool includeDraft = false) = 0;

    // Config Versioning
    virtual std::vector<ConfigVersion> getConfigVersions(const std::string& namespaceId, const std::string& key) = 0;
    virtual std::optional<ConfigVersion> getConfigVersion(const std::string& namespaceId, const std::string& key,
                                                           int32_t version) = 0;
    virtual bool rollbackConfig(const std::string& namespaceId, const std::string& key, int32_t targetVersion,
                                 const std::string& operatorId) = 0;

    // Change History
    virtual std::vector<ChangeLog> getChangeHistory(const ChangeLogQuery& query) = 0;

    // Release
    virtual bool createRelease(Release& release) = 0;
    virtual std::vector<Release> getReleases(const ReleaseQuery& query) = 0;
    virtual std::optional<Release> getRelease(int64_t releaseId) = 0;
    virtual bool retireRelease(int64_t releaseId) = 0;

    // Publish (transactional)
    virtual bool publishRelease(const std::string& namespaceId, const std::string& releaseNote,
                                 const std::string& operatorId, Release& outRelease) = 0;

    // Approval
    virtual bool createApproval(Approval& approval) = 0;
    virtual bool updateApprovalStatus(int64_t approvalId, const std::string& status, const std::string& comment) = 0;
    virtual std::vector<Approval> getPendingApprovals(const std::string& namespaceId) = 0;

    // User
    virtual bool createUser(const User& user) = 0;
    virtual std::optional<User> getUserByUsername(const std::string& username) = 0;
    virtual std::vector<User> listUsers() = 0;

    // Transaction support
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;
};

} // namespace config_center

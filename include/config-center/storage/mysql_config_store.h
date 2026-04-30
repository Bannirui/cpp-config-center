#pragma once

#include "config-center/storage/config_store.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace config_center {

class MysqlConnection {
public:
    virtual ~MysqlConnection() = default;
    virtual bool connect(const std::string& host, uint16_t port, const std::string& user, const std::string& password,
                         const std::string& database) = 0;
    virtual bool execute(const std::string& sql) = 0;
    virtual bool query(const std::string& sql, std::vector<std::vector<std::string>>& rows) = 0;
    virtual bool beginTransaction() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
    virtual bool isConnected() = 0;
    virtual void close() = 0;
};

class MysqlConnectionPool {
public:
    MysqlConnectionPool(size_t poolSize, const std::string& host, uint16_t port, const std::string& user,
                         const std::string& password, const std::string& database);
    ~MysqlConnectionPool();

    std::shared_ptr<MysqlConnection> acquire();
    void release(std::shared_ptr<MysqlConnection> conn);

private:
    std::queue<std::shared_ptr<MysqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t poolSize_;
};

class MysqlConfigStore : public ConfigStore {
public:
    explicit MysqlConfigStore() = default;
    ~MysqlConfigStore() override = default;

    bool initialize(const std::string& connectionString) override;
    bool healthCheck() override;

    bool createApplication(const Application& app) override;
    std::optional<Application> getApplication(const std::string& appId) override;
    std::vector<Application> listApplications() override;
    bool deleteApplication(const std::string& appId) override;

    bool createNamespace(const Namespace& ns) override;
    std::optional<Namespace> getNamespace(const std::string& nsId) override;
    std::vector<Namespace> listNamespaces(const std::string& appId, const std::string& env,
                                           const std::string& cluster) override;
    bool deleteNamespace(const std::string& nsId) override;

    bool createConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                       const std::string& operatorId) override;
    bool updateConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                       const std::string& operatorId, bool draft = true) override;
    bool deleteConfig(const std::string& namespaceId, const std::string& key, const std::string& operatorId,
                       bool draft = true) override;
    std::optional<ConfigKey> getConfig(const std::string& namespaceId, const std::string& key,
                                        bool includeDraft = false) override;
    std::vector<ConfigKey> listConfigs(const std::string& namespaceId, bool includeDraft = false) override;

    std::vector<ConfigVersion> getConfigVersions(const std::string& namespaceId, const std::string& key) override;
    std::optional<ConfigVersion> getConfigVersion(const std::string& namespaceId, const std::string& key,
                                                   int32_t version) override;
    bool rollbackConfig(const std::string& namespaceId, const std::string& key, int32_t targetVersion,
                         const std::string& operatorId) override;

    std::vector<ChangeLog> getChangeHistory(const ChangeLogQuery& query) override;

    bool createRelease(Release& release) override;
    std::vector<Release> getReleases(const ReleaseQuery& query) override;
    std::optional<Release> getRelease(int64_t releaseId) override;
    bool retireRelease(int64_t releaseId) override;

    bool publishRelease(const std::string& namespaceId, const std::string& releaseNote,
                         const std::string& operatorId, Release& outRelease) override;

    bool createApproval(Approval& approval) override;
    bool updateApprovalStatus(int64_t approvalId, const std::string& status, const std::string& comment) override;
    std::vector<Approval> getPendingApprovals(const std::string& namespaceId) override;

    bool createUser(const User& user) override;
    std::optional<User> getUserByUsername(const std::string& username) override;
    std::vector<User> listUsers() override;

    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private:
    bool autoMigrate();
    std::string escape(const std::string& value);
    std::string getTimestamp();

    std::unique_ptr<MysqlConnectionPool> pool_;
    std::string host_;
    uint16_t port_ = 3306;
    std::string user_;
    std::string password_;
    std::string database_;
};

} // namespace config_center

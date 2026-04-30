#pragma once

#include "config-center/storage/config_store.h"

#include <memory>
#include <mutex>
#include <sqlite_orm/sqlite_orm.h>

namespace config_center {

namespace orm = sqlite_orm;

inline auto createStorage(const std::string& path) {
    using namespace sqlite_orm;

    return make_storage(
        path,
        make_table("applications",
                   make_column("app_id", &Application::app_id, primary_key()),
                   make_column("name", &Application::name),
                   make_column("description", &Application::description),
                   make_column("created_at", &Application::created_at)),
        make_table("namespaces",
                   make_column("id", &Namespace::id, primary_key()),
                   make_column("app_id", &Namespace::app_id),
                   make_column("env", &Namespace::env),
                   make_column("cluster", &Namespace::cluster),
                   make_column("name", &Namespace::name),
                   make_column("type", &Namespace::type),
                   make_column("parent_namespace_id", &Namespace::parent_namespace_id),
                   make_column("approval_enabled", &Namespace::approval_enabled),
                   make_column("created_at", &Namespace::created_at),
                   foreign_key(&Namespace::app_id).references(&Application::app_id)),
        make_table("configs",
                   make_column("namespace_id", &ConfigKey::namespace_id),
                   make_column("key", &ConfigKey::key),
                   make_column("value", &ConfigKey::value),
                   make_column("version", &ConfigKey::version),
                   make_column("status", &ConfigKey::status),
                   make_column("created_at", &ConfigKey::created_at),
                   make_column("updated_at", &ConfigKey::updated_at),
                   primary_key(&ConfigKey::namespace_id, &ConfigKey::key)),
        make_table("config_versions",
                   make_column("version", &ConfigVersion::version),
                   make_column("config_id", &ConfigVersion::config_id),
                   make_column("value", &ConfigVersion::value),
                   make_column("change_type", &ConfigVersion::change_type),
                   make_column("operator_id", &ConfigVersion::operator_id),
                   make_column("created_at", &ConfigVersion::created_at)),
        make_table("change_logs",
                   make_column("id", &ChangeLog::id, autoincrement(), primary_key()),
                   make_column("config_id", &ChangeLog::config_id),
                   make_column("namespace_id", &ChangeLog::namespace_id),
                   make_column("key", &ChangeLog::key),
                   make_column("version", &ChangeLog::version),
                   make_column("old_value", &ChangeLog::old_value),
                   make_column("new_value", &ChangeLog::new_value),
                   make_column("operation", &ChangeLog::operation),
                   make_column("operator_id", &ChangeLog::operator_id),
                   make_column("created_at", &ChangeLog::created_at)),
        make_table("releases",
                   make_column("id", &Release::id, autoincrement(), primary_key()),
                   make_column("namespace_id", &Release::namespace_id),
                   make_column("release_version", &Release::release_version),
                   make_column("release_note", &Release::release_note),
                   make_column("release_type", &Release::release_type),
                   make_column("status", &Release::status),
                   make_column("grayscale_percentage", &Release::grayscale_percentage),
                   make_column("whitelist", &Release::whitelist),
                   make_column("operator_id", &Release::operator_id),
                   make_column("created_at", &Release::created_at)),
        make_table("approvals",
                   make_column("id", &Approval::id, autoincrement(), primary_key()),
                   make_column("namespace_id", &Approval::namespace_id),
                   make_column("submitter_id", &Approval::submitter_id),
                   make_column("approver_id", &Approval::approver_id),
                   make_column("status", &Approval::status),
                   make_column("comment", &Approval::comment),
                   make_column("created_at", &Approval::created_at),
                   make_column("updated_at", &Approval::updated_at)),
        make_table("users",
                   make_column("id", &User::id, autoincrement(), primary_key()),
                   make_column("username", &User::username, unique()),
                   make_column("password_hash", &User::password_hash),
                   make_column("role", &User::role),
                   make_column("created_at", &User::created_at)));
}

class SqliteConfigStore : public ConfigStore {
public:
    explicit SqliteConfigStore() = default;
    ~SqliteConfigStore() override = default;

    bool initialize(const std::string& connectionString) override;
    bool healthCheck() override;

    // Application
    bool createApplication(const Application& app) override;
    std::optional<Application> getApplication(const std::string& appId) override;
    std::vector<Application> listApplications() override;
    bool deleteApplication(const std::string& appId) override;

    // Namespace
    bool createNamespace(const Namespace& ns) override;
    std::optional<Namespace> getNamespace(const std::string& nsId) override;
    std::vector<Namespace> listNamespaces(const std::string& appId, const std::string& env,
                                           const std::string& cluster) override;
    bool deleteNamespace(const std::string& nsId) override;

    // Config
    bool createConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                       const std::string& operatorId) override;
    bool updateConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                       const std::string& operatorId, bool draft = true) override;
    bool deleteConfig(const std::string& namespaceId, const std::string& key, const std::string& operatorId,
                       bool draft = true) override;
    std::optional<ConfigKey> getConfig(const std::string& namespaceId, const std::string& key,
                                        bool includeDraft = false) override;
    std::vector<ConfigKey> listConfigs(const std::string& namespaceId, bool includeDraft = false) override;

    // Config Versioning
    std::vector<ConfigVersion> getConfigVersions(const std::string& namespaceId, const std::string& key) override;
    std::optional<ConfigVersion> getConfigVersion(const std::string& namespaceId, const std::string& key,
                                                   int32_t version) override;
    bool rollbackConfig(const std::string& namespaceId, const std::string& key, int32_t targetVersion,
                         const std::string& operatorId) override;

    // Change History
    std::vector<ChangeLog> getChangeHistory(const ChangeLogQuery& query) override;

    // Release
    bool createRelease(Release& release) override;
    std::vector<Release> getReleases(const ReleaseQuery& query) override;
    std::optional<Release> getRelease(int64_t releaseId) override;
    bool retireRelease(int64_t releaseId) override;

    // Publish
    bool publishRelease(const std::string& namespaceId, const std::string& releaseNote,
                         const std::string& operatorId, Release& outRelease) override;

    // Approval
    bool createApproval(Approval& approval) override;
    bool updateApprovalStatus(int64_t approvalId, const std::string& status, const std::string& comment) override;
    std::vector<Approval> getPendingApprovals(const std::string& namespaceId) override;

    // User
    bool createUser(const User& user) override;
    std::optional<User> getUserByUsername(const std::string& username) override;
    std::vector<User> listUsers() override;

    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private:
    std::string makeConfigId(const std::string& namespaceId, const std::string& key);
    std::string getTimestamp();

    std::unique_ptr<decltype(createStorage(""))> storage_;
    std::mutex mutex_;
};

} // namespace config_center

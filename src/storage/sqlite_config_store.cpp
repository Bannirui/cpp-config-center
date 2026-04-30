#include "config-center/storage/sqlite_config_store.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace config_center {

std::string SqliteConfigStore::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string SqliteConfigStore::makeConfigId(const std::string& namespaceId, const std::string& key) {
    return namespaceId + ":" + key;
}

bool SqliteConfigStore::initialize(const std::string& connectionString) {
    storage_ = std::make_unique<decltype(createStorage(""))>(createStorage(connectionString));
    storage_->sync_schema(true);
    return true;
}

bool SqliteConfigStore::healthCheck() {
    try {
        auto guard = storage_->get_allocator();
        (void)guard;
        return true;
    } catch (...) {
        return false;
    }
}

// Application
bool SqliteConfigStore::createApplication(const Application& app) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        storage_->insert(app);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<Application> SqliteConfigStore::getApplication(const std::string& appId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results = storage_->get_all<Application>(sqlite_orm::where(c(&Application::app_id) == appId));
    if (results.empty()) return std::nullopt;
    return results[0];
}

std::vector<Application> SqliteConfigStore::listApplications() {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_->get_all<Application>();
}

bool SqliteConfigStore::deleteApplication(const std::string& appId) {
    std::lock_guard<std::mutex> lock(mutex_);
    storage_->remove_all<Application>(sqlite_orm::where(c(&Application::app_id) == appId));
    return true;
}

// Namespace
bool SqliteConfigStore::createNamespace(const Namespace& ns) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        storage_->insert(ns);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<Namespace> SqliteConfigStore::getNamespace(const std::string& nsId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results = storage_->get_all<Namespace>(sqlite_orm::where(c(&Namespace::id) == nsId));
    if (results.empty()) return std::nullopt;
    return results[0];
}

std::vector<Namespace> SqliteConfigStore::listNamespaces(const std::string& appId, const std::string& env,
                                                          const std::string& cluster) {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_->get_all<Namespace>(sqlite_orm::where(
        c(&Namespace::app_id) == appId && c(&Namespace::env) == env && c(&Namespace::cluster) == cluster));
}

bool SqliteConfigStore::deleteNamespace(const std::string& nsId) {
    std::lock_guard<std::mutex> lock(mutex_);
    storage_->remove_all<Namespace>(sqlite_orm::where(c(&Namespace::id) == nsId));
    return true;
}

// Config
bool SqliteConfigStore::createConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                                      const std::string& operatorId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto ts = getTimestamp();

    ConfigKey config;
    config.namespace_id = namespaceId;
    config.key = key;
    config.value = value;
    config.version = 1;
    config.status = "draft";
    config.created_at = ts;
    config.updated_at = ts;

    try {
        storage_->insert(config);
    } catch (...) {
        return false;
    }

    ConfigVersion cv;
    cv.version = 1;
    cv.config_id = makeConfigId(namespaceId, key);
    cv.value = value;
    cv.change_type = "create";
    cv.operator_id = operatorId;
    cv.created_at = ts;
    storage_->insert(cv);

    ChangeLog cl;
    cl.config_id = makeConfigId(namespaceId, key);
    cl.namespace_id = namespaceId;
    cl.key = key;
    cl.version = 1;
    cl.new_value = value;
    cl.operation = "create";
    cl.operator_id = operatorId;
    cl.created_at = ts;
    storage_->insert(cl);

    return true;
}

bool SqliteConfigStore::updateConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                                      const std::string& operatorId, bool draft) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto ts = getTimestamp();
    auto results =
        storage_->get_all<ConfigKey>(sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId && c(&ConfigKey::key) == key));
    if (results.empty()) return false;

    auto& config = results[0];
    std::string oldValue = config.value;
    int32_t newVersion = config.version + 1;

    config.value = value;
    config.version = newVersion;
    config.status = draft ? "draft" : "live";
    config.updated_at = ts;

    storage_->update(config);

    ConfigVersion cv;
    cv.version = newVersion;
    cv.config_id = makeConfigId(namespaceId, key);
    cv.value = value;
    cv.change_type = "update";
    cv.operator_id = operatorId;
    cv.created_at = ts;
    storage_->insert(cv);

    ChangeLog cl;
    cl.config_id = makeConfigId(namespaceId, key);
    cl.namespace_id = namespaceId;
    cl.key = key;
    cl.version = newVersion;
    cl.old_value = oldValue;
    cl.new_value = value;
    cl.operation = "update";
    cl.operator_id = operatorId;
    cl.created_at = ts;
    storage_->insert(cl);

    return true;
}

bool SqliteConfigStore::deleteConfig(const std::string& namespaceId, const std::string& key,
                                      const std::string& operatorId, bool draft) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto ts = getTimestamp();

    if (draft) {
        storage_->remove_all<ConfigKey>(
            sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId && c(&ConfigKey::key) == key
                               && c(&ConfigKey::status) == "draft"));
    } else {
        storage_->remove_all<ConfigKey>(
            sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId && c(&ConfigKey::key) == key));
    }

    ChangeLog cl;
    cl.config_id = makeConfigId(namespaceId, key);
    cl.namespace_id = namespaceId;
    cl.key = key;
    cl.operation = "delete";
    cl.operator_id = operatorId;
    cl.created_at = ts;
    storage_->insert(cl);

    return true;
}

std::optional<ConfigKey> SqliteConfigStore::getConfig(const std::string& namespaceId, const std::string& key,
                                                       bool includeDraft) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results =
        storage_->get_all<ConfigKey>(sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId && c(&ConfigKey::key) == key));
    if (results.empty()) return std::nullopt;

    if (!includeDraft && results[0].status == "draft") {
        return std::nullopt;
    }

    return results[0];
}

std::vector<ConfigKey> SqliteConfigStore::listConfigs(const std::string& namespaceId, bool includeDraft) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results =
        storage_->get_all<ConfigKey>(sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId));
    if (!includeDraft) {
        results.erase(std::remove_if(results.begin(), results.end(),
                                      [](const ConfigKey& c) { return c.status == "draft"; }),
                      results.end());
    }
    return results;
}

// Config Versioning
std::vector<ConfigVersion> SqliteConfigStore::getConfigVersions(const std::string& namespaceId, const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_->get_all<ConfigVersion>(
        sqlite_orm::where(c(&ConfigVersion::config_id) == makeConfigId(namespaceId, key)),
        sqlite_orm::order_by(&ConfigVersion::version).desc());
}

std::optional<ConfigVersion> SqliteConfigStore::getConfigVersion(const std::string& namespaceId, const std::string& key,
                                                                  int32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results = storage_->get_all<ConfigVersion>(
        sqlite_orm::where(c(&ConfigVersion::config_id) == makeConfigId(namespaceId, key)
                           && c(&ConfigVersion::version) == version));
    if (results.empty()) return std::nullopt;
    return results[0];
}

bool SqliteConfigStore::rollbackConfig(const std::string& namespaceId, const std::string& key, int32_t targetVersion,
                                        const std::string& operatorId) {
    auto targetConfigVersion = getConfigVersion(namespaceId, key, targetVersion);
    if (!targetConfigVersion) return false;

    return updateConfig(namespaceId, key, targetConfigVersion->value, operatorId, true);
}

// Change History
std::vector<ChangeLog> SqliteConfigStore::getChangeHistory(const ChangeLogQuery& query) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto whereClause = sqlite_orm::where(sqlite_orm::c(1) == 1);

    if (query.key) {
        whereClause = whereClause && c(&ChangeLog::key) == *query.key;
    }
    if (query.namespace_id) {
        whereClause = whereClause && c(&ChangeLog::namespace_id) == *query.namespace_id;
    }
    if (query.time_from) {
        whereClause = whereClause && c(&ChangeLog::created_at) >= *query.time_from;
    }
    if (query.time_to) {
        whereClause = whereClause && c(&ChangeLog::created_at) <= *query.time_to;
    }

    int offset = (query.page - 1) * query.page_size;
    return storage_->get_all<ChangeLog>(
        whereClause, sqlite_orm::order_by(&ChangeLog::created_at).desc(),
        sqlite_orm::limit(query.page_size), sqlite_orm::offset(offset));
}

// Release
bool SqliteConfigStore::createRelease(Release& release) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        auto id = storage_->insert(release);
        release.id = id;
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<Release> SqliteConfigStore::getReleases(const ReleaseQuery& query) {
    std::lock_guard<std::mutex> lock(mutex_);
    int offset = (query.page - 1) * query.page_size;
    return storage_->get_all<Release>(
        sqlite_orm::where(c(&Release::namespace_id) == query.namespace_id),
        sqlite_orm::order_by(&Release::created_at).desc(), sqlite_orm::limit(query.page_size),
        sqlite_orm::offset(offset));
}

std::optional<Release> SqliteConfigStore::getRelease(int64_t releaseId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results = storage_->get_all<Release>(sqlite_orm::where(c(&Release::id) == releaseId));
    if (results.empty()) return std::nullopt;
    return results[0];
}

bool SqliteConfigStore::retireRelease(int64_t releaseId) {
    std::lock_guard<std::mutex> lock(mutex_);
    storage_->update_all(sqlite_orm::set(c(&Release::status) = "retired"),
                          sqlite_orm::where(c(&Release::id) == releaseId));
    return true;
}

// Publish
bool SqliteConfigStore::publishRelease(const std::string& namespaceId, const std::string& releaseNote,
                                        const std::string& operatorId, Release& outRelease) {
    std::lock_guard<std::mutex> lock(mutex_);

    beginTransaction();

    try {
        auto draftConfigs = storage_->get_all<ConfigKey>(
            sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId && c(&ConfigKey::status) == "draft"));

        storage_->update_all(sqlite_orm::set(c(&ConfigKey::status) = "live"),
                              sqlite_orm::where(c(&ConfigKey::namespace_id) == namespaceId));

        int maxVersion = 0;
        auto releases = storage_->get_all<Release>(
            sqlite_orm::where(c(&Release::namespace_id) == namespaceId),
            sqlite_orm::order_by(&Release::release_version).desc(), sqlite_orm::limit(1));
        if (!releases.empty()) {
            maxVersion = releases[0].release_version;
        }

        Release release;
        release.namespace_id = namespaceId;
        release.release_version = maxVersion + 1;
        release.release_note = releaseNote;
        release.release_type = "full";
        release.status = "active";
        release.operator_id = operatorId;
        release.created_at = getTimestamp();

        auto id = storage_->insert(release);
        release.id = id;
        outRelease = release;

        commitTransaction();
        return true;
    } catch (...) {
        rollbackTransaction();
        return false;
    }
}

// Approval
bool SqliteConfigStore::createApproval(Approval& approval) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        auto id = storage_->insert(approval);
        approval.id = id;
        return true;
    } catch (...) {
        return false;
    }
}

bool SqliteConfigStore::updateApprovalStatus(int64_t approvalId, const std::string& status, const std::string& comment) {
    std::lock_guard<std::mutex> lock(mutex_);
    storage_->update_all(sqlite_orm::set(c(&Approval::status) = status, c(&Approval::comment) = comment,
                                          c(&Approval::updated_at) = getTimestamp()),
                          sqlite_orm::where(c(&Approval::id) == approvalId));
    return true;
}

std::vector<Approval> SqliteConfigStore::getPendingApprovals(const std::string& namespaceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_->get_all<Approval>(
        sqlite_orm::where(c(&Approval::namespace_id) == namespaceId && c(&Approval::status) == "pending"));
}

// User
bool SqliteConfigStore::createUser(const User& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        storage_->insert(user);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<User> SqliteConfigStore::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto results = storage_->get_all<User>(sqlite_orm::where(c(&User::username) == username));
    if (results.empty()) return std::nullopt;
    return results[0];
}

std::vector<User> SqliteConfigStore::listUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    return storage_->get_all<User>();
}

bool SqliteConfigStore::beginTransaction() {
    storage_->begin_transaction();
    return true;
}

bool SqliteConfigStore::commitTransaction() {
    storage_->commit();
    return true;
}

bool SqliteConfigStore::rollbackTransaction() {
    storage_->rollback();
    return true;
}

} // namespace config_center

#include "config-center/storage/mysql_config_store.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace config_center {

// Connection Pool Implementation
MysqlConnectionPool::MysqlConnectionPool(size_t poolSize, const std::string& host, uint16_t port, const std::string& user,
                                           const std::string& password, const std::string& database)
    : poolSize_(poolSize) {
    for (size_t i = 0; i < poolSize; ++i) {
        auto conn = std::make_shared<MysqlConnection>();
        conn->connect(host, port, user, password, database);
        pool_.push(conn);
    }
}

MysqlConnectionPool::~MysqlConnectionPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pool_.empty()) {
        pool_.front()->close();
        pool_.pop();
    }
}

std::shared_ptr<MysqlConnection> MysqlConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !pool_.empty(); });
    auto conn = pool_.front();
    pool_.pop();
    return conn;
}

void MysqlConnectionPool::release(std::shared_ptr<MysqlConnection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(conn);
    cv_.notify_one();
}

// Helpers
std::string MysqlConfigStore::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string MysqlConfigStore::escape(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == '\'') escaped += "\\'";
        else if (c == '\\') escaped += "\\\\";
        else escaped += c;
    }
    return escaped;
}

bool MysqlConfigStore::initialize(const std::string& connectionString) {
    // Parse connection string: host:port/user:password@database
    size_t atPos = connectionString.find('@');
    size_t slashPos = connectionString.find('/');
    size_t colonPos = connectionString.find(':');

    host_ = connectionString.substr(0, slashPos);
    port_ = 3306;

    size_t portStart = host_.find(':');
    if (portStart != std::string::npos) {
        port_ = static_cast<uint16_t>(std::stoi(host_.substr(portStart + 1)));
        host_ = host_.substr(0, portStart);
    }

    size_t colonUserPwd = connectionString.find(':', slashPos + 1);
    if (colonUserPwd != std::string::npos) {
        user_ = connectionString.substr(slashPos + 1, colonUserPwd - slashPos - 1);
        password_ = connectionString.substr(colonUserPwd + 1, atPos - colonUserPwd - 1);
    }

    database_ = connectionString.substr(atPos + 1);

    pool_ = std::make_unique<MysqlConnectionPool>(10, host_, port_, user_, password_, database_);
    return autoMigrate();
}

bool MysqlConfigStore::autoMigrate() {
    auto conn = pool_->acquire();

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS applications (
            app_id VARCHAR(255) PRIMARY KEY,
            name VARCHAR(255) NOT NULL,
            description TEXT,
            created_at DATETIME NOT NULL
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS namespaces (
            id VARCHAR(255) PRIMARY KEY,
            app_id VARCHAR(255) NOT NULL,
            env VARCHAR(50) NOT NULL,
            cluster VARCHAR(100) NOT NULL DEFAULT 'default',
            name VARCHAR(255) NOT NULL,
            type VARCHAR(50) NOT NULL,
            parent_namespace_id VARCHAR(255),
            approval_enabled BOOLEAN DEFAULT FALSE,
            created_at DATETIME NOT NULL,
            FOREIGN KEY (app_id) REFERENCES applications(app_id) ON DELETE CASCADE
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS configs (
            namespace_id VARCHAR(255) NOT NULL,
            `key` VARCHAR(255) NOT NULL,
            value TEXT NOT NULL,
            version INT NOT NULL DEFAULT 1,
            status VARCHAR(20) NOT NULL DEFAULT 'live',
            created_at DATETIME NOT NULL,
            updated_at DATETIME NOT NULL,
            PRIMARY KEY (namespace_id, `key`),
            FOREIGN KEY (namespace_id) REFERENCES namespaces(id) ON DELETE CASCADE
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS config_versions (
            version INT NOT NULL,
            config_id VARCHAR(512) NOT NULL,
            value TEXT NOT NULL,
            change_type VARCHAR(50) NOT NULL,
            operator_id VARCHAR(255) NOT NULL,
            created_at DATETIME NOT NULL
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS change_logs (
            id BIGINT AUTO_INCREMENT PRIMARY KEY,
            config_id VARCHAR(512) NOT NULL,
            namespace_id VARCHAR(255) NOT NULL,
            `key` VARCHAR(255) NOT NULL,
            version INT NOT NULL,
            old_value TEXT,
            new_value TEXT,
            operation VARCHAR(50) NOT NULL,
            operator_id VARCHAR(255) NOT NULL,
            created_at DATETIME NOT NULL
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS releases (
            id BIGINT AUTO_INCREMENT PRIMARY KEY,
            namespace_id VARCHAR(255) NOT NULL,
            release_version INT NOT NULL,
            release_note TEXT,
            release_type VARCHAR(50) NOT NULL DEFAULT 'full',
            status VARCHAR(50) NOT NULL DEFAULT 'active',
            grayscale_percentage INT,
            whitelist TEXT,
            operator_id VARCHAR(255) NOT NULL,
            created_at DATETIME NOT NULL,
            FOREIGN KEY (namespace_id) REFERENCES namespaces(id) ON DELETE CASCADE
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS approvals (
            id BIGINT AUTO_INCREMENT PRIMARY KEY,
            namespace_id VARCHAR(255) NOT NULL,
            submitter_id VARCHAR(255) NOT NULL,
            approver_id VARCHAR(255),
            status VARCHAR(50) NOT NULL DEFAULT 'pending',
            comment TEXT,
            created_at DATETIME NOT NULL,
            updated_at DATETIME,
            FOREIGN KEY (namespace_id) REFERENCES namespaces(id) ON DELETE CASCADE
        )
    )");

    conn->execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id BIGINT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(255) UNIQUE NOT NULL,
            password_hash VARCHAR(255) NOT NULL,
            role VARCHAR(50) NOT NULL DEFAULT 'viewer',
            created_at DATETIME NOT NULL
        )
    )");

    pool_->release(conn);
    return true;
}

bool MysqlConfigStore::healthCheck() {
    auto conn = pool_->acquire();
    bool ok = conn->isConnected();
    pool_->release(conn);
    return ok;
}

// Application
bool MysqlConfigStore::createApplication(const Application& app) {
    auto conn = pool_->acquire();
    std::string sql = "INSERT INTO applications (app_id, name, description, created_at) VALUES ('" +
                      escape(app.app_id) + "', '" + escape(app.name) + "', " +
                      (app.description ? "'" + escape(*app.description) + "'" : "NULL") + ", '" +
                      escape(app.created_at) + "')";
    bool ok = conn->execute(sql);
    pool_->release(conn);
    return ok;
}

std::optional<Application> MysqlConfigStore::getApplication(const std::string& appId) {
    auto conn = pool_->acquire();
    std::string sql = "SELECT app_id, name, description, created_at FROM applications WHERE app_id = '" +
                      escape(appId) + "'";
    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    Application app;
    app.app_id = rows[0][0];
    app.name = rows[0][1];
    if (!rows[0][2].empty()) app.description = rows[0][2];
    app.created_at = rows[0][3];
    return app;
}

std::vector<Application> MysqlConfigStore::listApplications() {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query("SELECT app_id, name, description, created_at FROM applications", rows);
    pool_->release(conn);

    std::vector<Application> apps;
    for (auto& row : rows) {
        Application app;
        app.app_id = row[0];
        app.name = row[1];
        if (!row[2].empty()) app.description = row[2];
        app.created_at = row[3];
        apps.push_back(app);
    }
    return apps;
}

bool MysqlConfigStore::deleteApplication(const std::string& appId) {
    auto conn = pool_->acquire();
    bool ok = conn->execute("DELETE FROM applications WHERE app_id = '" + escape(appId) + "'");
    pool_->release(conn);
    return ok;
}

// Namespace
bool MysqlConfigStore::createNamespace(const Namespace& ns) {
    auto conn = pool_->acquire();
    std::string sql = "INSERT INTO namespaces (id, app_id, env, cluster, name, type, parent_namespace_id, "
                      "approval_enabled, created_at) VALUES ('" +
                      escape(ns.id) + "', '" + escape(ns.app_id) + "', '" + escape(ns.env) + "', '" +
                      escape(ns.cluster) + "', '" + escape(ns.name) + "', '" + escape(ns.type) + "', " +
                      (ns.parent_namespace_id ? "'" + escape(*ns.parent_namespace_id) + "'" : "NULL") + ", " +
                      (ns.approval_enabled ? (*ns.approval_enabled ? "TRUE" : "FALSE") : "FALSE") + ", '" +
                      escape(ns.created_at) + "')";
    bool ok = conn->execute(sql);
    pool_->release(conn);
    return ok;
}

std::optional<Namespace> MysqlConfigStore::getNamespace(const std::string& nsId) {
    auto conn = pool_->acquire();
    std::string sql = "SELECT id, app_id, env, cluster, name, type, parent_namespace_id, approval_enabled, created_at "
                      "FROM namespaces WHERE id = '" +
                      escape(nsId) + "'";
    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    Namespace ns;
    ns.id = rows[0][0];
    ns.app_id = rows[0][1];
    ns.env = rows[0][2];
    ns.cluster = rows[0][3];
    ns.name = rows[0][4];
    ns.type = rows[0][5];
    if (!rows[0][6].empty()) ns.parent_namespace_id = rows[0][6];
    if (!rows[0][7].empty()) ns.approval_enabled = (rows[0][7] == "1" || rows[0][7] == "true");
    ns.created_at = rows[0][8];
    return ns;
}

std::vector<Namespace> MysqlConfigStore::listNamespaces(const std::string& appId, const std::string& env,
                                                         const std::string& cluster) {
    auto conn = pool_->acquire();
    std::string sql =
        "SELECT id, app_id, env, cluster, name, type, parent_namespace_id, approval_enabled, created_at "
        "FROM namespaces WHERE app_id = '" +
        escape(appId) + "' AND env = '" + escape(env) + "' AND cluster = '" + escape(cluster) + "'";
    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    std::vector<Namespace> namespaces;
    for (auto& row : rows) {
        Namespace ns;
        ns.id = row[0];
        ns.app_id = row[1];
        ns.env = row[2];
        ns.cluster = row[3];
        ns.name = row[4];
        ns.type = row[5];
        if (!row[6].empty()) ns.parent_namespace_id = row[6];
        if (!row[7].empty()) ns.approval_enabled = (row[7] == "1" || row[7] == "true");
        ns.created_at = row[8];
        namespaces.push_back(ns);
    }
    return namespaces;
}

bool MysqlConfigStore::deleteNamespace(const std::string& nsId) {
    auto conn = pool_->acquire();
    bool ok = conn->execute("DELETE FROM namespaces WHERE id = '" + escape(nsId) + "'");
    pool_->release(conn);
    return ok;
}

// Config
bool MysqlConfigStore::createConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                                     const std::string& operatorId) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    std::string configId = namespaceId + ":" + key;

    std::string sql = "INSERT INTO configs (namespace_id, `key`, value, version, status, created_at, updated_at) VALUES ('" +
                      escape(namespaceId) + "', '" + escape(key) + "', '" + escape(value) +
                      "', 1, 'draft', '" + ts + "', '" + ts + "')";
    conn->execute(sql);

    conn->execute("INSERT INTO config_versions (version, config_id, value, change_type, operator_id, created_at) VALUES (1, '" +
                  escape(configId) + "', '" + escape(value) + "', 'create', '" + escape(operatorId) + "', '" + ts +
                  "')");
    conn->execute("INSERT INTO change_logs (config_id, namespace_id, `key`, version, new_value, operation, operator_id, "
                  "created_at) VALUES ('" +
                  escape(configId) + "', '" + escape(namespaceId) + "', '" + escape(key) +
                  "', 1, '" + escape(value) + "', 'create', '" + escape(operatorId) + "', '" + ts + "')");

    pool_->release(conn);
    return true;
}

bool MysqlConfigStore::updateConfig(const std::string& namespaceId, const std::string& key, const std::string& value,
                                     const std::string& operatorId, bool draft) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    std::string configId = namespaceId + ":" + key;

    // Get current version and value
    std::vector<std::vector<std::string>> rows;
    conn->query("SELECT version, value FROM configs WHERE namespace_id = '" + escape(namespaceId) + "' AND `key` = '" +
                    escape(key) + "'",
                rows);
    if (rows.empty()) {
        pool_->release(conn);
        return false;
    }

    int32_t oldVersion = std::stoi(rows[0][0]);
    std::string oldValue = rows[0][1];
    int32_t newVersion = oldVersion + 1;

    std::string status = draft ? "draft" : "live";
    conn->execute("UPDATE configs SET value = '" + escape(value) + "', version = " + std::to_string(newVersion) +
                  ", status = '" + status + "', updated_at = '" + ts + "' WHERE namespace_id = '" +
                  escape(namespaceId) + "' AND `key` = '" + escape(key) + "'");

    conn->execute("INSERT INTO config_versions (version, config_id, value, change_type, operator_id, created_at) VALUES (" +
                  std::to_string(newVersion) + ", '" + escape(configId) + "', '" + escape(value) +
                  "', 'update', '" + escape(operatorId) + "', '" + ts + "')");
    conn->execute(
        "INSERT INTO change_logs (config_id, namespace_id, `key`, version, old_value, new_value, operation, "
        "operator_id, created_at) VALUES ('" +
        escape(configId) + "', '" + escape(namespaceId) + "', '" + escape(key) + "', " + std::to_string(newVersion) +
        ", '" + escape(oldValue) + "', '" + escape(value) + "', 'update', '" + escape(operatorId) + "', '" + ts + "')");

    pool_->release(conn);
    return true;
}

bool MysqlConfigStore::deleteConfig(const std::string& namespaceId, const std::string& key,
                                     const std::string& operatorId, bool draft) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    std::string configId = namespaceId + ":" + key;

    if (draft) {
        conn->execute("DELETE FROM configs WHERE namespace_id = '" + escape(namespaceId) + "' AND `key` = '" +
                      escape(key) + "' AND status = 'draft'");
    } else {
        conn->execute("DELETE FROM configs WHERE namespace_id = '" + escape(namespaceId) + "' AND `key` = '" +
                      escape(key) + "'");
    }

    conn->execute("INSERT INTO change_logs (config_id, namespace_id, `key`, version, operation, operator_id, "
                  "created_at) VALUES ('" +
                  escape(configId) + "', '" + escape(namespaceId) + "', '" + escape(key) +
                  "', 0, 'delete', '" + escape(operatorId) + "', '" + ts + "')");

    pool_->release(conn);
    return true;
}

std::optional<ConfigKey> MysqlConfigStore::getConfig(const std::string& namespaceId, const std::string& key,
                                                      bool includeDraft) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query("SELECT namespace_id, `key`, value, version, status, created_at, updated_at FROM configs WHERE namespace_id = '" +
                    escape(namespaceId) + "' AND `key` = '" + escape(key) + "'",
                rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    if (!includeDraft && rows[0][4] == "draft") return std::nullopt;

    ConfigKey config;
    config.namespace_id = rows[0][0];
    config.key = rows[0][1];
    config.value = rows[0][2];
    config.version = std::stoi(rows[0][3]);
    config.status = rows[0][4];
    config.created_at = rows[0][5];
    config.updated_at = rows[0][6];
    return config;
}

std::vector<ConfigKey> MysqlConfigStore::listConfigs(const std::string& namespaceId, bool includeDraft) {
    auto conn = pool_->acquire();
    std::string sql =
        "SELECT namespace_id, `key`, value, version, status, created_at, updated_at FROM configs WHERE namespace_id = '" +
        escape(namespaceId) + "'";
    if (!includeDraft) {
        sql += " AND status = 'live'";
    }
    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    std::vector<ConfigKey> configs;
    for (auto& row : rows) {
        ConfigKey config;
        config.namespace_id = row[0];
        config.key = row[1];
        config.value = row[2];
        config.version = std::stoi(row[3]);
        config.status = row[4];
        config.created_at = row[5];
        config.updated_at = row[6];
        configs.push_back(config);
    }
    return configs;
}

// Config Versioning
std::vector<ConfigVersion> MysqlConfigStore::getConfigVersions(const std::string& namespaceId, const std::string& key) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query(
        "SELECT version, config_id, value, change_type, operator_id, created_at FROM config_versions WHERE config_id = '" +
            escape(namespaceId + ":" + key) + "' ORDER BY version DESC",
        rows);
    pool_->release(conn);

    std::vector<ConfigVersion> versions;
    for (auto& row : rows) {
        ConfigVersion cv;
        cv.version = std::stoi(row[0]);
        cv.config_id = row[1];
        cv.value = row[2];
        cv.change_type = row[3];
        cv.operator_id = row[4];
        cv.created_at = row[5];
        versions.push_back(cv);
    }
    return versions;
}

std::optional<ConfigVersion> MysqlConfigStore::getConfigVersion(const std::string& namespaceId, const std::string& key,
                                                                 int32_t version) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query(
        "SELECT version, config_id, value, change_type, operator_id, created_at FROM config_versions WHERE config_id = '" +
            escape(namespaceId + ":" + key) + "' AND version = " + std::to_string(version),
        rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    ConfigVersion cv;
    cv.version = std::stoi(rows[0][0]);
    cv.config_id = rows[0][1];
    cv.value = rows[0][2];
    cv.change_type = rows[0][3];
    cv.operator_id = rows[0][4];
    cv.created_at = rows[0][5];
    return cv;
}

bool MysqlConfigStore::rollbackConfig(const std::string& namespaceId, const std::string& key, int32_t targetVersion,
                                       const std::string& operatorId) {
    auto target = getConfigVersion(namespaceId, key, targetVersion);
    if (!target) return false;
    return updateConfig(namespaceId, key, target->value, operatorId, true);
}

// Change History
std::vector<ChangeLog> MysqlConfigStore::getChangeHistory(const ChangeLogQuery& query) {
    auto conn = pool_->acquire();
    std::string sql =
        "SELECT id, config_id, namespace_id, `key`, version, old_value, new_value, operation, operator_id, "
        "created_at FROM change_logs WHERE 1=1";

    if (query.key) sql += " AND `key` = '" + escape(*query.key) + "'";
    if (query.namespace_id) sql += " AND namespace_id = '" + escape(*query.namespace_id) + "'";
    if (query.time_from) sql += " AND created_at >= '" + escape(*query.time_from) + "'";
    if (query.time_to) sql += " AND created_at <= '" + escape(*query.time_to) + "'";

    sql += " ORDER BY created_at DESC LIMIT " + std::to_string(query.page_size) + " OFFSET " +
           std::to_string((query.page - 1) * query.page_size);

    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    std::vector<ChangeLog> logs;
    for (auto& row : rows) {
        ChangeLog cl;
        cl.id = std::stoll(row[0]);
        cl.config_id = row[1];
        cl.namespace_id = row[2];
        cl.key = row[3];
        cl.version = std::stoi(row[4]);
        cl.old_value = row[5];
        cl.new_value = row[6];
        cl.operation = row[7];
        cl.operator_id = row[8];
        cl.created_at = row[9];
        logs.push_back(cl);
    }
    return logs;
}

// Release
bool MysqlConfigStore::createRelease(Release& release) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    release.created_at = ts;

    std::string sql =
        "INSERT INTO releases (namespace_id, release_version, release_note, release_type, status, "
        "grayscale_percentage, whitelist, operator_id, created_at) VALUES ('" +
        escape(release.namespace_id) + "', " + std::to_string(release.release_version) + ", '" +
        escape(release.release_note) + "', '" + escape(release.release_type) + "', '" + escape(release.status) +
        "', " + (release.grayscale_percentage ? std::to_string(*release.grayscale_percentage) : "NULL") + ", " +
        (release.whitelist ? "'" + escape(*release.whitelist) + "'" : "NULL") + ", '" +
        escape(release.operator_id) + "', '" + ts + "')";
    conn->execute(sql);

    std::vector<std::vector<std::string>> idRows;
    conn->query("SELECT LAST_INSERT_ID()", idRows);
    if (!idRows.empty() && !idRows[0].empty()) {
        release.id = std::stoll(idRows[0][0]);
    }

    pool_->release(conn);
    return true;
}

std::vector<Release> MysqlConfigStore::getReleases(const ReleaseQuery& query) {
    auto conn = pool_->acquire();
    std::string sql =
        "SELECT id, namespace_id, release_version, release_note, release_type, status, grayscale_percentage, "
        "whitelist, operator_id, created_at FROM releases WHERE namespace_id = '" +
        escape(query.namespace_id) + "' ORDER BY created_at DESC LIMIT " + std::to_string(query.page_size) +
        " OFFSET " + std::to_string((query.page - 1) * query.page_size);

    std::vector<std::vector<std::string>> rows;
    conn->query(sql, rows);
    pool_->release(conn);

    std::vector<Release> releases;
    for (auto& row : rows) {
        Release r;
        r.id = std::stoll(row[0]);
        r.namespace_id = row[1];
        r.release_version = std::stoi(row[2]);
        r.release_note = row[3];
        r.release_type = row[4];
        r.status = row[5];
        if (!row[6].empty()) r.grayscale_percentage = std::stoi(row[6]);
        if (!row[7].empty()) r.whitelist = row[7];
        r.operator_id = row[8];
        r.created_at = row[9];
        releases.push_back(r);
    }
    return releases;
}

std::optional<Release> MysqlConfigStore::getRelease(int64_t releaseId) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query(
        "SELECT id, namespace_id, release_version, release_note, release_type, status, grayscale_percentage, "
        "whitelist, operator_id, created_at FROM releases WHERE id = " +
            std::to_string(releaseId),
        rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    Release r;
    r.id = std::stoll(rows[0][0]);
    r.namespace_id = rows[0][1];
    r.release_version = std::stoi(rows[0][2]);
    r.release_note = rows[0][3];
    r.release_type = rows[0][4];
    r.status = rows[0][5];
    if (!rows[0][6].empty()) r.grayscale_percentage = std::stoi(rows[0][6]);
    if (!rows[0][7].empty()) r.whitelist = rows[0][7];
    r.operator_id = rows[0][8];
    r.created_at = rows[0][9];
    return r;
}

bool MysqlConfigStore::retireRelease(int64_t releaseId) {
    auto conn = pool_->acquire();
    conn->execute("UPDATE releases SET status = 'retired' WHERE id = " + std::to_string(releaseId));
    pool_->release(conn);
    return true;
}

// Publish (transactional)
bool MysqlConfigStore::publishRelease(const std::string& namespaceId, const std::string& releaseNote,
                                       const std::string& operatorId, Release& outRelease) {
    auto conn = pool_->acquire();
    conn->beginTransaction();

    try {
        conn->execute("UPDATE configs SET status = 'live' WHERE namespace_id = '" + escape(namespaceId) + "'");

        std::vector<std::vector<std::string>> versionRows;
        conn->query(
            "SELECT MAX(release_version) FROM releases WHERE namespace_id = '" + escape(namespaceId) + "'",
            versionRows);
        int nextVersion = 1;
        if (!versionRows.empty() && !versionRows[0].empty() && !versionRows[0][0].empty()) {
            nextVersion = std::stoi(versionRows[0][0]) + 1;
        }

        Release release;
        release.namespace_id = namespaceId;
        release.release_version = nextVersion;
        release.release_note = releaseNote;
        release.release_type = "full";
        release.status = "active";
        release.operator_id = operatorId;
        release.created_at = getTimestamp();

        std::string sql =
            "INSERT INTO releases (namespace_id, release_version, release_note, release_type, status, "
            "operator_id, created_at) VALUES ('" +
            escape(namespaceId) + "', " + std::to_string(nextVersion) + ", '" + escape(releaseNote) +
            "', 'full', 'active', '" + escape(operatorId) + "', '" + release.created_at + "')";
        conn->execute(sql);

        std::vector<std::vector<std::string>> idRows;
        conn->query("SELECT LAST_INSERT_ID()", idRows);
        if (!idRows.empty() && !idRows[0].empty()) {
            release.id = std::stoll(idRows[0][0]);
        }

        outRelease = release;
        conn->commit();
        pool_->release(conn);
        return true;
    } catch (...) {
        conn->rollback();
        pool_->release(conn);
        return false;
    }
}

// Approval
bool MysqlConfigStore::createApproval(Approval& approval) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    approval.created_at = ts;

    std::string sql =
        "INSERT INTO approvals (namespace_id, submitter_id, approver_id, status, comment, created_at) VALUES ('" +
        escape(approval.namespace_id) + "', '" + escape(approval.submitter_id) + "', " +
        (approval.approver_id.empty() ? "NULL" : "'" + escape(approval.approver_id) + "'") + ", '" +
        escape(approval.status) + "', '" + escape(approval.comment) + "', '" + ts + "')";
    conn->execute(sql);

    std::vector<std::vector<std::string>> idRows;
    conn->query("SELECT LAST_INSERT_ID()", idRows);
    if (!idRows.empty() && !idRows[0].empty()) {
        approval.id = std::stoll(idRows[0][0]);
    }

    pool_->release(conn);
    return true;
}

bool MysqlConfigStore::updateApprovalStatus(int64_t approvalId, const std::string& status, const std::string& comment) {
    auto conn = pool_->acquire();
    auto ts = getTimestamp();
    conn->execute("UPDATE approvals SET status = '" + escape(status) + "', comment = '" + escape(comment) +
                  "', updated_at = '" + ts + "' WHERE id = " + std::to_string(approvalId));
    pool_->release(conn);
    return true;
}

std::vector<Approval> MysqlConfigStore::getPendingApprovals(const std::string& namespaceId) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query(
        "SELECT id, namespace_id, submitter_id, approver_id, status, comment, created_at, updated_at "
        "FROM approvals WHERE namespace_id = '" +
            escape(namespaceId) + "' AND status = 'pending'",
        rows);
    pool_->release(conn);

    std::vector<Approval> approvals;
    for (auto& row : rows) {
        Approval a;
        a.id = std::stoll(row[0]);
        a.namespace_id = row[1];
        a.submitter_id = row[2];
        a.approver_id = row[3];
        a.status = row[4];
        a.comment = row[5];
        a.created_at = row[6];
        a.updated_at = row[7];
        approvals.push_back(a);
    }
    return approvals;
}

// User
bool MysqlConfigStore::createUser(const User& user) {
    auto conn = pool_->acquire();
    std::string sql = "INSERT INTO users (username, password_hash, role, created_at) VALUES ('" +
                      escape(user.username) + "', '" + escape(user.password_hash) + "', '" + escape(user.role) +
                      "', '" + escape(user.created_at) + "')";
    bool ok = conn->execute(sql);
    pool_->release(conn);
    return ok;
}

std::optional<User> MysqlConfigStore::getUserByUsername(const std::string& username) {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query(
        "SELECT id, username, password_hash, role, created_at FROM users WHERE username = '" + escape(username) +
            "'",
        rows);
    pool_->release(conn);

    if (rows.empty()) return std::nullopt;
    User u;
    u.id = std::stoll(rows[0][0]);
    u.username = rows[0][1];
    u.password_hash = rows[0][2];
    u.role = rows[0][3];
    u.created_at = rows[0][4];
    return u;
}

std::vector<User> MysqlConfigStore::listUsers() {
    auto conn = pool_->acquire();
    std::vector<std::vector<std::string>> rows;
    conn->query("SELECT id, username, password_hash, role, created_at FROM users", rows);
    pool_->release(conn);

    std::vector<User> users;
    for (auto& row : rows) {
        User u;
        u.id = std::stoll(row[0]);
        u.username = row[1];
        u.password_hash = row[2];
        u.role = row[3];
        u.created_at = row[4];
        users.push_back(u);
    }
    return users;
}

bool MysqlConfigStore::beginTransaction() {
    auto conn = pool_->acquire();
    bool ok = conn->beginTransaction();
    return ok;
}

bool MysqlConfigStore::commitTransaction() {
    auto conn = pool_->acquire();
    bool ok = conn->commit();
    return ok;
}

bool MysqlConfigStore::rollbackTransaction() {
    auto conn = pool_->acquire();
    bool ok = conn->rollback();
    return ok;
}

} // namespace config_center

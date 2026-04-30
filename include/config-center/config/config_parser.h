#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace config_center {

struct ServerHttpConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8080;
    int threads = 0;
};

struct ServerGrpcConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 9090;
    int threads = 0;
};

struct ServerConfig {
    ServerHttpConfig http;
    ServerGrpcConfig grpc;
};

struct SqliteConfig {
    std::string path = "data/config-center.db";
};

struct MysqlConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "root";
    std::string password;
    std::string database = "config_center";
    int pool_size = 10;
    int timeout_seconds = 30;
};

struct StorageConfig {
    std::string backend = "sqlite";
    SqliteConfig sqlite;
    MysqlConfig mysql;
};

struct JwtConfig {
    std::string secret = "change-me-in-production";
    int expiry_hours = 24;
};

struct AuthConfig {
    bool enabled = true;
    JwtConfig jwt;
    bool read_auth_required = false;
};

struct NamespaceConfig {
    std::vector<std::string> default_environments = {"DEV", "FAT", "UAT", "PROD"};
};

struct ApprovalConfig {
    bool default_enabled = false;
};

struct GrayscaleConfig {
    int default_percentage = 100;
};

struct PublishConfig {
    ApprovalConfig approval;
    GrayscaleConfig grayscale;
};

struct LongPollingConfig {
    int max_timeout_seconds = 120;
    int default_timeout_seconds = 60;
};

struct GrpcNotifyConfig {
    int keepalive_seconds = 30;
};

struct NotificationConfig {
    LongPollingConfig long_polling;
    GrpcNotifyConfig grpc;
};

struct LimitsConfig {
    int config_key_max_length = 128;
    int config_value_max_bytes = 1048576;
    int release_note_max_length = 4096;
};

struct LoggingConfig {
    std::string level = "info";
    std::string file;
    std::string format = "text";
};

struct AppConfig {
    ServerConfig server;
    StorageConfig storage;
    AuthConfig auth;
    NamespaceConfig namespace_config;
    PublishConfig publish;
    NotificationConfig notification;
    LimitsConfig limits;
    LoggingConfig logging;
};

class ConfigParser {
public:
    static AppConfig parseFile(const std::string& path);
    static AppConfig parseString(const std::string& yamlContent);

private:
    static void parseServer(const YAML::Node& node, ServerConfig& config);
    static void parseStorage(const YAML::Node& node, StorageConfig& config);
    static void parseAuth(const YAML::Node& node, AuthConfig& config);
    static void parseNamespace(const YAML::Node& node, NamespaceConfig& config);
    static void parsePublish(const YAML::Node& node, PublishConfig& config);
    static void parseNotification(const YAML::Node& node, NotificationConfig& config);
    static void parseLimits(const YAML::Node& node, LimitsConfig& config);
    static void parseLogging(const YAML::Node& node, LoggingConfig& config);
};

} // namespace config_center

#include "config-center/config/config_parser.h"

#include <fstream>
#include <stdexcept>

namespace config_center {

AppConfig ConfigParser::parseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parseString(content);
}

AppConfig ConfigParser::parseString(const std::string& yamlContent) {
    YAML::Node root = YAML::Load(yamlContent);
    AppConfig config;

    if (root["server"]) {
        parseServer(root["server"], config.server);
    }
    if (root["storage"]) {
        parseStorage(root["storage"], config.storage);
    }
    if (root["auth"]) {
        parseAuth(root["auth"], config.auth);
    }
    if (root["namespace"]) {
        parseNamespace(root["namespace"], config.namespace_config);
    }
    if (root["publish"]) {
        parsePublish(root["publish"], config.publish);
    }
    if (root["notification"]) {
        parseNotification(root["notification"], config.notification);
    }
    if (root["limits"]) {
        parseLimits(root["limits"], config.limits);
    }
    if (root["logging"]) {
        parseLogging(root["logging"], config.logging);
    }

    return config;
}

void ConfigParser::parseServer(const YAML::Node& node, ServerConfig& config) {
    if (node["http"]) {
        auto& http = node["http"];
        if (http["host"]) config.http.host = http["host"].as<std::string>();
        if (http["port"]) config.http.port = http["port"].as<uint16_t>();
        if (http["threads"]) config.http.threads = http["threads"].as<int>();
    }
    if (node["grpc"]) {
        auto& grpc = node["grpc"];
        if (grpc["host"]) config.grpc.host = grpc["host"].as<std::string>();
        if (grpc["port"]) config.grpc.port = grpc["port"].as<uint16_t>();
        if (grpc["threads"]) config.grpc.threads = grpc["threads"].as<int>();
    }
}

void ConfigParser::parseStorage(const YAML::Node& node, StorageConfig& config) {
    if (node["backend"]) config.backend = node["backend"].as<std::string>();

    if (node["sqlite"]) {
        auto& sqlite = node["sqlite"];
        if (sqlite["path"]) config.sqlite.path = sqlite["path"].as<std::string>();
    }
    if (node["mysql"]) {
        auto& mysql = node["mysql"];
        if (mysql["host"]) config.mysql.host = mysql["host"].as<std::string>();
        if (mysql["port"]) config.mysql.port = mysql["port"].as<uint16_t>();
        if (mysql["user"]) config.mysql.user = mysql["user"].as<std::string>();
        if (mysql["password"]) config.mysql.password = mysql["password"].as<std::string>();
        if (mysql["database"]) config.mysql.database = mysql["database"].as<std::string>();
        if (mysql["pool_size"]) config.mysql.pool_size = mysql["pool_size"].as<int>();
        if (mysql["timeout_seconds"]) config.mysql.timeout_seconds = mysql["timeout_seconds"].as<int>();
    }
}

void ConfigParser::parseAuth(const YAML::Node& node, AuthConfig& config) {
    if (node["enabled"]) config.enabled = node["enabled"].as<bool>();
    if (node["read_auth_required"]) config.read_auth_required = node["read_auth_required"].as<bool>();

    if (node["jwt"]) {
        auto& jwt = node["jwt"];
        if (jwt["secret"]) config.jwt.secret = jwt["secret"].as<std::string>();
        if (jwt["expiry_hours"]) config.jwt.expiry_hours = jwt["expiry_hours"].as<int>();
    }
}

void ConfigParser::parseNamespace(const YAML::Node& node, NamespaceConfig& config) {
    if (node["default_environments"]) {
        config.default_environments.clear();
        for (const auto& env : node["default_environments"]) {
            config.default_environments.push_back(env.as<std::string>());
        }
    }
}

void ConfigParser::parsePublish(const YAML::Node& node, PublishConfig& config) {
    if (node["approval"] && node["approval"]["default_enabled"]) {
        config.approval.default_enabled = node["approval"]["default_enabled"].as<bool>();
    }
    if (node["grayscale"] && node["grayscale"]["default_percentage"]) {
        config.grayscale.default_percentage = node["grayscale"]["default_percentage"].as<int>();
    }
}

void ConfigParser::parseNotification(const YAML::Node& node, NotificationConfig& config) {
    if (node["long_polling"]) {
        auto& lp = node["long_polling"];
        if (lp["max_timeout_seconds"]) config.long_polling.max_timeout_seconds = lp["max_timeout_seconds"].as<int>();
        if (lp["default_timeout_seconds"])
            config.long_polling.default_timeout_seconds = lp["default_timeout_seconds"].as<int>();
    }
    if (node["grpc"] && node["grpc"]["keepalive_seconds"]) {
        config.grpc.keepalive_seconds = node["grpc"]["keepalive_seconds"].as<int>();
    }
}

void ConfigParser::parseLimits(const YAML::Node& node, LimitsConfig& config) {
    if (node["config_key_max_length"]) config.config_key_max_length = node["config_key_max_length"].as<int>();
    if (node["config_value_max_bytes"]) config.config_value_max_bytes = node["config_value_max_bytes"].as<int>();
    if (node["release_note_max_length"]) config.release_note_max_length = node["release_note_max_length"].as<int>();
}

void ConfigParser::parseLogging(const YAML::Node& node, LoggingConfig& config) {
    if (node["level"]) config.level = node["level"].as<std::string>();
    if (node["file"]) config.file = node["file"].as<std::string>();
    if (node["format"]) config.format = node["format"].as<std::string>();
}

} // namespace config_center

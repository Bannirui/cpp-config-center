#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace config_center {

struct Application {
    std::string app_id;
    std::string name;
    std::optional<std::string> description;
    std::string created_at;
};

struct Namespace {
    std::string id;
    std::string app_id;
    std::string env;
    std::string cluster;
    std::string name;
    std::string type; // "private", "public", "common"
    std::optional<std::string> parent_namespace_id;
    std::optional<bool> approval_enabled;
    std::string created_at;
};

struct ConfigKey {
    std::string namespace_id;
    std::string key;
    std::string value;
    int32_t version = 0;
    std::string status; // "live", "draft"
    std::string created_at;
    std::string updated_at;
};

struct ConfigVersion {
    int32_t version = 0;
    std::string config_id;
    std::string value;
    std::string change_type; // "create", "update", "rollback", "delete"
    std::string operator_id;
    std::string created_at;
};

struct ChangeLog {
    int64_t id = 0;
    std::string config_id;
    std::string namespace_id;
    std::string key;
    int32_t version = 0;
    std::string old_value;
    std::string new_value;
    std::string operation; // "create", "update", "rollback", "delete"
    std::string operator_id;
    std::string created_at;
};

struct Release {
    int64_t id = 0;
    std::string namespace_id;
    int32_t release_version = 0;
    std::string release_note;
    std::string release_type; // "full", "grayscale"
    std::string status; // "active", "retired"
    std::optional<int> grayscale_percentage;
    std::optional<std::string> whitelist; // JSON array of IPs/instance-ids
    std::string operator_id;
    std::string created_at;
};

struct Approval {
    int64_t id = 0;
    std::string namespace_id;
    std::string submitter_id;
    std::string approver_id;
    std::string status; // "pending", "approved", "rejected"
    std::string comment;
    std::string created_at;
    std::string updated_at;
};

struct User {
    int64_t id = 0;
    std::string username;
    std::string password_hash;
    std::string role; // "admin", "operator", "viewer"
    std::string created_at;
};

struct ChangeLogQuery {
    std::optional<std::string> key;
    std::optional<std::string> namespace_id;
    std::optional<std::string> time_from;
    std::optional<std::string> time_to;
    int page = 1;
    int page_size = 20;
};

struct ReleaseQuery {
    std::string namespace_id;
    int page = 1;
    int page_size = 20;
};

} // namespace config_center

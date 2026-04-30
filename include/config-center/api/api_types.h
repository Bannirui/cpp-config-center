#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

namespace config_center {

using json = nlohmann::json;

class ApiResponse {
public:
    static json error(int code, const std::string& message);
    static json success(const json& data);
    static json success();
};

class ConfigStore;
class NamespaceManager;
class PublishEngine;
class NotificationChannel;
class AuthMiddleware;
class JwtManager;

struct ApiContext {
    std::shared_ptr<ConfigStore> store;
    std::shared_ptr<NamespaceManager> namespaceManager;
    std::shared_ptr<PublishEngine> publishEngine;
    std::shared_ptr<NotificationChannel> notificationChannel;
    std::shared_ptr<AuthMiddleware> auth;
    std::shared_ptr<JwtManager> jwt;
};

} // namespace config_center

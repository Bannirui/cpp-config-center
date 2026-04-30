#include "config-center/api/config_handler.h"

namespace config_center {

json ConfigHandler::handleGetConfig(const std::string& namespaceId, const std::string& key) {
    auto config = context_.store->getConfig(namespaceId, key, false);
    if (!config) return ApiResponse::error(404, "Config not found");
    return json{{"key", config->key}, {"value", config->value}, {"version", config->version}};
}

json ConfigHandler::handleSetConfig(const std::string& namespaceId, const std::string& key, const json& body, const std::string& operatorId) {
    std::string value = body.value("value", "");

    auto existing = context_.store->getConfig(namespaceId, key, false);
    if (existing) {
        context_.store->updateConfig(namespaceId, key, value, operatorId, true);
    } else {
        context_.store->createConfig(namespaceId, key, value, operatorId);
    }
    return ApiResponse::success();
}

json ConfigHandler::handleDeleteConfig(const std::string& namespaceId, const std::string& key, const std::string& operatorId) {
    context_.store->deleteConfig(namespaceId, key, operatorId, true);
    return ApiResponse::success();
}

} // namespace config_center

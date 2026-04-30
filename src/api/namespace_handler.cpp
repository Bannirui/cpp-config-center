#include "config-center/api/namespace_handler.h"

namespace config_center {

json NamespaceHandler::handleCreateApp(const json& body) {
    if (!body.contains("app_id") || !body.contains("name")) {
        return ApiResponse::error(400, "Missing required fields: app_id, name");
    }
    bool ok = context_.namespaceManager->registerApplication(
        body["app_id"], body["name"],
        body.contains("description") ? std::optional<std::string>(body["description"]) : std::nullopt);
    if (!ok) return ApiResponse::error(409, "Application already exists");
    return json{{"app_id", body["app_id"]}, {"name", body["name"]}};
}

json NamespaceHandler::handleListApps() {
    auto apps = context_.namespaceManager->listApplications();
    json arr = json::array();
    for (auto& app : apps) {
        json obj = {{"app_id", app.app_id}, {"name", app.name}};
        if (app.description) obj["description"] = *app.description;
        obj["created_at"] = app.created_at;
        arr.push_back(obj);
    }
    return json{{"applications", arr}};
}

json NamespaceHandler::handleGetApp(const std::string& appId) {
    auto app = context_.namespaceManager->getApplication(appId);
    if (!app) return ApiResponse::error(404, "Application not found");
    json obj = {{"app_id", app->app_id}, {"name", app->name}, {"created_at", app->created_at}};
    if (app->description) obj["description"] = *app->description;
    return obj;
}

json NamespaceHandler::handleDeleteApp(const std::string& appId) {
    context_.namespaceManager->removeApplication(appId);
    return ApiResponse::success();
}

json NamespaceHandler::handleCreateNamespace(const json& body) {
    bool ok = context_.namespaceManager->createNamespace(
        body["app_id"], body["env"], body.value("cluster", "default"),
        body["name"], body.value("type", "private"),
        body.contains("parent_namespace_id")
            ? std::optional<std::string>(body["parent_namespace_id"])
            : std::nullopt);
    if (!ok) return ApiResponse::error(400, "Failed to create namespace");
    return json{{"id", body["app_id"].get<std::string>() + "/" + body["env"].get<std::string>() + "/" +
                             body.value("cluster", "default") + "/" + body["name"].get<std::string>()}};
}

json NamespaceHandler::handleListNamespaces(const std::string& appId) {
    auto namespaces = context_.store->listNamespaces(appId, "", "");
    json arr = json::array();
    for (auto& ns : namespaces) {
        json obj;
        obj["id"] = ns.id;
        obj["app_id"] = ns.app_id;
        obj["env"] = ns.env;
        obj["cluster"] = ns.cluster;
        obj["name"] = ns.name;
        obj["type"] = ns.type;
        if (ns.parent_namespace_id) obj["parent_namespace_id"] = *ns.parent_namespace_id;
        arr.push_back(obj);
    }
    return json{{"namespaces", arr}};
}

} // namespace config_center

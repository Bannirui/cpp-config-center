#include "config-center/server/http_server.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <nlohmann/json.hpp>

#include <iostream>

namespace config_center {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket socket, ApiContext context)
        : socket_(std::move(socket)), context_(std::move(context)) {
    }

    void run() {
        readRequest();
    }

private:
    void readRequest() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, request_,
                         [self](beast::error_code ec, std::size_t) {
                             if (!ec) {
                                 self->handleRequest();
                             }
                         });
    }

    void handleRequest() {
        auto const& target = std::string(request_.target());
        auto const method = request_.method();
        json responseBody;
        http::status status = http::status::ok;

        // Route: /health
        if (target == "/health") {
            responseBody = handleHealth();
        }
        // Route: /api/v1/auth/*
        else if (target == "/api/v1/auth/login" && method == http::verb::post) {
            responseBody = handleLogin();
        }
        // Route: /api/v1/apps
        else if (target == "/api/v1/apps") {
            if (method == http::verb::post) {
                responseBody = handleCreateApp();
            } else if (method == http::verb::get) {
                responseBody = handleListApps();
            }
        }
        // Route: /api/v1/apps/{appId}
        else if (target.find("/api/v1/apps/") == 0 && target.find("/namespaces") == std::string::npos) {
            auto appId = extractPathParam(target, "/api/v1/apps/");
            if (method == http::verb::get) {
                responseBody = handleGetApp(appId);
            } else if (method == http::verb::delete_) {
                responseBody = handleDeleteApp(appId);
            }
        }
        // Route: /api/v1/apps/{appId}/namespaces
        else if (target.find("/api/v1/apps/") == 0 && target.find("/namespaces") != std::string::npos) {
            auto appId = extractPathParam(target, "/api/v1/apps/", "/namespaces");
            if (method == http::verb::get) {
                responseBody = handleListNamespaces(appId);
            }
        }
        // Route: /api/v1/namespaces
        else if (target == "/api/v1/namespaces" && method == http::verb::post) {
            responseBody = handleCreateNamespace();
        }
        // Route: /api/v1/namespaces/{ns}/keys/{key}
        else if (target.find("/api/v1/namespaces/") == 0 && target.find("/keys/") != std::string::npos) {
            auto ns = extractPathParam(target, "/api/v1/namespaces/", "/keys/");
            auto key = extractPathParam(target, "/keys/");
            if (method == http::verb::get) {
                responseBody = handleGetConfig(ns, key);
            } else if (method == http::verb::put) {
                responseBody = handleSetConfig(ns, key);
            } else if (method == http::verb::delete_) {
                responseBody = handleDeleteConfig(ns, key);
            }
        }
        // Route: /api/v1/namespaces/{ns}/publish
        else if (target.find("/api/v1/namespaces/") == 0 && target.find("/publish") != std::string::npos) {
            auto ns = extractPathParam(target, "/api/v1/namespaces/", "/publish");
            responseBody = handlePublish(ns);
        }
        // Route: /api/v1/namespaces/{ns}/releases
        else if (target.find("/api/v1/namespaces/") == 0 && target.find("/releases") != std::string::npos) {
            auto ns = extractPathParam(target, "/api/v1/namespaces/", "/releases");
            responseBody = handleGetReleases(ns);
        }
        // Route: long-polling
        else if (target == "/api/v1/notifications") {
            responseBody = handleNotifications();
        }
        else {
            status = http::status::not_found;
            responseBody = ApiResponse::error(404, "Not found");
        }

        sendResponse(status, responseBody);
    }

    void sendResponse(http::status status, const json& body) {
        auto self = shared_from_this();
        http::response<http::string_body> res{status, request_.version()};
        res.set(http::field::content_type, "application/json");
        res.body() = body.dump();
        res.prepare_payload();

        auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(socket_, *sp,
                          [self, sp](beast::error_code ec, std::size_t) {
                              self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                          });
    }

    std::string extractPathParam(const std::string& url, const std::string& prefix,
                                  const std::string& suffix = "") {
        auto start = url.find(prefix);
        if (start == std::string::npos) return "";
        start += prefix.length();

        if (suffix.empty()) return url.substr(start);

        auto end = url.find(suffix, start);
        if (end == std::string::npos) return "";
        return url.substr(start, end - start);
    }

    // Handlers
    json handleHealth() {
        return json{
            {"status", "UP"},
            {"uptime_seconds", 0},
            {"db_connected", context_.store->healthCheck()}};
    }

    json handleLogin() {
        auto body = json::parse(request_.body());
        // Delegate to auth handler
        return ApiResponse::error(501, "Not implemented");
    }

    json handleCreateApp() {
        auto body = json::parse(request_.body());
        if (!body.contains("app_id") || !body.contains("name")) {
            return ApiResponse::error(400, "Missing required fields: app_id, name");
        }
        bool ok = context_.namespaceManager->registerApplication(
            body["app_id"], body["name"],
            body.contains("description") ? std::optional<std::string>(body["description"]) : std::nullopt);
        if (!ok) return ApiResponse::error(409, "Application already exists");
        return json{{"app_id", body["app_id"]}, {"name", body["name"]}};
    }

    json handleListApps() {
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

    json handleGetApp(const std::string& appId) {
        auto app = context_.namespaceManager->getApplication(appId);
        if (!app) return ApiResponse::error(404, "Application not found");
        json obj = {{"app_id", app->app_id}, {"name", app->name}, {"created_at", app->created_at}};
        if (app->description) obj["description"] = *app->description;
        return obj;
    }

    json handleDeleteApp(const std::string& appId) {
        context_.namespaceManager->removeApplication(appId);
        return ApiResponse::success();
    }

    json handleListNamespaces(const std::string& appId) {
        // Get all namespaces across all envs/clusters for this app
        auto apps = context_.namespaceManager->listApplications();
        json arr = json::array();
        for (auto& ns : context_.store->listNamespaces(appId, "", "")) {
            json obj;
            obj["id"] = ns.id;
            obj["app_id"] = ns.app_id;
            obj["env"] = ns.env;
            obj["cluster"] = ns.cluster;
            obj["name"] = ns.name;
            obj["type"] = ns.type;
            arr.push_back(obj);
        }
        return json{{"namespaces", arr}};
    }

    json handleCreateNamespace() {
        auto body = json::parse(request_.body());
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

    json handleGetConfig(const std::string& ns, const std::string& key) {
        auto config = context_.store->getConfig(ns, key, false);
        if (!config) return ApiResponse::error(404, "Config not found");
        return json{{"key", config->key}, {"value", config->value}, {"version", config->version}};
    }

    json handleSetConfig(const std::string& ns, const std::string& key) {
        auto body = json::parse(request_.body());
        std::string value = body["value"];

        auto existing = context_.store->getConfig(ns, key, false);
        if (existing) {
            context_.store->updateConfig(ns, key, value, "system", true);
        } else {
            context_.store->createConfig(ns, key, value, "system");
        }
        return ApiResponse::success();
    }

    json handleDeleteConfig(const std::string& ns, const std::string& key) {
        context_.store->deleteConfig(ns, key, "system", true);
        return ApiResponse::success();
    }

    json handlePublish(const std::string& ns) {
        auto body = json::parse(request_.body());
        std::string releaseNote = body.value("release_note", "");

        Release release;
        bool ok = context_.store->publishRelease(ns, releaseNote, "system", release);
        if (!ok) return ApiResponse::error(500, "Publish failed");

        return json{
            {"release_id", release.id},
            {"release_version", release.release_version},
            {"release_note", release.release_note},
            {"created_at", release.created_at}};
    }

    json handleGetReleases(const std::string& ns) {
        ReleaseQuery query;
        query.namespace_id = ns;
        auto releases = context_.store->getReleases(query);

        json arr = json::array();
        for (auto& r : releases) {
            json obj;
            obj["id"] = r.id;
            obj["release_version"] = r.release_version;
            obj["release_note"] = r.release_note;
            obj["release_type"] = r.release_type;
            obj["status"] = r.status;
            obj["operator_id"] = r.operator_id;
            obj["created_at"] = r.created_at;
            arr.push_back(obj);
        }
        return json{{"releases", arr}};
    }

    json handleNotifications() {
        return ApiResponse::error(501, "Long-polling not implemented yet");
    }

    tcp::socket socket_;
    ApiContext context_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
};

HttpServer::HttpServer(boost::asio::io_context& ioc, const std::string& host, uint16_t port, ApiContext context)
    : ioc_(ioc), host_(host), port_(port), context_(std::move(context)) {
    acceptor_ = new tcp::acceptor(ioc_, tcp::endpoint(net::ip::make_address(host), port));
}

HttpServer::~HttpServer() {
    delete acceptor_;
}

void HttpServer::start() {
    doAccept();
}

void HttpServer::stop() {
    acceptor_->close();
}

void HttpServer::doAccept() {
    acceptor_->async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<HttpSession>(std::move(socket), context_)->run();
        }
        doAccept();
    });
}

} // namespace config_center

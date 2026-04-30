#include "config-center/config/config_parser.h"
#include "config-center/server/http_server.h"
#include "config-center/server/grpc_server.h"
#include "config-center/storage/sqlite_config_store.h"
#include "config-center/storage/namespace_manager.h"
#include "config-center/publish/publish_engine.h"
#include "config-center/subscription/notification_channel.h"
#include "config-center/auth/auth_types.h"
#include "config-center/api/api_types.h"

#include <boost/asio/io_context.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    std::string configPath = "config-server.yaml";
    if (argc > 1) {
        configPath = argv[1];
    }

    std::cout << "Config Center Server starting..." << std::endl;

    // Parse configuration
    auto appConfig = config_center::ConfigParser::parseFile(configPath);

    // Initialize storage
    std::shared_ptr<config_center::ConfigStore> store;
    if (appConfig.storage.backend == "sqlite") {
        auto sqliteStore = std::make_shared<config_center::SqliteConfigStore>();
        sqliteStore->initialize(appConfig.storage.sqlite.path);
        store = sqliteStore;
    } else {
        std::cerr << "MySQL backend not yet supported" << std::endl;
        return 1;
    }

    // Initialize managers
    auto namespaceManager = std::make_shared<config_center::NamespaceManager>(store);
    auto publishEngine = std::make_shared<config_center::PublishEngine>(store);
    auto notificationChannel = std::make_shared<config_center::NotificationChannel>();
    auto jwtManager = std::make_shared<config_center::JwtManager>(appConfig.auth.jwt.secret, appConfig.auth.jwt.expiry_hours);
    auto authMiddleware = std::make_shared<config_center::AuthMiddleware>(jwtManager);

    // API context
    config_center::ApiContext apiContext{store, namespaceManager, publishEngine, notificationChannel, authMiddleware, jwtManager};

    // Start HTTP server
    boost::asio::io_context ioc;
    auto httpServer = std::make_shared<config_center::HttpServer>(
        ioc, appConfig.server.http.host, appConfig.server.http.port, apiContext);
    httpServer->start();

    // Determine thread count
    int numThreads = appConfig.server.http.threads > 0 ?
        appConfig.server.http.threads :
        static_cast<int>(std::thread::hardware_concurrency());

    std::cout << "HTTP server listening on " << appConfig.server.http.host << ":"
              << appConfig.server.http.port << std::endl;

    // Run IO context on thread pool
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&ioc] {
            ioc.run();
        });
    }

    // Start gRPC server
    auto grpcServer = std::make_shared<config_center::GrpcServer>(
        appConfig.server.grpc.host, appConfig.server.grpc.port, apiContext);
    grpcServer->start();

    std::cout << "gRPC server listening on " << appConfig.server.grpc.host << ":"
              << appConfig.server.grpc.port << std::endl;
    std::cout << "Config Center Server is ready." << std::endl;

    // Wait for threads
    for (auto& t : threads) {
        t.join();
    }

    grpcServer->stop();
    std::cout << "Config Center Server stopped." << std::endl;

    return 0;
}

#pragma once

#include "config-center/api/api_types.h"

#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace config_center {

class GrpcServer {
public:
    GrpcServer(const std::string& host, uint16_t port, ApiContext context);
    ~GrpcServer();

    void start();
    void stop();

private:
    std::string host_;
    uint16_t port_;
    ApiContext context_;
    std::unique_ptr<grpc::Server> server_;
};

class ConfigGrpcService final : public class ConfigService::Service {
public:
    explicit ConfigGrpcService(ApiContext context) : context_(std::move(context)) {}

    grpc::Status GetConfig(grpc::ServerContext* ctx, const GetConfigRequest* req,
                           GetConfigResponse* resp) override;
    grpc::Status SetConfig(grpc::ServerContext* ctx, const SetConfigRequest* req,
                           SetConfigResponse* resp) override;
    grpc::Status DeleteConfig(grpc::ServerContext* ctx, const DeleteConfigRequest* req,
                              DeleteConfigResponse* resp) override;
    grpc::Status WatchConfig(grpc::ServerContext* ctx, const WatchConfigRequest* req,
                             grpc::ServerWriter<ConfigChangeNotification>* writer) override;
    grpc::Status PublishRelease(grpc::ServerContext* ctx, const PublishReleaseRequest* req,
                                PublishReleaseResponse* resp) override;
    grpc::Status GetReleaseHistory(grpc::ServerContext* ctx, const GetReleaseHistoryRequest* req,
                                   GetReleaseHistoryResponse* resp) override;

private:
    ApiContext context_;
};

} // namespace config_center

#include "config-center/api/api_types.h"
#include "config_service.grpc.pb.h"
#include "config_service.pb.h"

namespace config_center {

class GrpcServiceImpl final : public ConfigService::Service {
public:
    explicit GrpcServiceImpl(ApiContext context) : context_(std::move(context)) {}

    grpc::Status GetConfig(grpc::ServerContext* ctx, const GetConfigRequest* req, GetConfigResponse* resp) override;
    grpc::Status SetConfig(grpc::ServerContext* ctx, const SetConfigRequest* req, SetConfigResponse* resp) override;
    grpc::Status DeleteConfig(grpc::ServerContext* ctx, const DeleteConfigRequest* req, DeleteConfigResponse* resp) override;
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

#include "config-center/server/grpc_server.h"

#include "config_service.grpc.pb.h"
#include "config_service.pb.h"

#include <grpcpp/server_builder.h>

namespace config_center {

GrpcServer::GrpcServer(const std::string& host, uint16_t port, ApiContext context)
    : host_(host), port_(port), context_(std::move(context)) {
}

GrpcServer::~GrpcServer() {
    stop();
}

void GrpcServer::start() {
    std::string serverAddress = host_ + ":" + std::to_string(port_);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());

    auto service = std::make_unique<ConfigGrpcService>(context_);
    builder.RegisterService(service.get());

    server_ = builder.BuildAndStart();
}

void GrpcServer::stop() {
    if (server_) {
        server_->Shutdown();
    }
}

// Unary RPC Handlers
grpc::Status ConfigGrpcService::GetConfig(grpc::ServerContext* ctx, const GetConfigRequest* req,
                                           GetConfigResponse* resp) {
    auto config = context_.store->getConfig(req->namespace_id(), req->key(), false);
    if (!config) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Config not found");
    }

    auto* c = resp->mutable_config();
    c->set_namespace_id(config->namespace_id);
    c->set_key(config->key);
    c->set_value(config->value);
    c->set_version(config->version);
    c->set_status(config->status);
    return grpc::Status::OK;
}

grpc::Status ConfigGrpcService::SetConfig(grpc::ServerContext* ctx, const SetConfigRequest* req,
                                           SetConfigResponse* resp) {
    std::string opId = req->operator_id().empty() ? "grpc" : req->operator_id();

    auto existing = context_.store->getConfig(req->namespace_id(), req->key(), false);
    if (existing) {
        context_.store->updateConfig(req->namespace_id(), req->key(), req->value(), opId, true);
    } else {
        context_.store->createConfig(req->namespace_id(), req->key(), req->value(), opId);
    }

    auto updated = context_.store->getConfig(req->namespace_id(), req->key(), false);
    if (updated) {
        auto* c = resp->mutable_config();
        c->set_namespace_id(updated->namespace_id);
        c->set_key(updated->key);
        c->set_value(updated->value);
        c->set_version(updated->version);
        c->set_status(updated->status);
    }
    return grpc::Status::OK;
}

grpc::Status ConfigGrpcService::DeleteConfig(grpc::ServerContext* ctx, const DeleteConfigRequest* req,
                                              DeleteConfigResponse* resp) {
    std::string opId = req->operator_id().empty() ? "grpc" : req->operator_id();
    bool ok = context_.store->deleteConfig(req->namespace_id(), req->key(), opId, true);
    resp->set_success(ok);
    return grpc::Status::OK;
}

grpc::Status ConfigGrpcService::WatchConfig(grpc::ServerContext* ctx, const WatchConfigRequest* req,
                                             grpc::ServerWriter<ConfigChangeNotification>* writer) {
    // Keep the stream open, pushing notifications when config changes
    while (!ctx->IsCancelled()) {
        ConfigChangeNotification notification;
        notification.set_namespace_id("");
        notification.set_release_id(0);
        notification.set_timestamp(0);

        if (writer->Write(notification)) {
            // Keep alive
            std::this_thread::sleep_for(std::chrono::seconds(30));
        } else {
            break;
        }
    }
    return grpc::Status::OK;
}

grpc::Status ConfigGrpcService::PublishRelease(grpc::ServerContext* ctx, const PublishReleaseRequest* req,
                                                PublishReleaseResponse* resp) {
    std::string opId = req->operator_id().empty() ? "grpc" : req->operator_id();

    Release release;
    bool ok = context_.store->publishRelease(req->namespace_id(), req->release_note(), opId, release);
    if (!ok) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Publish failed");
    }

    auto* r = resp->mutable_release();
    r->set_id(release.id);
    r->set_namespace_id(release.namespace_id);
    r->set_release_version(release.release_version);
    r->set_release_note(release.release_note);
    r->set_release_type(release.release_type);
    r->set_status(release.status);
    r->set_operator_id(release.operator_id);
    r->set_created_at(release.created_at);

    return grpc::Status::OK;
}

grpc::Status ConfigGrpcService::GetReleaseHistory(grpc::ServerContext* ctx, const GetReleaseHistoryRequest* req,
                                                   GetReleaseHistoryResponse* resp) {
    ReleaseQuery query;
    query.namespace_id = req->namespace_id();
    query.page = req->page() > 0 ? req->page() : 1;
    query.page_size = req->page_size() > 0 ? req->page_size() : 20;

    auto releases = context_.store->getReleases(query);
    for (auto& rel : releases) {
        auto* r = resp->add_releases();
        r->set_id(rel.id);
        r->set_namespace_id(rel.namespace_id);
        r->set_release_version(rel.release_version);
        r->set_release_note(rel.release_note);
        r->set_release_type(rel.release_type);
        r->set_status(rel.status);
        r->set_operator_id(rel.operator_id);
        r->set_created_at(rel.created_at);
    }
    resp->set_total(static_cast<int32_t>(releases.size()));

    return grpc::Status::OK;
}

} // namespace config_center

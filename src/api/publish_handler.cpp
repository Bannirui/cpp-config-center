#include "config-center/api/publish_handler.h"

namespace config_center {

json PublishHandler::handlePublish(const std::string& namespaceId, const json& body, const std::string& operatorId) {
    std::string releaseNote = body.value("release_note", "");
    Release release;
    if (!context_.store->publishRelease(namespaceId, releaseNote, operatorId, release)) {
        return ApiResponse::error(500, "Publish failed");
    }
    return json{{"release_id", release.id}, {"release_version", release.release_version},
                {"release_note", release.release_note}, {"created_at", release.created_at}};
}

json PublishHandler::handleGetReleases(const std::string& namespaceId) {
    ReleaseQuery query;
    query.namespace_id = namespaceId;
    auto releases = context_.store->getReleases(query);
    json arr = json::array();
    for (auto& r : releases) {
        arr.push_back({{"id", r.id}, {"release_version", r.release_version}, {"release_note", r.release_note},
                       {"release_type", r.release_type}, {"status", r.status}, {"operator_id", r.operator_id},
                       {"created_at", r.created_at}});
    }
    return json{{"releases", arr}};
}

} // namespace config_center

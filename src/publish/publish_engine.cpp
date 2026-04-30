#include "config-center/publish/publish_engine.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace config_center {

PublishEngine::PublishEngine(std::shared_ptr<ConfigStore> store)
    : store_(std::move(store)) {
}

std::vector<ConfigKey> PublishEngine::getDraftChanges(const std::string& namespaceId) {
    auto configs = store_->listConfigs(namespaceId, true);
    std::vector<ConfigKey> drafts;
    std::copy_if(configs.begin(), configs.end(), std::back_inserter(drafts),
                  [](const ConfigKey& c) { return c.status == "draft"; });
    return drafts;
}

bool PublishEngine::addDraftChange(const std::string& namespaceId, const std::string& key, const std::string& value,
                                    const std::string& operatorId) {
    auto existing = store_->getConfig(namespaceId, key, false);
    if (existing) {
        return store_->updateConfig(namespaceId, key, value, operatorId, true);
    }
    return store_->createConfig(namespaceId, key, value, operatorId);
}

bool PublishEngine::removeDraftChange(const std::string& namespaceId, const std::string& key,
                                       const std::string& operatorId) {
    return store_->deleteConfig(namespaceId, key, operatorId, true);
}

bool PublishEngine::publish(const std::string& namespaceId, const std::string& releaseNote,
                             const std::string& operatorId, Release& outRelease) {
    return store_->publishRelease(namespaceId, releaseNote, operatorId, outRelease);
}

bool PublishEngine::submitForApproval(const std::string& namespaceId, const std::string& submitterId,
                                       Approval& outApproval) {
    Approval approval;
    approval.namespace_id = namespaceId;
    approval.submitter_id = submitterId;
    approval.status = "pending";
    approval.created_at = "";
    store_->createApproval(approval);
    outApproval = approval;
    return true;
}

bool PublishEngine::approve(int64_t approvalId, const std::string& approverId, const std::string& comment) {
    return store_->updateApprovalStatus(approvalId, "approved", comment);
}

bool PublishEngine::reject(int64_t approvalId, const std::string& approverId, const std::string& comment) {
    return store_->updateApprovalStatus(approvalId, "rejected", comment);
}

std::vector<Approval> PublishEngine::getPendingApprovals(const std::string& namespaceId) {
    return store_->getPendingApprovals(namespaceId);
}

bool PublishEngine::publishGrayscale(const std::string& namespaceId, const std::string& releaseNote,
                                      const std::string& operatorId, int percentage,
                                      const std::vector<std::string>& whitelist, Release& outRelease) {
    store_->beginTransaction();
    try {
        store_->publishRelease(namespaceId, releaseNote, operatorId, outRelease);
        outRelease.release_type = "grayscale";
        outRelease.grayscale_percentage = percentage;

        if (!whitelist.empty()) {
            nlohmann::json arr = whitelist;
            outRelease.whitelist = arr.dump();
        }

        store_->commitTransaction();
        return true;
    } catch (...) {
        store_->rollbackTransaction();
        return false;
    }
}

bool PublishEngine::fullRollout(const std::string& namespaceId, const std::string& operatorId, int64_t releaseId) {
    auto release = store_->getRelease(releaseId);
    if (!release || release->status != "active") return false;

    store_->retireRelease(releaseId);

    Release newRelease;
    newRelease.namespace_id = namespaceId;
    newRelease.release_note = "Full rollout of release " + std::to_string(releaseId);
    newRelease.release_type = "full";
    newRelease.status = "active";
    newRelease.operator_id = operatorId;
    return store_->createRelease(newRelease);
}

std::vector<Release> PublishEngine::getReleaseHistory(const std::string& namespaceId, int page, int pageSize) {
    ReleaseQuery query;
    query.namespace_id = namespaceId;
    query.page = page;
    query.page_size = pageSize;
    return store_->getReleases(query);
}

} // namespace config_center

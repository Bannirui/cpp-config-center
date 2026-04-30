#pragma once

#include "config-center/storage/config_store.h"
#include "config-center/storage/data_types.h"

#include <memory>
#include <string>
#include <vector>

namespace config_center {

class PublishEngine {
public:
    explicit PublishEngine(std::shared_ptr<ConfigStore> store);

    // Draft management
    std::vector<ConfigKey> getDraftChanges(const std::string& namespaceId);
    bool addDraftChange(const std::string& namespaceId, const std::string& key, const std::string& value,
                         const std::string& operatorId);
    bool removeDraftChange(const std::string& namespaceId, const std::string& key, const std::string& operatorId);

    // Publish
    bool publish(const std::string& namespaceId, const std::string& releaseNote, const std::string& operatorId,
                  Release& outRelease);

    // Approval
    bool submitForApproval(const std::string& namespaceId, const std::string& submitterId, Approval& outApproval);
    bool approve(int64_t approvalId, const std::string& approverId, const std::string& comment);
    bool reject(int64_t approvalId, const std::string& approverId, const std::string& comment);
    std::vector<Approval> getPendingApprovals(const std::string& namespaceId);

    // Grayscale release
    bool publishGrayscale(const std::string& namespaceId, const std::string& releaseNote,
                           const std::string& operatorId, int percentage,
                           const std::vector<std::string>& whitelist, Release& outRelease);
    bool fullRollout(const std::string& namespaceId, const std::string& operatorId, int64_t releaseId);

    // Release history
    std::vector<Release> getReleaseHistory(const std::string& namespaceId, int page, int pageSize);

private:
    std::shared_ptr<ConfigStore> store_;
};

} // namespace config_center

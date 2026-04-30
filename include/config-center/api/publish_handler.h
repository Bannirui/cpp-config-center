#include "config-center/api/api_types.h"
#include <string>

namespace config_center {

class PublishHandler {
public:
    explicit PublishHandler(ApiContext context) : context_(std::move(context)) {}
    json handlePublish(const std::string& namespaceId, const json& body, const std::string& operatorId);
    json handleGetReleases(const std::string& namespaceId);
private:
    ApiContext context_;
};

} // namespace config_center

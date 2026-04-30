#include "config-center/api/api_types.h"

#include <optional>
#include <string>

namespace config_center {

class ConfigHandler {
public:
    explicit ConfigHandler(ApiContext context) : context_(std::move(context)) {}

    json handleGetConfig(const std::string& namespaceId, const std::string& key);
    json handleSetConfig(const std::string& namespaceId, const std::string& key, const json& body, const std::string& operatorId);
    json handleDeleteConfig(const std::string& namespaceId, const std::string& key, const std::string& operatorId);

private:
    ApiContext context_;
};

} // namespace config_center

#include "config-center/api/api_types.h"

#include <string>

namespace config_center {

class NamespaceHandler {
public:
    explicit NamespaceHandler(ApiContext context) : context_(std::move(context)) {}

    json handleCreateApp(const json& body);
    json handleListApps();
    json handleGetApp(const std::string& appId);
    json handleDeleteApp(const std::string& appId);
    json handleCreateNamespace(const json& body);
    json handleListNamespaces(const std::string& appId);

private:
    ApiContext context_;
};

} // namespace config_center

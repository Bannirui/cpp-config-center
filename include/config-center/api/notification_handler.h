#include "config-center/api/api_types.h"
#include <string>

namespace config_center {

class NotificationHandler {
public:
    explicit NotificationHandler(ApiContext context) : context_(std::move(context)) {}
    json handleLongPoll(const std::string& namespaceId, const std::string& lastReleaseId, int timeout);
private:
    ApiContext context_;
};

} // namespace config_center

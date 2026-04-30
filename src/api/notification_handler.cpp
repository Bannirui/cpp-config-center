#include "config-center/api/notification_handler.h"

namespace config_center {

json NotificationHandler::handleLongPoll(const std::string& namespaceId, const std::string& lastReleaseId, int timeout) {
    // Placeholder: actual long-polling implementation in subscription module
    return json{{"namespace", namespaceId}, {"changed", false}};
}

} // namespace config_center

#include "config-center/api/health_handler.h"

namespace config_center {

json HealthHandler::handleHealth() {
    return json{{"status", "UP"}, {"uptime_seconds", 0}, {"db_connected", context_.store->healthCheck()}};
}

} // namespace config_center

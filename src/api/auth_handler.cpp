#include "config-center/api/auth_handler.h"

namespace config_center {

json AuthHandler::handleLogin(const std::string& username, const std::string& password) {
    // Will be connected to JWT manager and user store
    return ApiResponse::error(501, "Auth not yet configured");
}

} // namespace config_center

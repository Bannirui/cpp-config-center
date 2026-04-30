#include "config-center/api/api_types.h"

namespace config_center {

json ApiResponse::error(int code, const std::string& message) {
    return json{{"error", message}, {"code", code}};
}

json ApiResponse::success(const json& data) {
    return data;
}

json ApiResponse::success() {
    return json{{"status", "ok"}};
}

} // namespace config_center

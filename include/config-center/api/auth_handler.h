#include "config-center/api/api_types.h"
#include <string>

namespace config_center {

class AuthHandler {
public:
    explicit AuthHandler(ApiContext context) : context_(std::move(context)) {}
    json handleLogin(const std::string& username, const std::string& password);
private:
    ApiContext context_;
};

} // namespace config_center

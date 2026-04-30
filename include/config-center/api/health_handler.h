#include "config-center/api/api_types.h"
#include <string>

namespace config_center {

class HealthHandler {
public:
    explicit HealthHandler(ApiContext context) : context_(std::move(context)) {}
    json handleHealth();
private:
    ApiContext context_;
};

} // namespace config_center

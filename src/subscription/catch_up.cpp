#include "config-center/subscription/notification_channel.h"
// Catch-up mechanism
namespace config_center {
class CatchUpHandler {
    std::shared_ptr<NotificationChannel> channel_;
public:
    explicit CatchUpHandler(std::shared_ptr<NotificationChannel> ch) : channel_(std::move(ch)) {}
};
} // namespace config_center

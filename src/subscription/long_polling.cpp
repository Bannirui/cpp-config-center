#include "config-center/subscription/notification_channel.h"
// Long-polling handler
namespace config_center {
class LongPollingHandler {
    std::shared_ptr<NotificationChannel> channel_;
public:
    explicit LongPollingHandler(std::shared_ptr<NotificationChannel> ch) : channel_(std::move(ch)) {}
};
} // namespace config_center

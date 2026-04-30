#include "config-center/subscription/notification_channel.h"
// gRPC WatchConfig streaming handler
namespace config_center {
class WatchStreamHandler {
    std::shared_ptr<NotificationChannel> channel_;
public:
    explicit WatchStreamHandler(std::shared_ptr<NotificationChannel> ch) : channel_(std::move(ch)) {}
};
} // namespace config_center

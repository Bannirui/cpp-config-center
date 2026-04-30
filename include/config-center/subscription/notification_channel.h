#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace config_center {

struct Notification {
    std::string namespaceId;
    int64_t releaseId = 0;
    std::vector<std::string> changedKeys;
    int64_t timestamp = 0;
};

using NotificationCallback = std::function<void(const Notification&)>;

class NotificationChannel {
public:
    NotificationChannel();

    void subscribe(const std::string& namespaceId, NotificationCallback callback);
    void unsubscribe(const std::string& namespaceId);
    void publish(const Notification& notification);

    // Long-polling support
    bool waitForNotification(const std::string& namespaceId, Notification& outNotification, int timeoutSeconds);
    std::vector<Notification> getMissedNotifications(const std::string& namespaceId, int64_t lastReleaseId);

private:
    std::mutex mutex_;
    std::map<std::string, std::vector<NotificationCallback>> subscribers_;
    std::map<std::string, std::vector<Notification>> history_;
    std::condition_variable cv_;
    std::vector<Notification> pending_;
};

} // namespace config_center

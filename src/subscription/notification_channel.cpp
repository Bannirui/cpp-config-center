#include "config-center/subscription/notification_channel.h"

#include <chrono>
#include <ctime>

namespace config_center {

NotificationChannel::NotificationChannel() {
}

void NotificationChannel::subscribe(const std::string& namespaceId, NotificationCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[namespaceId].push_back(std::move(callback));
}

void NotificationChannel::unsubscribe(const std::string& namespaceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.erase(namespaceId);
}

void NotificationChannel::publish(const Notification& notification) {
    std::lock_guard<std::mutex> lock(mutex_);

    history_[notification.namespaceId].push_back(notification);

    auto it = subscribers_.find(notification.namespaceId);
    if (it != subscribers_.end()) {
        for (auto& cb : it->second) {
            cb(notification);
        }
    }

    pending_.push_back(notification);
    cv_.notify_all();
}

bool NotificationChannel::waitForNotification(const std::string& namespaceId, Notification& outNotification,
                                               int timeoutSeconds) {
    std::unique_lock<std::mutex> lock(mutex_);

    // Check if there's already a pending notification for this namespace
    for (auto it = pending_.begin(); it != pending_.end(); ++it) {
        if (it->namespaceId == namespaceId) {
            outNotification = *it;
            pending_.erase(it);
            return true;
        }
    }

    bool notified = cv_.wait_for(lock, std::chrono::seconds(timeoutSeconds), [this, &namespaceId, &outNotification] {
        for (auto it = pending_.begin(); it != pending_.end(); ++it) {
            if (it->namespaceId == namespaceId) {
                outNotification = *it;
                pending_.erase(it);
                return true;
            }
        }
        return false;
    });

    return notified;
}

std::vector<Notification> NotificationChannel::getMissedNotifications(const std::string& namespaceId,
                                                                        int64_t lastReleaseId) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Notification> missed;
    auto it = history_.find(namespaceId);
    if (it != history_.end()) {
        for (auto& n : it->second) {
            if (n.releaseId > lastReleaseId) {
                missed.push_back(n);
            }
        }
    }
    return missed;
}

} // namespace config_center

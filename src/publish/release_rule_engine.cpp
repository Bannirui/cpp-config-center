#include "config-center/publish/publish_engine.h"

// Grayscale release rule engine
namespace config_center {

class ReleaseRuleEngine {
public:
    bool evaluateClientMatch(const std::string& instanceId, const std::string& ipAddress,
                             int grayscalePercentage, const std::vector<std::string>& whitelist) {
        // Check whitelist first
        for (auto& w : whitelist) {
            if (w == instanceId || w == ipAddress) return true;
        }

        if (grayscalePercentage >= 100) return true;
        if (grayscalePercentage <= 0) return false;

        // Consistent hashing based on instance ID
        size_t hash = std::hash<std::string>{}(instanceId + ipAddress);
        int bucket = static_cast<int>(hash % 100);
        return bucket < grayscalePercentage;
    }
};

} // namespace config_center

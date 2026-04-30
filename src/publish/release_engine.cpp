#include "config-center/publish/publish_engine.h"
// ReleaseEngine wraps release operations
namespace config_center {
class ReleaseEngine {
    std::shared_ptr<PublishEngine> engine_;
public:
    explicit ReleaseEngine(std::shared_ptr<PublishEngine> e) : engine_(std::move(e)) {}
};
} // namespace config_center

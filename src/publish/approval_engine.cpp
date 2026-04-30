#include "config-center/publish/publish_engine.h"
// ApprovalEngine wraps approval operations
namespace config_center {
class ApprovalEngine {
    std::shared_ptr<PublishEngine> engine_;
public:
    explicit ApprovalEngine(std::shared_ptr<PublishEngine> e) : engine_(std::move(e)) {}
};
} // namespace config_center

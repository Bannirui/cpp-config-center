#include "config-center/publish/publish_engine.h"
// DraftManager is a thin wrapper around PublishEngine draft operations
namespace config_center {
class DraftManager {
    std::shared_ptr<PublishEngine> engine_;
public:
    explicit DraftManager(std::shared_ptr<PublishEngine> e) : engine_(std::move(e)) {}
};
} // namespace config_center

#include "capture_freeze_scope.h"

#include "app_config_store.h"

namespace markshot {

CaptureFreezeScope configuredCaptureFreezeScope()
{
    bool ok = false;
    const QJsonObject root = readAppConfigRoot(&ok);
    if (!ok) {
        return defaultCaptureFreezeScope();
    }
    return captureFreezeScopeFromConfigRoot(root);
}

}  // namespace markshot

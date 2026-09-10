#include "clipboard_publish_policy.h"

namespace markshot {

ClipboardPublishPath chooseClipboardPublishPath(bool persistentOwnerAvailable,
                                                bool qtClipboardAvailable)
{
    // 1. Wayland 上必须由独立进程持有选择，否则本进程退出后内容会丢失
    if (persistentOwnerAvailable) {
        return ClipboardPublishPath::PersistentOwner;
    }
    // 2. 没有 wl-copy / xclip 时退回 Qt 剪贴板
    if (qtClipboardAvailable) {
        return ClipboardPublishPath::QtClipboard;
    }
    return ClipboardPublishPath::None;
}

}  // namespace markshot

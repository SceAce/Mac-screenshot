#pragma once

namespace markshot {

enum class ClipboardPublishPath {
    PersistentOwner,
    QtClipboard,
    None,
};

/**
 * 选择剪贴板发布路径，避免 Qt 与 wl-copy 争抢同一选择。
 * @param persistentOwnerAvailable 是否能启动持久剪贴板进程。
 * @param qtClipboardAvailable 是否存在 QClipboard。
 * @return 应使用的发布路径。
 */
ClipboardPublishPath chooseClipboardPublishPath(bool persistentOwnerAvailable,
                                                bool qtClipboardAvailable);

}  // namespace markshot

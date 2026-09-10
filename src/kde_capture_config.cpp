#include "kde_capture_config.h"

#include "app_config_store.h"
#include "config_value.h"
#include "window_detection.h"

#include <QFileInfo>
#include <QJsonValue>
#include <QMutex>
#include <QMutexLocker>

#include <optional>

namespace {

/// @brief 返回 capture.wayland.kde.kwinScreenshot.enabled 配置字段。
/// @param root 应用配置根对象。
/// @return 配置字段值，缺失时返回 undefined。
QJsonValue kwinScreenshotEnabledValue(const QJsonObject &root)
{
    const QJsonObject capture = markshot::config::objectValue(root, QStringLiteral("capture"));
    const QJsonObject wayland = markshot::config::objectValue(capture, QStringLiteral("wayland"));
    const QJsonObject kde = markshot::config::objectValue(wayland, QStringLiteral("kde"));
    const QJsonObject kwinScreenshot =
        markshot::config::objectValue(kde, QStringLiteral("kwinScreenshot"));
    return kwinScreenshot.value(QStringLiteral("enabled"));
}

/**
 * 按配置文件 mtime 缓存 KWin ScreenShot2 开关。
 *
 * 滚动捕获每个 tick 都会读取该开关；直接读文件意味着每秒约 22 次磁盘
 * I/O 与 JSON 解析。mtime 未变化时直接返回缓存值。
 *
 * @return 配置生效值。
 */
bool cachedKwinScreenshotEnabled()
{
    static QMutex mutex;
    static qint64 cachedMtime = -1;
    static bool cachedEnabled = markshot::defaultKdeKWinScreenshotEnabled();

    const QString path = markshot::appConfigPath();
    const qint64 mtime = QFileInfo(path).lastModified().toMSecsSinceEpoch();

    QMutexLocker locker(&mutex);
    if (mtime != cachedMtime) {
        bool ok = false;
        const QJsonObject root = markshot::readAppConfigRoot(&ok);
        cachedEnabled = ok ? markshot::kdeKWinScreenshotEnabledFromConfigRoot(root)
                           : markshot::defaultKdeKWinScreenshotEnabled();
        cachedMtime = mtime;
    }
    return cachedEnabled;
}

}  // namespace

namespace markshot {

bool defaultKdeKWinScreenshotEnabled()
{
    return true;
}

bool kdeKWinScreenshotEnabledFromConfigRoot(const QJsonObject &root)
{
    const std::optional<bool> value = config::boolValue(kwinScreenshotEnabledValue(root));
    return value.value_or(defaultKdeKWinScreenshotEnabled());
}

bool configuredKdeKWinScreenshotEnabled()
{
    return cachedKwinScreenshotEnabled();
}

}  // namespace markshot

#include "capture_freeze_scope.h"

#include "config_value.h"

#include <QJsonValue>

#include <utility>

namespace {

/// @brief 归一化用户配置字符串。
/// @param value 原始配置字符串。
/// @return 去除分隔符并转小写后的字符串。
QString normalizedScopeText(QString value)
{
    return markshot::config::normalizedKey(std::move(value));
}

/// @brief 返回配置对象中的冻结范围字段。
/// @param root 应用配置根对象。
/// @return 配置字段值，缺失时返回 undefined。
QJsonValue freezeScopeValue(const QJsonObject &root)
{
    const QJsonObject capture =
        markshot::config::firstNonEmptyObjectValue(root,
                                                   {QStringLiteral("capture"),
                                                    QStringLiteral("screenshot"),
                                                    QStringLiteral("screenCapture")});
    const QJsonValue nestedValue =
        markshot::config::valueForKeys(capture,
                                       {QStringLiteral("freezeScope"),
                                        QStringLiteral("freezeDisplayScope"),
                                        QStringLiteral("freezeDisplays"),
                                        QStringLiteral("displayFreezeScope"),
                                        QStringLiteral("screenFreezeScope")});
    if (!nestedValue.isUndefined()) {
        return nestedValue;
    }

    return markshot::config::valueForKeys(root,
                                          {QStringLiteral("captureFreezeScope"),
                                           QStringLiteral("freezeScope"),
                                           QStringLiteral("freezeDisplayScope")});
}

}  // namespace

namespace markshot {

CaptureFreezeScope defaultCaptureFreezeScope()
{
    return CaptureFreezeScope::AllScreens;
}

std::optional<CaptureFreezeScope> captureFreezeScopeFromText(QString value)
{
    const QString text = normalizedScopeText(std::move(value));
    if (text.isEmpty()) {
        return std::nullopt;
    }

    if (text == QStringLiteral("cursorscreen")
        || text == QStringLiteral("mousescreen")
        || text == QStringLiteral("currentscreen")
        || text == QStringLiteral("focusedscreen")
        || text == QStringLiteral("screen")
        || text == QStringLiteral("single")
        || text == QStringLiteral("singlescreen")) {
        return CaptureFreezeScope::CursorScreen;
    }

    if (text == QStringLiteral("allscreens")
        || text == QStringLiteral("alldisplays")
        || text == QStringLiteral("allmonitors")
        || text == QStringLiteral("alloutputs")
        || text == QStringLiteral("virtualdesktop")
        || text == QStringLiteral("all")) {
        return CaptureFreezeScope::AllScreens;
    }

    return std::nullopt;
}

CaptureFreezeScope captureFreezeScopeFromConfigRoot(const QJsonObject &root)
{
    const QJsonValue value = freezeScopeValue(root);
    if (value.isString()) {
        if (const std::optional<CaptureFreezeScope> scope = captureFreezeScopeFromText(value.toString())) {
            return *scope;
        }
    }
    return defaultCaptureFreezeScope();
}

QString captureFreezeScopeName(CaptureFreezeScope scope)
{
    switch (scope) {
    case CaptureFreezeScope::CursorScreen:
        return QStringLiteral("cursor-screen");
    case CaptureFreezeScope::AllScreens:
        return QStringLiteral("all-screens");
    }
    return QStringLiteral("all-screens");
}

}  // namespace markshot

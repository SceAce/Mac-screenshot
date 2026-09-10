#include "selection_loupe_config.h"

#include "config_value.h"

#include <QJsonValue>

namespace {

/**
 * 返回配置里选区放大镜开关对应的字段。
 * @param root 应用配置根对象。
 * @return 字段值，缺失时返回 undefined。
 */
QJsonValue selectionLoupeEnabledValue(const QJsonObject &root)
{
    const QJsonObject capture =
        markshot::config::firstNonEmptyObjectValue(root,
                                                   {QStringLiteral("capture"),
                                                    QStringLiteral("screenshot"),
                                                    QStringLiteral("screenCapture")});
    const QJsonObject loupe =
        markshot::config::firstNonEmptyObjectValue(capture,
                                                   {QStringLiteral("selectionLoupe"),
                                                    QStringLiteral("loupe")});
    const QJsonValue nestedEnabled =
        markshot::config::valueForKeys(loupe,
                                       {QStringLiteral("enabled"),
                                        QStringLiteral("enable")});
    if (!nestedEnabled.isUndefined() && !nestedEnabled.isNull()) {
        return nestedEnabled;
    }

    const QJsonValue nestedBool =
        markshot::config::valueForKeys(capture,
                                       {QStringLiteral("selectionLoupe"),
                                        QStringLiteral("selectionLoupeEnabled"),
                                        QStringLiteral("loupeEnabled")});
    if (!nestedBool.isUndefined() && !nestedBool.isNull() && !nestedBool.isObject()) {
        return nestedBool;
    }

    return markshot::config::valueForKeys(root,
                                          {QStringLiteral("selectionLoupeEnabled"),
                                           QStringLiteral("captureSelectionLoupe")});
}

}  // namespace

namespace markshot {

bool defaultSelectionLoupeEnabled()
{
    return false;
}

bool selectionLoupeEnabledFromConfigRoot(const QJsonObject &root)
{
    const std::optional<bool> value = config::boolValue(selectionLoupeEnabledValue(root));
    return value.value_or(defaultSelectionLoupeEnabled());
}

}  // namespace markshot

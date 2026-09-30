#include "selection_frame_config.h"

#include "config_value.h"

#include <QJsonValue>

namespace markshot {

QColor defaultSelectionFrameColor()
{
    return QColor(94, 234, 212);
}

QColor selectionFrameColorFromConfigRoot(const QJsonObject &root)
{
    const QJsonObject capture =
        config::firstNonEmptyObjectValue(root,
                                         {QStringLiteral("capture"),
                                          QStringLiteral("screenshot"),
                                          QStringLiteral("screenCapture")});
    const QJsonValue value =
        config::valueForKeys(capture,
                             {QStringLiteral("selectionColor"),
                              QStringLiteral("selectionFrameColor"),
                              QStringLiteral("frameColor")});
    if (!value.isString()) {
        return defaultSelectionFrameColor();
    }

    QColor color(value.toString().trimmed());
    if (!color.isValid()) {
        return defaultSelectionFrameColor();
    }
    color.setAlpha(255);
    return color;
}

}  // namespace markshot

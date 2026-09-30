#pragma once

#include <QColor>
#include <QJsonObject>

namespace markshot {

/**
 * 返回截图选区边框的默认颜色。
 * @return 默认青绿色。
 */
QColor defaultSelectionFrameColor();

/**
 * 从应用配置根对象读取截图选区边框颜色。
 * @param root 应用配置根对象。
 * @return 有效的、不透明选区颜色；无效或缺失时返回默认值。
 */
QColor selectionFrameColorFromConfigRoot(const QJsonObject &root);

}  // namespace markshot

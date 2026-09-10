#pragma once

#include <QJsonObject>

namespace markshot {

/**
 * 返回选区放大镜与方向键微调的默认开关。
 * @return 默认关闭。
 */
bool defaultSelectionLoupeEnabled();

/**
 * 从应用配置根对象读取选区放大镜开关。
 * @param root 应用配置根对象。
 * @return 开启时返回 true，缺失时返回默认值。
 */
bool selectionLoupeEnabledFromConfigRoot(const QJsonObject &root);

}  // namespace markshot

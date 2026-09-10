#pragma once

#include <QPointF>

namespace markshot::shot {

/**
 * 将线段终点吸附到最近的水平、垂直或 45 度方向。
 * @param start 线段起点。
 * @param end 原始线段终点。
 * @return 吸附后的线段终点。
 */
QPointF constrainedLineEnd(QPointF start, QPointF end);

}  // namespace markshot::shot

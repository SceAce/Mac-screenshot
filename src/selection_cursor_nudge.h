#pragma once

#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <Qt>

namespace markshot::shot {

/**
 * 把方向键转换成图像坐标步进。
 * @param key Qt 方向键。
 * @param shiftPressed 是否按下 Shift，按下时步进放大。
 * @return 图像像素步进，非方向键返回 (0, 0)。
 */
QPoint selectionCursorNudgeDelta(int key, bool shiftPressed);

/**
 * 按图像像素步进移动光标点，并限制在图像范围内。
 * @param imagePoint 当前图像坐标。
 * @param delta 图像像素步进。
 * @param imageBounds 图像有效范围，右下角为最后一像素。
 * @return 夹紧后的新图像坐标。
 */
QPointF nudgeSelectionCursor(QPointF imagePoint, QPoint delta, QRectF imageBounds);

/**
 * 判断按键是否为选区光标微调方向键。
 * @param key Qt 按键。
 * @return 是方向键时返回 true。
 */
bool isSelectionCursorNudgeKey(int key);

/**
 * 判断 QCursor::setPos 是否把系统光标送到了目标点。
 * @param requested 请求的全局坐标。
 * @param actual setPos 之后读到的全局坐标。
 * @return 到达目标时返回 true。
 */
bool cursorReachedWarpTarget(QPoint requested, QPoint actual);

/**
 * 判断硬件光标是否已离开分离时的锚点。
 * @param anchor 分离时光标的窗口坐标。
 * @param current 当前硬件光标的窗口坐标。
 * @return 明显移动时返回 true。
 */
bool hardwarePointerMoved(QPoint anchor, QPoint current);

}  // namespace markshot::shot

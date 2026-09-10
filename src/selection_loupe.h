#pragma once

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>

class QPainter;

namespace markshot::shot {

struct SelectionLoupeLayout {
    QRectF loupe;
    QRect sourceRect;
};

/**
 * 计算选区放大镜的窗口位置和取样矩形。
 * @param widgetPoint 光标在窗口中的位置。
 * @param viewport 覆盖层窗口尺寸。
 * @param loupeSize 放大镜直径。
 * @param imagePoint 光标对应的图像坐标。
 * @param imageSize 冻结帧尺寸。
 * @return 放大镜窗口矩形和源图像取样矩形。
 */
SelectionLoupeLayout selectionLoupeLayout(QPointF widgetPoint,
                                          QSize viewport,
                                          qreal loupeSize,
                                          QPointF imagePoint,
                                          QSize imageSize);

/**
 * 返回放大镜需要重绘的窗口区域。
 * @param layout 放大镜布局。
 * @return 包含边框余量的脏区。
 */
QRect selectionLoupeDirtyRect(const SelectionLoupeLayout &layout);

/**
 * 绘制选区放大镜。
 * @param painter 当前绘制器。
 * @param frame 冻结截图。
 * @param layout 放大镜布局。
 * @param caption 底部说明文字，空字符串时不绘制标签。
 * @return 无返回值。
 */
void drawSelectionLoupe(QPainter &painter,
                        const QImage &frame,
                        const SelectionLoupeLayout &layout,
                        const QString &caption);

/**
 * 在逻辑光标位置绘制与系统十字光标一致的软件指针。
 * @param painter 当前绘制器。
 * @param widgetPoint 逻辑光标的窗口坐标。
 * @return 无返回值。
 */
void drawSelectionPointer(QPainter &painter, QPointF widgetPoint);

/**
 * 返回软件指针需要重绘的窗口区域。
 * @param widgetPoint 逻辑光标的窗口坐标。
 * @return 包含十字线余量的脏区。
 */
QRect selectionPointerDirtyRect(QPointF widgetPoint);

}  // namespace markshot::shot

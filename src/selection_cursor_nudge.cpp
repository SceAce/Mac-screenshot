#include "selection_cursor_nudge.h"

#include <algorithm>

namespace markshot::shot {

namespace {

constexpr int kFineStep = 1;
constexpr int kCoarseStep = 10;

}  // namespace

QPoint selectionCursorNudgeDelta(int key, bool shiftPressed)
{
    const int step = shiftPressed ? kCoarseStep : kFineStep;
    switch (key) {
    case Qt::Key_Left:
        return {-step, 0};
    case Qt::Key_Right:
        return {step, 0};
    case Qt::Key_Up:
        return {0, -step};
    case Qt::Key_Down:
        return {0, step};
    default:
        return {};
    }
}

QPointF nudgeSelectionCursor(QPointF imagePoint, QPoint delta, QRectF imageBounds)
{
    if (imageBounds.isEmpty() || !imageBounds.isValid()) {
        return imagePoint;
    }

    const qreal minX = imageBounds.left();
    const qreal minY = imageBounds.top();
    const qreal maxX = imageBounds.right();
    const qreal maxY = imageBounds.bottom();
    return {
        std::clamp(imagePoint.x() + static_cast<qreal>(delta.x()), minX, maxX),
        std::clamp(imagePoint.y() + static_cast<qreal>(delta.y()), minY, maxY),
    };
}

bool isSelectionCursorNudgeKey(int key)
{
    return key == Qt::Key_Left || key == Qt::Key_Right
        || key == Qt::Key_Up || key == Qt::Key_Down;
}

bool cursorReachedWarpTarget(QPoint requested, QPoint actual)
{
    return (actual - requested).manhattanLength() <= 1;
}

bool hardwarePointerMoved(QPoint anchor, QPoint current)
{
    return (current - anchor).manhattanLength() > 1;
}

}  // namespace markshot::shot

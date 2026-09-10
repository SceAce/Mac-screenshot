#include "shot_window_module.h"

#include "selection_cursor_nudge.h"

#include <utility>

using namespace markshot::shot;

void ShotWindow::scheduleInitialSelectionRepaint(const QRegion &region)
{
    if (region.isEmpty()) {
        return;
    }

    m_pendingInitialSelectionRepaint |= region;
    if (m_initialSelectionRepaintTimer && !m_initialSelectionRepaintTimer->isActive()) {
        m_initialSelectionRepaintTimer->start();
    }
}

void ShotWindow::flushInitialSelectionRepaint()
{
    if (m_initialSelectionRepaintTimer) {
        m_initialSelectionRepaintTimer->stop();
    }

    const QRegion pending = std::exchange(m_pendingInitialSelectionRepaint, QRegion());
    if (!pending.isEmpty()) {
        update(pending);
    }
}

QRegion ShotWindow::initialSelectionDirtyRegion(const QRectF &previousSelection,
                                                 bool previousSelectionUsable) const
{
    const QRectF currentSelection = normalizedSelection();
    const bool currentSelectionUsable = currentSelection.width() >= kMinSelectionSize
        && currentSelection.height() >= kMinSelectionSize;

    // Before a usable rectangle exists the entire overlay uses the lighter
    // startup dim. Crossing that threshold changes the whole backdrop once.
    if (!previousSelectionUsable || !currentSelectionUsable) {
        return QRegion(rect());
    }

    const QRect previousRect = imageRectToWidget(previousSelection).toAlignedRect();
    const QRect currentRect = imageRectToWidget(currentSelection).toAlignedRect();
    QRegion dirty(previousRect);
    dirty ^= QRegion(currentRect);

    // The selection border and its dimension label are drawn outside the
    // changing dim area. Add just that chrome instead of invalidating the
    // complete selection rectangle.
    auto chromeRegion = [](const QRect &selection) {
        constexpr int kBorderPadding = 4;
        constexpr int kLabelWidth = 160;
        constexpr int kLabelHeight = 40;

        const QRect outer = selection.adjusted(-kBorderPadding,
                                               -kBorderPadding,
                                               kBorderPadding,
                                               kBorderPadding);
        const QRect inner = selection.adjusted(kBorderPadding,
                                               kBorderPadding,
                                               -kBorderPadding,
                                               -kBorderPadding);
        QRegion chrome(outer);
        if (inner.isValid()) {
            chrome -= inner;
        }
        chrome |= QRect(selection.left() + 4,
                        selection.top() + 4,
                        kLabelWidth,
                        kLabelHeight);
        return chrome;
    };

    dirty |= chromeRegion(previousRect);
    dirty |= chromeRegion(currentRect);
    return dirty.intersected(rect());
}

QPointF ShotWindow::selectingPointerImagePoint(QPointF widgetPos)
{
    if (m_selectionPointerDetached
        && !hardwarePointerMoved(m_selectionPointerHardwareAnchor, widgetPos.toPoint())) {
        return m_startupHoverImagePoint;
    }
    if (m_selectionPointerDetached) {
        attachSelectionPointer();
    }
    return widgetToImage(widgetPos);
}

void ShotWindow::attachSelectionPointer()
{
    if (!m_selectionPointerDetached) {
        return;
    }
    m_selectionPointerDetached = false;
    updateCursor();
}

void ShotWindow::detachSelectionPointerIfWarpFailed(QPoint widgetPoint)
{
    const QPoint requested = mapToGlobal(widgetPoint);
    QCursor::setPos(requested);
    if (cursorReachedWarpTarget(requested, QCursor::pos())) {
        m_selectionPointerDetached = false;
        updateCursor();
        return;
    }

    m_selectionPointerDetached = true;
    m_selectionPointerHardwareAnchor = mapFromGlobal(QCursor::pos());
    setCursor(Qt::BlankCursor);
}

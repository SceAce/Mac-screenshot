#include "selection_cursor_nudge.h"

#include <QtTest/QtTest>

class SelectionCursorNudgeTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证方向键生成 1 像素步进，Shift 放大到 10 像素。
     * @return 无返回值。
     */
    void arrowKeysUseFineAndCoarseSteps()
    {
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_Left, false), QPoint(-1, 0));
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_Right, false), QPoint(1, 0));
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_Up, false), QPoint(0, -1));
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_Down, false), QPoint(0, 1));
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_Left, true), QPoint(-10, 0));
        QCOMPARE(markshot::shot::selectionCursorNudgeDelta(Qt::Key_A, false), QPoint(0, 0));
    }

    /**
     * 验证微调结果会被限制在图像范围内。
     * @return 无返回值。
     */
    void nudgeStaysInsideImageBounds()
    {
        const QRectF bounds(0.0, 0.0, 99.0, 49.0);
        QCOMPARE(markshot::shot::nudgeSelectionCursor(QPointF(0.0, 10.0), QPoint(-1, 0), bounds),
                 QPointF(0.0, 10.0));
        QCOMPARE(markshot::shot::nudgeSelectionCursor(QPointF(99.0, 49.0), QPoint(1, 1), bounds),
                 QPointF(99.0, 49.0));
        QCOMPARE(markshot::shot::nudgeSelectionCursor(QPointF(20.0, 20.0), QPoint(1, -1), bounds),
                 QPointF(21.0, 19.0));
    }

    /**
     * 验证 setPos 失败时判定为未到达目标，硬件光标移动后才重新附着。
     * @return 无返回值。
     */
    void warpFailureKeepsPointerDetachedUntilHardwareMoves()
    {
        QVERIFY(markshot::shot::cursorReachedWarpTarget(QPoint(10, 10), QPoint(10, 11)));
        QVERIFY(!markshot::shot::cursorReachedWarpTarget(QPoint(10, 10), QPoint(40, 10)));
        QVERIFY(!markshot::shot::hardwarePointerMoved(QPoint(10, 10), QPoint(11, 10)));
        QVERIFY(markshot::shot::hardwarePointerMoved(QPoint(10, 10), QPoint(13, 10)));
    }
};

QTEST_APPLESS_MAIN(SelectionCursorNudgeTest)

#include "selection_cursor_nudge_test.moc"

#include "shot_window_line_constraint.h"

#include <QPointF>
#include <QtTest/QtTest>

#include <cmath>

class ShotWindowLineConstraintTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证接近水平方向时终点吸附为水平线。
     * @return 无返回值。
     */
    void snapsToHorizontal()
    {
        const QPointF end = markshot::shot::constrainedLineEnd(QPointF(0.0, 0.0), QPointF(10.0, 2.0));

        QVERIFY(std::abs(end.y()) < 0.0001);
        QVERIFY(end.x() > 9.0);
    }

    /**
     * 验证接近垂直方向时终点吸附为垂直线。
     * @return 无返回值。
     */
    void snapsToVertical()
    {
        const QPointF end = markshot::shot::constrainedLineEnd(QPointF(0.0, 0.0), QPointF(2.0, 10.0));

        QVERIFY(std::abs(end.x()) < 0.0001);
        QVERIFY(end.y() > 9.0);
    }

    /**
     * 验证接近 45 度方向时终点吸附为 45 度线。
     * @return 无返回值。
     */
    void snapsToFortyFiveDegrees()
    {
        const QPointF end = markshot::shot::constrainedLineEnd(QPointF(0.0, 0.0), QPointF(10.0, 7.0));

        QVERIFY(std::abs(end.x() - end.y()) < 0.0001);
        QVERIFY(end.x() > 8.0);
    }

    /**
     * 验证反向斜线也会吸附到 45 度倍数方向。
     * @return 无返回值。
     */
    void snapsToReverseFortyFiveDegrees()
    {
        const QPointF end = markshot::shot::constrainedLineEnd(QPointF(0.0, 0.0), QPointF(-10.0, 7.0));

        QVERIFY(std::abs(std::abs(end.x()) - std::abs(end.y())) < 0.0001);
        QVERIFY(end.x() < -8.0);
        QVERIFY(end.y() > 8.0);
    }
};

QTEST_APPLESS_MAIN(ShotWindowLineConstraintTest)

#include "shot_window_line_constraint_test.moc"

#include "selection_loupe.h"

#include <QtTest/QtTest>

class SelectionLoupeTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证放大镜默认放在光标右下方，并取样 13x13 像素。
     * @return 无返回值。
     */
    void placesLoupeBelowAndRightOfCursor()
    {
        const markshot::shot::SelectionLoupeLayout layout =
            markshot::shot::selectionLoupeLayout(QPointF(80.0, 90.0),
                                                 QSize(800, 600),
                                                 112.0,
                                                 QPointF(40.0, 50.0),
                                                 QSize(200, 200));

        QCOMPARE(layout.loupe.topLeft(), QPointF(102.0, 112.0));
        QCOMPARE(layout.loupe.size(), QSizeF(112.0, 112.0));
        QCOMPARE(layout.sourceRect, QRect(34, 44, 13, 13));
    }

    /**
     * 验证靠近右下角时放大镜翻到光标左上方。
     * @return 无返回值。
     */
    void flipsLoupeNearBottomRight()
    {
        const markshot::shot::SelectionLoupeLayout layout =
            markshot::shot::selectionLoupeLayout(QPointF(760.0, 560.0),
                                                 QSize(800, 600),
                                                 112.0,
                                                 QPointF(190.0, 190.0),
                                                 QSize(200, 200));

        QVERIFY(layout.loupe.right() <= 800.0 - 12.0);
        QVERIFY(layout.loupe.bottom() <= 600.0 - 12.0);
        QVERIFY(layout.loupe.right() < 760.0);
        QVERIFY(layout.loupe.bottom() < 560.0);
    }
};

QTEST_APPLESS_MAIN(SelectionLoupeTest)

#include "selection_loupe_test.moc"

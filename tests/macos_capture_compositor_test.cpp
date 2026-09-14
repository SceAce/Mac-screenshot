#include "platform/macos/macos_capture_compositor.h"

#include <QtTest>

class MacosCaptureCompositorTest : public QObject {
    Q_OBJECT

    static CaptureResult frame(QRect geometry, int scale, QColor color)
    {
        QImage image(geometry.size() * scale, QImage::Format_ARGB32_Premultiplied);
        image.fill(color);
        return {image, {}, {}, geometry};
    }

private slots:
    void retinaRegionKeepsPixelsAndOrientation()
    {
        CaptureResult display = frame(QRect(0, 0, 100, 80), 2, Qt::blue);
        display.image.setPixelColor(20, 30, Qt::red);
        const auto result = markshot::macos::composeDisplayCaptures({display}, QRect(10, 15, 20, 25));
        QCOMPARE(result.image.size(), QSize(40, 50));
        QCOMPARE(result.image.pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(result.image.pixelColor(39, 49), QColor(Qt::blue));
        QCOMPARE(result.image.devicePixelRatio(), 1.0);
        QCOMPARE(result.sourceGeometry, QRect(10, 15, 20, 25));
    }

    void mixedScaleDisplaysWithNegativeOrigin()
    {
        const auto left = frame(QRect(-100, -20, 100, 80), 1, Qt::red);
        const auto right = frame(QRect(0, 0, 100, 80), 2, Qt::blue);
        const auto result = markshot::macos::composeDisplayCaptures({left, right}, QRect(-100, -20, 200, 100));
        QCOMPARE(result.image.size(), QSize(400, 200));
        QCOMPARE(result.image.pixelColor(199, 50), QColor(Qt::red));
        QCOMPARE(result.image.pixelColor(200, 50), QColor(Qt::blue));
        QCOMPARE(result.image.pixelColor(399, 199), QColor(Qt::blue));
        QCOMPARE(result.image.pixelColor(300, 0).alpha(), 0);
    }

    void rejectsEmptyDisplayAndExcessiveAllocation()
    {
        QVERIFY(markshot::macos::composeDisplayCaptures({}, QRect(0, 0, 100, 100)).image.isNull());
        const auto display = frame(QRect(0, 0, 100, 100), 2, Qt::blue);
        const auto result = markshot::macos::composeDisplayCaptures({display}, QRect(0, 0, 100000, 100000));
        QVERIFY(result.image.isNull());
        QVERIFY(!result.error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(MacosCaptureCompositorTest)
#include "macos_capture_compositor_test.moc"

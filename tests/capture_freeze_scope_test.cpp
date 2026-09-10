#include "capture_freeze_scope.h"

#include <QJsonObject>
#include <QtTest/QtTest>

class CaptureFreezeScopeTest : public QObject {
    Q_OBJECT

private slots:
    void parseCursorScreenAliases()
    {
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("cursor-screen")).value(),
                 markshot::CaptureFreezeScope::CursorScreen);
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("mouse_screen")).value(),
                 markshot::CaptureFreezeScope::CursorScreen);
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("current screen")).value(),
                 markshot::CaptureFreezeScope::CursorScreen);
    }

    void parseAllScreensAliases()
    {
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("all-screens")).value(),
                 markshot::CaptureFreezeScope::AllScreens);
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("all_displays")).value(),
                 markshot::CaptureFreezeScope::AllScreens);
        QCOMPARE(markshot::captureFreezeScopeFromText(QStringLiteral("virtual desktop")).value(),
                 markshot::CaptureFreezeScope::AllScreens);
    }

    void invalidTextIsEmpty()
    {
        QVERIFY(!markshot::captureFreezeScopeFromText(QStringLiteral("invalid")).has_value());
        QVERIFY(!markshot::captureFreezeScopeFromText(QString()).has_value());
    }

    void configRootReadsNestedCaptureValue()
    {
        QJsonObject capture;
        capture.insert(QStringLiteral("freezeScope"), QStringLiteral("cursor-screen"));
        QJsonObject root;
        root.insert(QStringLiteral("capture"), capture);

        QCOMPARE(markshot::captureFreezeScopeFromConfigRoot(root),
                 markshot::CaptureFreezeScope::CursorScreen);
    }

    void configRootDefaultsToAllScreens()
    {
        QCOMPARE(markshot::defaultCaptureFreezeScope(),
                 markshot::CaptureFreezeScope::AllScreens);
        QCOMPARE(markshot::captureFreezeScopeFromConfigRoot(QJsonObject()),
                 markshot::CaptureFreezeScope::AllScreens);
    }
};

QTEST_MAIN(CaptureFreezeScopeTest)
#include "capture_freeze_scope_test.moc"

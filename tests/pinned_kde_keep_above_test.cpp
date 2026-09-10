#include "pinned_window/pinned_kde_keep_above.h"

#include <QtTest/QtTest>

class PinnedKdeKeepAboveTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证中英文钉图标题和 OCR 标题都会匹配。
     * @return 无返回值。
     */
    void matchesPinnedAndOcrTitles()
    {
        QVERIFY(markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("Pinned Mark Shot")));
        QVERIFY(markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("钉住的截图")));
        QVERIFY(markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("OCR Result")));
        QVERIFY(markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("OCR 结果")));
        QVERIFY(!markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("Mark Shot")));
        QVERIFY(!markshot::shot::isKdePinnedKeepAboveTitle(QStringLiteral("Settings")));
    }

    /**
     * 验证置顶脚本会设置 keepAbove 并监听新建窗口。
     * @return 无返回值。
     */
    void enableScriptSetsKeepAboveAndListens()
    {
        const QString source =
            markshot::shot::kdePinnedKeepAboveScriptSource(QStringLiteral("钉住的截图"), true);

        QVERIFY(source.contains(QStringLiteral("const targetTitle = '钉住的截图';")));
        QVERIFY(source.contains(QStringLiteral("const keepAbove = true;")));
        QVERIFY(source.contains(QStringLiteral("const listenAdded = true;")));
        QVERIFY(source.contains(QStringLiteral("window.keepAbove = keepAbove;")));
    }

    /**
     * 验证取消置顶脚本只改写现有窗口，不再常驻监听。
     * @return 无返回值。
     */
    void disableScriptClearsKeepAboveWithoutListener()
    {
        const QString source =
            markshot::shot::kdePinnedKeepAboveScriptSource(QStringLiteral("Pinned Mark Shot"),
                                                           false);

        QVERIFY(source.contains(QStringLiteral("const targetTitle = 'Pinned Mark Shot';")));
        QVERIFY(source.contains(QStringLiteral("const keepAbove = false;")));
        QVERIFY(source.contains(QStringLiteral("const listenAdded = false;")));
    }

    /**
     * 验证标题中的引号会被转义进 JavaScript 字面量。
     * @return 无返回值。
     */
    void escapesQuotesInTitleLiteral()
    {
        const QString source =
            markshot::shot::kdePinnedKeepAboveScriptSource(QStringLiteral("Pinned O'Clock"),
                                                           true);

        QVERIFY(source.contains(QStringLiteral("const targetTitle = 'Pinned O\\'Clock';")));
    }
};

QTEST_APPLESS_MAIN(PinnedKdeKeepAboveTest)

#include "pinned_kde_keep_above_test.moc"

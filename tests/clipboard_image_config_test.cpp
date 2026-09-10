#include "clipboard_image_config.h"

#include <QJsonObject>
#include <QtTest/QtTest>

class ClipboardImageConfigTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证缺省配置使用 image/png 模式。
     * @return 无返回值。
     */
    void defaultModeCopiesImagePng()
    {
        const markshot::ClipboardImageConfig config =
            markshot::clipboardImageConfigFromRoot(QJsonObject());

        QCOMPARE(config.mode, markshot::ClipboardImageMode::ImagePng);
        QCOMPARE(config.thresholdM, 4);
    }

    /**
     * 验证固定 URL 模式可以从配置读取。
     * @return 无返回值。
     */
    void readsUrlMode()
    {
        QJsonObject image;
        image.insert(QStringLiteral("mode"), QStringLiteral("url"));
        QJsonObject clipboard;
        clipboard.insert(QStringLiteral("image"), image);
        QJsonObject root;
        root.insert(QStringLiteral("clipboard"), clipboard);

        const markshot::ClipboardImageConfig config =
            markshot::clipboardImageConfigFromRoot(root);

        QCOMPARE(config.mode, markshot::ClipboardImageMode::Url);
    }

    /**
     * 验证阈值模式使用 M 作为配置单位。
     * @return 无返回值。
     */
    void readsThresholdModeInM()
    {
        QJsonObject image;
        image.insert(QStringLiteral("mode"), QStringLiteral("threshold"));
        image.insert(QStringLiteral("thresholdM"), 8);
        QJsonObject clipboard;
        clipboard.insert(QStringLiteral("image"), image);
        QJsonObject root;
        root.insert(QStringLiteral("clipboard"), clipboard);

        const markshot::ClipboardImageConfig config =
            markshot::clipboardImageConfigFromRoot(root);

        QCOMPARE(config.mode, markshot::ClipboardImageMode::Threshold);
        QCOMPARE(config.thresholdM, 8);
        QCOMPARE(markshot::clipboardImageThresholdBytes(config), qsizetype(8 * 1024 * 1024));
    }

    /**
     * 验证非法模式保留默认 image/png。
     * @return 无返回值。
     */
    void invalidModeKeepsDefault()
    {
        QJsonObject image;
        image.insert(QStringLiteral("mode"), QStringLiteral("unknown"));
        QJsonObject clipboard;
        clipboard.insert(QStringLiteral("image"), image);
        QJsonObject root;
        root.insert(QStringLiteral("clipboard"), clipboard);

        const markshot::ClipboardImageConfig config =
            markshot::clipboardImageConfigFromRoot(root);

        QCOMPARE(config.mode, markshot::ClipboardImageMode::ImagePng);
    }
};

QTEST_APPLESS_MAIN(ClipboardImageConfigTest)

#include "clipboard_image_config_test.moc"

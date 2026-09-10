#include "selection_loupe_config.h"

#include <QJsonObject>
#include <QtTest/QtTest>

class SelectionLoupeConfigTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证缺省关闭选区放大镜。
     * @return 无返回值。
     */
    void defaultsToDisabled()
    {
        QCOMPARE(markshot::defaultSelectionLoupeEnabled(), false);
        QCOMPARE(markshot::selectionLoupeEnabledFromConfigRoot(QJsonObject()), false);
    }

    /**
     * 验证嵌套 enabled 字段可以打开开关。
     * @return 无返回值。
     */
    void readsNestedEnabledObject()
    {
        QJsonObject loupe;
        loupe.insert(QStringLiteral("enabled"), true);
        QJsonObject capture;
        capture.insert(QStringLiteral("selectionLoupe"), loupe);
        QJsonObject root;
        root.insert(QStringLiteral("capture"), capture);

        QCOMPARE(markshot::selectionLoupeEnabledFromConfigRoot(root), true);
    }

    /**
     * 验证布尔简写字段可以打开开关。
     * @return 无返回值。
     */
    void readsBooleanShortcut()
    {
        QJsonObject capture;
        capture.insert(QStringLiteral("selectionLoupeEnabled"), true);
        QJsonObject root;
        root.insert(QStringLiteral("capture"), capture);

        QCOMPARE(markshot::selectionLoupeEnabledFromConfigRoot(root), true);
    }
};

QTEST_APPLESS_MAIN(SelectionLoupeConfigTest)

#include "selection_loupe_config_test.moc"

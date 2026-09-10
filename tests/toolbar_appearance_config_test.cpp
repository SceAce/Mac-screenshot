#include "toolbar_appearance_config.h"

#include <QJsonObject>
#include <QtTest/QtTest>

class ToolbarAppearanceConfigTest : public QObject {
    Q_OBJECT

private slots:
    /// @brief 验证默认配置保持当前工具栏尺寸。
    /// @return 无返回值。
    void defaultsMatchCurrentToolbarSizes()
    {
        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(QJsonObject());

        QCOMPARE(config.toolbarIconSize, 22);
        QCOMPARE(config.actionToolbarIconSize, 20);
        QCOMPARE(config.fontSize, 11);
        QCOMPARE(config.toolbarButtonSize, 30);
        QCOMPARE(config.actionToolbarButtonSize, 28);
    }

    /// @brief 验证命名预设可以解析为图标和字体尺寸。
    /// @return 无返回值。
    void readsNamedPresets()
    {
        QJsonObject toolbar;
        toolbar.insert(QStringLiteral("iconSize"), QStringLiteral("large"));
        toolbar.insert(QStringLiteral("fontSize"), QStringLiteral("small"));
        QJsonObject root;
        root.insert(QStringLiteral("toolbar"), toolbar);

        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(root);

        QCOMPARE(config.toolbarIconSize, 28);
        QCOMPARE(config.actionToolbarIconSize, 28);
        QCOMPARE(config.fontSize, 10);
        QCOMPARE(config.toolbarButtonSize, 36);
        QCOMPARE(config.actionToolbarButtonSize, 36);
    }

    /// @brief 验证数字值可以直接作为尺寸配置。
    /// @return 无返回值。
    void readsNumericValues()
    {
        QJsonObject toolbar;
        toolbar.insert(QStringLiteral("iconSize"), 24);
        toolbar.insert(QStringLiteral("fontSize"), 14);
        QJsonObject root;
        root.insert(QStringLiteral("toolbar"), toolbar);

        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(root);

        QCOMPARE(config.toolbarIconSize, 24);
        QCOMPARE(config.actionToolbarIconSize, 24);
        QCOMPARE(config.fontSize, 14);
        QCOMPARE(config.toolbarButtonSize, 33);
        QCOMPARE(config.actionToolbarButtonSize, 33);
    }

    /// @brief 验证字符串数字、px 后缀和别名键可以正常解析。
    /// @return 无返回值。
    void readsStringNumbersAndAliases()
    {
        QJsonObject toolbar;
        toolbar.insert(QStringLiteral("icon-size"), QStringLiteral("30px"));
        toolbar.insert(QStringLiteral("font-size"), QStringLiteral("12"));
        QJsonObject annotation;
        annotation.insert(QStringLiteral("toolbar"), toolbar);
        QJsonObject root;
        root.insert(QStringLiteral("annotation"), annotation);

        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(root);

        QCOMPARE(config.toolbarIconSize, 30);
        QCOMPARE(config.actionToolbarIconSize, 30);
        QCOMPARE(config.fontSize, 12);
        QCOMPARE(config.toolbarButtonSize, 38);
        QCOMPARE(config.actionToolbarButtonSize, 38);
    }

    /// @brief 验证非法值保持默认配置。
    /// @return 无返回值。
    void invalidValuesKeepDefaults()
    {
        QJsonObject toolbar;
        toolbar.insert(QStringLiteral("iconSize"), QStringLiteral("huge"));
        toolbar.insert(QStringLiteral("fontSize"), QJsonObject());
        QJsonObject root;
        root.insert(QStringLiteral("toolbar"), toolbar);

        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(root);

        QCOMPARE(config.toolbarIconSize, 22);
        QCOMPARE(config.actionToolbarIconSize, 20);
        QCOMPARE(config.fontSize, 11);
        QCOMPARE(config.toolbarButtonSize, 30);
        QCOMPARE(config.actionToolbarButtonSize, 28);
    }

    /// @brief 验证超出范围的数字会被限制到允许范围。
    /// @return 无返回值。
    void clampsNumericValues()
    {
        QJsonObject toolbar;
        toolbar.insert(QStringLiteral("iconSize"), 100);
        toolbar.insert(QStringLiteral("fontSize"), 1);
        QJsonObject root;
        root.insert(QStringLiteral("toolbar"), toolbar);

        const markshot::ToolbarAppearanceConfig config = markshot::toolbarAppearanceFromConfigRoot(root);

        QCOMPARE(config.toolbarIconSize, 48);
        QCOMPARE(config.actionToolbarIconSize, 48);
        QCOMPARE(config.fontSize, 8);
        QCOMPARE(config.toolbarButtonSize, 56);
        QCOMPARE(config.actionToolbarButtonSize, 56);
    }
};

QTEST_APPLESS_MAIN(ToolbarAppearanceConfigTest)

#include "toolbar_appearance_config_test.moc"

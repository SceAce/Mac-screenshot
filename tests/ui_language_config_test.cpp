#include "ui/interface_language_config.h"

#include <QtTest/QtTest>

/// @brief 界面语言配置测试。
class UiLanguageConfigTest : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 验证常见配置名称可以解析为语言模式。
    void parsesLanguageModeNames()
    {
        QCOMPARE(markshot::ui::uiLanguageModeFromString(QStringLiteral("system")),
                 markshot::ui::UiLanguageMode::System);
        QCOMPARE(markshot::ui::uiLanguageModeFromString(QStringLiteral("english")),
                 markshot::ui::UiLanguageMode::English);
        QCOMPARE(markshot::ui::uiLanguageModeFromString(QStringLiteral("zh-CN")),
                 markshot::ui::UiLanguageMode::Chinese);
    }

    /// @brief 验证语言模式可以写回稳定配置名称。
    void writesStableConfigNames()
    {
        QCOMPARE(markshot::ui::uiLanguageModeName(markshot::ui::UiLanguageMode::System),
                 QStringLiteral("system"));
        QCOMPARE(markshot::ui::uiLanguageModeName(markshot::ui::UiLanguageMode::English),
                 QStringLiteral("english"));
        QCOMPARE(markshot::ui::uiLanguageModeName(markshot::ui::UiLanguageMode::Chinese),
                 QStringLiteral("chinese"));
    }

    /// @brief 验证可以从应用配置根对象读取界面语言模式。
    void readsConfigRoot()
    {
        QJsonObject ui;
        ui.insert(QStringLiteral("language"), QStringLiteral("english"));

        QJsonObject root;
        root.insert(QStringLiteral("ui"), ui);

        QCOMPARE(markshot::ui::uiLanguageModeFromConfigRoot(root),
                 markshot::ui::UiLanguageMode::English);
        QCOMPARE(markshot::ui::uiLanguageModeFromConfigRoot(QJsonObject()),
                 markshot::ui::UiLanguageMode::System);
    }
};

/// @brief 界面语言配置测试入口。
/// @param argc 参数数量。
/// @param argv 参数数组。
/// @return 进程退出码。
QTEST_APPLESS_MAIN(UiLanguageConfigTest)

#include "ui_language_config_test.moc"

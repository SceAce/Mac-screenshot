#include "config_value.h"

#include <QtTest/QtTest>

/// @brief Test suite for testing configurations and JSON parsers.
class ConfigValueTest : public QObject
{
    /// @brief Qt private signal structure.
    /// @brief Qt meta-object instance.
    Q_OBJECT

private slots:
    void readsObjectValuesWithBothSelectionModes()
    {
        QJsonObject root;
        root.insert(QStringLiteral("empty"), QJsonObject());

        QJsonObject child;
        child.insert(QStringLiteral("enabled"), true);
        root.insert(QStringLiteral("child"), child);

        QCOMPARE(markshot::config::objectValue(root, QStringLiteral("child")), child);
        QCOMPARE(markshot::config::firstObjectValue(
                     root, {QStringLiteral("empty"), QStringLiteral("child")}),
                 QJsonObject());
        QCOMPARE(markshot::config::firstNonEmptyObjectValue(
                     root, {QStringLiteral("empty"), QStringLiteral("child")}),
                 child);
    }

    void parsesCommonBooleanForms()
    {
        QCOMPARE(markshot::config::boolValue(QJsonValue(true)), std::optional<bool>(true));
        QCOMPARE(markshot::config::boolValue(QJsonValue(QStringLiteral("enabled"))), std::optional<bool>(true));
        QCOMPARE(markshot::config::boolValue(QJsonValue(QStringLiteral("off"))), std::optional<bool>(false));
        QVERIFY(!markshot::config::boolValue(QJsonValue(QStringLiteral("maybe"))).has_value());
    }

    void stringifiesEnvironmentValues()
    {
        QCOMPARE(markshot::config::environmentStringValue(QJsonValue(QStringLiteral("text"))),
                 std::optional<QString>(QStringLiteral("text")));
        QCOMPARE(markshot::config::environmentStringValue(QJsonValue(false)),
                 std::optional<QString>(QStringLiteral("0")));
        QCOMPARE(markshot::config::environmentStringValue(QJsonValue(12.5)),
                 std::optional<QString>(QStringLiteral("12.5")));
    }

    void parsesPortableAndNativeShortcuts()
    {
        const std::optional<QKeySequence> portable =
            markshot::config::keySequenceValue(QJsonValue(QStringLiteral("Ctrl+Shift+S")));
        QVERIFY(portable.has_value());
        QCOMPARE(portable->toString(QKeySequence::PortableText), QStringLiteral("Ctrl+Shift+S"));

        QVERIFY(!markshot::config::keySequenceValue(QJsonValue(QString())).has_value());
        QVERIFY(!markshot::config::keySequenceValue(QJsonValue(42)).has_value());
    }
};

/// @brief Main function for the ConfigValue test suite.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return Standard C++ exit code.
QTEST_APPLESS_MAIN(ConfigValueTest)

#include "config_value_test.moc"

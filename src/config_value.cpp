#include "config_value.h"

#include <algorithm>

namespace markshot::config {

QJsonObject objectValue(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isObject() ? value.toObject() : QJsonObject();
}

QJsonObject firstObjectValue(const QJsonObject &object, const QString &key)
{
    return objectValue(object, key);
}

QJsonObject firstObjectValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isObject()) {
            return value.toObject();
        }
    }
    return {};
}

QJsonObject firstNonEmptyObjectValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonObject child = objectValue(object, key);
        if (!child.isEmpty()) {
            return child;
        }
    }
    return {};
}

QJsonValue valueForKeys(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (!value.isUndefined()) {
            return value;
        }
    }
    return {};
}

QString normalizedKey(QString key)
{
    key = key.trimmed().toLower();
    key.remove(QLatin1Char(' '));
    key.remove(QLatin1Char('_'));
    key.remove(QLatin1Char('-'));
    key.remove(QLatin1Char('.'));
    return key;
}

std::optional<bool> boolValue(const QJsonValue &value)
{
    if (value.isBool()) {
        return value.toBool();
    }
    if (value.isDouble()) {
        return !qFuzzyIsNull(value.toDouble());
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("1")
            || text == QStringLiteral("true")
            || text == QStringLiteral("yes")
            || text == QStringLiteral("on")
            || text == QStringLiteral("enabled")) {
            return true;
        }
        if (text == QStringLiteral("0")
            || text == QStringLiteral("false")
            || text == QStringLiteral("no")
            || text == QStringLiteral("off")
            || text == QStringLiteral("disabled")) {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<bool> boolValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isBool()) {
            return value.toBool();
        }
    }
    return std::nullopt;
}

std::optional<int> intValue(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toInt();
    }
    if (value.isString()) {
        bool ok = false;
        const int number = value.toString().trimmed().toInt(&ok);
        if (ok) {
            return number;
        }
    }
    return std::nullopt;
}

std::optional<int> clampedIntValue(const QJsonValue &value, int minimum, int maximum)
{
    if (const std::optional<int> number = intValue(value)) {
        return std::clamp(*number, minimum, maximum);
    }
    return std::nullopt;
}

std::optional<QString> environmentStringValue(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
    }
    return std::nullopt;
}

std::optional<QKeySequence> keySequenceValue(const QJsonValue &value)
{
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return std::nullopt;
    }

    QKeySequence sequence(text, QKeySequence::PortableText);
    if (sequence.isEmpty()) {
        sequence = QKeySequence(text, QKeySequence::NativeText);
    }
    return sequence.isEmpty() ? std::nullopt : std::optional<QKeySequence>(sequence);
}

}  // namespace markshot::config

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QKeySequence>
#include <QString>
#include <QStringList>

#include <optional>

namespace markshot::config {

// JSON helpers used by startup config, shortcut config, capture behavior, and
// extension settings. They accept a small amount of schema drift so older config
// files and human-edited files continue to work.
QJsonObject objectValue(const QJsonObject &object, const QString &key);
QJsonObject firstObjectValue(const QJsonObject &object, const QString &key);
QJsonObject firstObjectValue(const QJsonObject &object, const QStringList &keys);
QJsonObject firstNonEmptyObjectValue(const QJsonObject &object, const QStringList &keys);
QJsonValue valueForKeys(const QJsonObject &object, const QStringList &keys);

// Normalizes user-facing keys before fallback lookup, for example by trimming
// and smoothing common separator differences.
QString normalizedKey(QString key);

// Type readers return std::nullopt when a value is absent or unsupported. Callers
// decide whether to keep defaults, clamp ranges, or emit a warning.
std::optional<bool> boolValue(const QJsonValue &value);
std::optional<bool> boolValue(const QJsonObject &object, const QStringList &keys);
std::optional<int> intValue(const QJsonValue &value);
std::optional<int> clampedIntValue(const QJsonValue &value, int minimum, int maximum);
std::optional<QString> environmentStringValue(const QJsonValue &value);
std::optional<QKeySequence> keySequenceValue(const QJsonValue &value);

}  // namespace markshot::config

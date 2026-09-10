#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace markshot::code_scan {

struct Result {
    QString format;
    QString text;
    QVector<QPointF> points;
    QRectF imageRect;
    qreal confidence = 0.0;
};

struct ParsedOutput {
    bool validJson = false;
    QString backend;
    QStringList errors;
    QVector<Result> results;
};

std::optional<QPointF> pointFromJsonValue(const QJsonValue &value);
QVector<QPointF> pointsFromJsonValue(const QJsonValue &value);
std::optional<QRectF> rectFromJsonValue(const QJsonValue &value);
ParsedOutput parseOutput(const QByteArray &output, QSize imageSize = {});
QString resultText(const Result &result);
QString resultsText(const QVector<Result> &results);
bool outputReportsMissingBackend(const QByteArray &stdoutData, const QByteArray &stderrData);

}  // namespace markshot::code_scan

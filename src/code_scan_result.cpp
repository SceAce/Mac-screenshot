#include "code_scan_result.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QSizeF>

#include <algorithm>
#include <utility>

namespace markshot::code_scan {
namespace {

/**
 * 从对象的多个候选键中读取字符串。
 * @param object JSON 对象。
 * @param keys 候选键列表。
 * @return 第一个非空字符串。
 */
QString firstStringValue(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QString value = object.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

/**
 * 从 JSON 值中读取字符串列表。
 * @param value JSON 值。
 * @return 解析出的字符串列表。
 */
QStringList stringListFromJsonValue(const QJsonValue &value)
{
    QStringList items;
    if (value.isString()) {
        items.append(value.toString());
        return items;
    }
    if (!value.isArray()) {
        return items;
    }
    for (const QJsonValue &item : value.toArray()) {
        if (item.isString()) {
            items.append(item.toString());
        }
    }
    return items;
}

/**
 * 根据点集合计算外接矩形。
 * @param points 码位置点集合。
 * @return 非空外接矩形；点集合为空时返回 std::nullopt。
 */
std::optional<QRectF> rectFromPoints(const QVector<QPointF> &points)
{
    if (points.isEmpty()) {
        return std::nullopt;
    }

    qreal minX = points.first().x();
    qreal minY = points.first().y();
    qreal maxX = minX;
    qreal maxY = minY;
    for (const QPointF &point : points) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
}

/**
 * 从对象读取码位置矩形。
 * @param object 单个识别结果对象。
 * @return 解析出的矩形；没有位置时返回 std::nullopt。
 */
std::optional<QRectF> resultRect(const QJsonObject &object)
{
    for (const QString &key : {QStringLiteral("box"),
                               QStringLiteral("bbox"),
                               QStringLiteral("rect"),
                               QStringLiteral("bounds")}) {
        if (object.contains(key)) {
            return rectFromJsonValue(object.value(key));
        }
    }
    return rectFromPoints(pointsFromJsonValue(object.value(QStringLiteral("points"))));
}

/**
 * 判断错误信息是否表示扫码后端缺失。
 * @param errorText 错误文本。
 * @return 缺少后端时返回 true，否则返回 false。
 */
bool missingBackendError(QString errorText)
{
    const QString text = errorText.toLower();
    return text.contains(QStringLiteral("no module named"))
        || text.contains(QStringLiteral("modulenotfounderror"))
        || text.contains(QStringLiteral("importerror"))
        || text.contains(QStringLiteral("not installed"))
        || text.contains(QStringLiteral("not found"))
        || text.contains(QStringLiteral("no supported barcode scanner backend"));
}

}  // namespace

/**
 * 从 JSON 值中解析单个点。
 * @param value JSON 点值，支持 [x, y] 或包含 x/y 的对象。
 * @return 解析出的点；格式无效时返回 std::nullopt。
 */
std::optional<QPointF> pointFromJsonValue(const QJsonValue &value)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (array.size() >= 2 && array.at(0).isDouble() && array.at(1).isDouble()) {
            return QPointF(array.at(0).toDouble(), array.at(1).toDouble());
        }
        return std::nullopt;
    }

    if (!value.isObject()) {
        return std::nullopt;
    }
    const QJsonObject object = value.toObject();
    if (object.value(QStringLiteral("x")).isDouble() && object.value(QStringLiteral("y")).isDouble()) {
        return QPointF(object.value(QStringLiteral("x")).toDouble(),
                       object.value(QStringLiteral("y")).toDouble());
    }
    return std::nullopt;
}

/**
 * 从 JSON 值中解析点集合。
 * @param value JSON 点集合。
 * @return 成功解析出的点集合。
 */
QVector<QPointF> pointsFromJsonValue(const QJsonValue &value)
{
    QVector<QPointF> points;
    if (!value.isArray()) {
        return points;
    }

    const QJsonArray array = value.toArray();
    points.reserve(array.size());
    for (const QJsonValue &pointValue : array) {
        if (const std::optional<QPointF> point = pointFromJsonValue(pointValue)) {
            points.append(*point);
        }
    }
    return points;
}

/**
 * 从 JSON 值中解析矩形。
 * @param value JSON 矩形值，支持 [x, y, width, height] 或点集合。
 * @return 解析出的矩形；格式无效时返回 std::nullopt。
 */
std::optional<QRectF> rectFromJsonValue(const QJsonValue &value)
{
    if (!value.isArray()) {
        return std::nullopt;
    }

    const QJsonArray array = value.toArray();
    if (array.size() == 4
        && array.at(0).isDouble()
        && array.at(1).isDouble()
        && array.at(2).isDouble()
        && array.at(3).isDouble()) {
        return QRectF(array.at(0).toDouble(),
                      array.at(1).toDouble(),
                      array.at(2).toDouble(),
                      array.at(3).toDouble()).normalized();
    }

    return rectFromPoints(pointsFromJsonValue(value));
}

/**
 * 解析扫码 helper 的 JSON 输出。
 * @param output helper 标准输出。
 * @param imageSize 选区图片尺寸，用于裁剪位置范围。
 * @return 解析后的扫码结果。
 */
ParsedOutput parseOutput(const QByteArray &output, QSize imageSize)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return {};
    }

    QJsonArray resultArray;
    ParsedOutput parsed;
    parsed.validJson = true;

    if (document.isArray()) {
        resultArray = document.array();
    } else if (document.isObject()) {
        const QJsonObject root = document.object();
        parsed.backend = root.value(QStringLiteral("backend")).toString().trimmed();
        parsed.errors = stringListFromJsonValue(root.value(QStringLiteral("errors")));
        for (const QString &key : {QStringLiteral("results"),
                                   QStringLiteral("codes"),
                                   QStringLiteral("barcodes")}) {
            if (root.value(key).isArray()) {
                resultArray = root.value(key).toArray();
                break;
            }
        }
    }

    const bool clipToImage = imageSize.isValid() && !imageSize.isEmpty();
    const QRectF imageBounds(QPointF(0.0, 0.0), QSizeF(imageSize));
    parsed.results.reserve(resultArray.size());

    for (const QJsonValue &value : resultArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        Result result;
        result.text = firstStringValue(object,
                                       {QStringLiteral("text"),
                                        QStringLiteral("value"),
                                        QStringLiteral("data"),
                                        QStringLiteral("rawValue"),
                                        QStringLiteral("content")});
        if (result.text.isEmpty()) {
            continue;
        }

        result.format = firstStringValue(object,
                                         {QStringLiteral("format"),
                                          QStringLiteral("type"),
                                          QStringLiteral("symbology")});
        result.points = pointsFromJsonValue(object.value(QStringLiteral("points")));
        result.confidence = object.value(QStringLiteral("confidence")).toDouble(0.0);
        if (const std::optional<QRectF> rect = resultRect(object)) {
            result.imageRect = rect->normalized();
            if (clipToImage) {
                result.imageRect = result.imageRect.intersected(imageBounds);
            }
        }

        parsed.results.append(result);
    }

    return parsed;
}

/**
 * 生成单个扫码结果的展示文本。
 * @param result 单个扫码结果。
 * @return 可展示和复制的文本。
 */
QString resultText(const Result &result)
{
    const QString title = result.format.trimmed().isEmpty()
        ? QStringLiteral("Code")
        : result.format.trimmed();
    return QStringLiteral("%1\n%2").arg(title, result.text);
}

/**
 * 生成多个扫码结果的展示文本。
 * @param results 扫码结果列表。
 * @return 可展示和复制的文本。
 */
QString resultsText(const QVector<Result> &results)
{
    if (results.isEmpty()) {
        return {};
    }
    if (results.size() == 1) {
        return resultText(results.first());
    }

    QStringList sections;
    sections.reserve(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const Result &result = results.at(i);
        const QString title = result.format.trimmed().isEmpty()
            ? QStringLiteral("Code")
            : result.format.trimmed();
        sections.append(QStringLiteral("%1. %2\n%3").arg(i + 1).arg(title, result.text));
    }
    return sections.join(QStringLiteral("\n\n"));
}

/**
 * 判断扫码 helper 输出是否表示识别后端缺失。
 * @param stdoutData helper 标准输出。
 * @param stderrData helper 标准错误。
 * @return 缺少后端时返回 true，否则返回 false。
 */
bool outputReportsMissingBackend(const QByteArray &stdoutData, const QByteArray &stderrData)
{
    QStringList errors;
    const ParsedOutput parsed = parseOutput(stdoutData);
    if (parsed.validJson) {
        errors = parsed.errors;
    }
    if (!stderrData.trimmed().isEmpty()) {
        errors.append(QString::fromUtf8(stderrData).trimmed());
    }
    if (!parsed.validJson && !stdoutData.trimmed().isEmpty()) {
        errors.append(QString::fromUtf8(stdoutData).trimmed());
    }

    for (const QString &error : std::as_const(errors)) {
        if (missingBackendError(error)) {
            return true;
        }
    }
    return false;
}

}  // namespace markshot::code_scan

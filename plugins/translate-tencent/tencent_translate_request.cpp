#include "tencent_translate_request.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace markshot::translate_tencent {
namespace {

/** 源语言固定为自动识别。 */
const QString kAutoSource = QStringLiteral("auto");

/**
 * 读取规范语言键到腾讯云语言代码的映射表。
 * @return 映射表，缺席的语言表示腾讯云机器翻译不支持。
 */
const QHash<QString, QString> &tencentLanguageCodes()
{
    static const QHash<QString, QString> codes = {
        {QStringLiteral("zh-Hans"), QStringLiteral("zh")},
        {QStringLiteral("zh-Hant"), QStringLiteral("zh-TW")},
        {QStringLiteral("en"), QStringLiteral("en")},
        {QStringLiteral("ja"), QStringLiteral("ja")},
        {QStringLiteral("ko"), QStringLiteral("ko")},
        {QStringLiteral("fr"), QStringLiteral("fr")},
        {QStringLiteral("de"), QStringLiteral("de")},
        {QStringLiteral("ru"), QStringLiteral("ru")},
        {QStringLiteral("es"), QStringLiteral("es")},
        {QStringLiteral("pt"), QStringLiteral("pt")},
        {QStringLiteral("it"), QStringLiteral("it")},
        {QStringLiteral("ar"), QStringLiteral("ar")},
        {QStringLiteral("hi"), QStringLiteral("hi")},
        {QStringLiteral("th"), QStringLiteral("th")},
        {QStringLiteral("vi"), QStringLiteral("vi")},
        {QStringLiteral("id"), QStringLiteral("id")},
        {QStringLiteral("ms"), QStringLiteral("ms")},
        {QStringLiteral("tr"), QStringLiteral("tr")},
    };
    return codes;
}

/**
 * 读取响应体中的 Response 对象。
 * @param body HTTP 响应体。
 * @return Response 对象，响应体不是 JSON 对象时为空对象。
 */
QJsonObject responseObject(const QByteArray &body)
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject()) {
        return {};
    }
    return document.object().value(QStringLiteral("Response")).toObject();
}

}  // namespace

QString tencentLanguageCode(const QString &canonicalKey)
{
    return tencentLanguageCodes().value(canonicalKey);
}

TencentEndpoint resolveTencentEndpoint(const QString &endpoint)
{
    QString authority = endpoint.trimmed();

    // 1. 未显式给出协议时按腾讯云默认走 https，本地联调可写 http 前缀
    QString scheme = QStringLiteral("https");
    if (authority.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)) {
        scheme = QStringLiteral("http");
        authority = authority.mid(7);
    } else if (authority.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        authority = authority.mid(8);
    }

    // 2. 请求路径固定为单个斜杠，配置里多写的路径一律丢弃以免与签名不一致
    const int pathStart = authority.indexOf(QLatin1Char('/'));
    if (pathStart >= 0) {
        authority = authority.left(pathStart);
    }

    TencentEndpoint resolved;
    resolved.host = authority;
    resolved.url = QUrl(QStringLiteral("%1://%2/").arg(scheme, authority));
    return resolved;
}

QByteArray buildBatchTranslatePayload(const QStringList &texts, const QString &targetCode, int projectId)
{
    QJsonArray sourceTextList;
    for (const QString &text : texts) {
        sourceTextList.append(text);
    }

    const QJsonObject payload{{QStringLiteral("SourceTextList"), sourceTextList},
                              {QStringLiteral("Source"), kAutoSource},
                              {QStringLiteral("Target"), targetCode},
                              {QStringLiteral("ProjectId"), projectId}};
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QByteArray buildTextTranslatePayload(const QString &text, const QString &targetCode, int projectId)
{
    const QJsonObject payload{{QStringLiteral("SourceText"), text},
                              {QStringLiteral("Source"), kAutoSource},
                              {QStringLiteral("Target"), targetCode},
                              {QStringLiteral("ProjectId"), projectId}};
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

bool parseTencentApiError(const QByteArray &body, TencentApiError *apiError)
{
    const QJsonObject response = responseObject(body);
    const QJsonValue errorValue = response.value(QStringLiteral("Error"));
    if (!errorValue.isObject()) {
        return false;
    }
    if (apiError) {
        const QJsonObject errorObject = errorValue.toObject();
        apiError->code = errorObject.value(QStringLiteral("Code")).toString();
        apiError->message = errorObject.value(QStringLiteral("Message")).toString();
        apiError->requestId = response.value(QStringLiteral("RequestId")).toString();
    }
    return true;
}

QString formatTencentApiError(const TencentApiError &apiError)
{
    return QStringLiteral("tencent tmt error %1: %2 (RequestId: %3)")
        .arg(apiError.code.isEmpty() ? QStringLiteral("unknown") : apiError.code,
             apiError.message,
             apiError.requestId);
}

bool isTencentActionUnavailable(const QString &code)
{
    // 1. 接口下线与接口不存在都说明批量入口不可用，需要降级到单段翻译
    return code.startsWith(QLatin1String("ActionOffline"), Qt::CaseInsensitive)
        || code.startsWith(QLatin1String("InvalidAction"), Qt::CaseInsensitive);
}

bool parseBatchTranslateResponse(const QByteArray &body, QStringList *texts, QString *error)
{
    if (!texts) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    texts->clear();

    const QJsonValue listValue = responseObject(body).value(QStringLiteral("TargetTextList"));
    if (!listValue.isArray()) {
        if (error) {
            *error = QStringLiteral("tencent tmt response missing TargetTextList");
        }
        return false;
    }
    const QJsonArray list = listValue.toArray();
    for (const QJsonValue &item : list) {
        texts->append(item.toString());
    }
    return true;
}

bool parseTextTranslateResponse(const QByteArray &body, QString *text, QString *error)
{
    if (!text) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    text->clear();

    const QJsonObject response = responseObject(body);
    if (!response.contains(QStringLiteral("TargetText"))) {
        if (error) {
            *error = QStringLiteral("tencent tmt response missing TargetText");
        }
        return false;
    }
    *text = response.value(QStringLiteral("TargetText")).toString();
    return true;
}

}  // namespace markshot::translate_tencent

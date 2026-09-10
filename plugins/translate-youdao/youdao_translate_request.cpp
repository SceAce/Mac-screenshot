#include "youdao_translate_request.h"

#include "youdao_translate_signer.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>

namespace markshot::translate_youdao {
namespace {

/**
 * 读取规范语言键到有道语言代码的映射表。
 * @return 映射表，键为 translate-common 的规范语言键。
 */
const QHash<QString, QString> &languageCodes()
{
    static const QHash<QString, QString> codes = {
        {QStringLiteral("zh-Hans"), QStringLiteral("zh-CHS")},
        {QStringLiteral("zh-Hant"), QStringLiteral("zh-CHT")},
        {QStringLiteral("yue"), QStringLiteral("yue")},
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
        {QStringLiteral("nl"), QStringLiteral("nl")},
        {QStringLiteral("pl"), QStringLiteral("pl")},
    };
    return codes;
}

/**
 * 读取有道错误码的文字说明。
 * @return 错误码到英文说明的映射表。
 */
const QHash<QString, QString> &errorDescriptions()
{
    static const QHash<QString, QString> descriptions = {
        {QStringLiteral("101"), QStringLiteral("missing required parameter")},
        {QStringLiteral("102"), QStringLiteral("unsupported language pair")},
        {QStringLiteral("103"), QStringLiteral("query text is too long")},
        {QStringLiteral("105"), QStringLiteral("unsupported signature type")},
        {QStringLiteral("108"), QStringLiteral("invalid application id")},
        {QStringLiteral("111"), QStringLiteral("invalid developer account")},
        {QStringLiteral("113"), QStringLiteral("query text is empty")},
        {QStringLiteral("202"), QStringLiteral("signature check failed")},
        {QStringLiteral("203"), QStringLiteral("client address is not in the allow list")},
        {QStringLiteral("206"), QStringLiteral("invalid timestamp")},
        {QStringLiteral("207"), QStringLiteral("replayed request")},
        {QStringLiteral("401"), QStringLiteral("account is in arrears")},
        {QStringLiteral("411"), QStringLiteral("request rate limited")},
        {QStringLiteral("412"), QStringLiteral("too many long requests")},
    };
    return descriptions;
}

/**
 * 读取易混淆错误码的针对性排查提示。
 * @param errorCode 有道错误码。
 * @return 提示文本，无针对性提示时返回空串。
 */
QString errorHint(const QString &errorCode)
{
    if (errorCode == QStringLiteral("202")) {
        return QStringLiteral("appKey and appSecret look correct only if the query text is UTF-8 encoded");
    }
    if (errorCode == QStringLiteral("206")) {
        return QStringLiteral("curtime must be the current UTC timestamp in seconds, check the system clock");
    }
    if (errorCode == QStringLiteral("207")) {
        return QStringLiteral("salt and curtime must be regenerated for every request");
    }
    return {};
}

/**
 * 组装业务错误信息。
 * @param errorCode 有道错误码。
 * @param requestId 有道请求标识，便于向服务方追查。
 * @return 面向用户的错误信息。
 */
QString describeError(const QString &errorCode, const QString &requestId)
{
    QString message = QStringLiteral("youdao error %1")
                          .arg(errorCode.isEmpty() ? QStringLiteral("unknown") : errorCode);
    const QString description = errorDescriptions().value(errorCode);
    if (!description.isEmpty()) {
        message += QStringLiteral(": ") + description;
    }
    const QString hint = errorHint(errorCode);
    if (!hint.isEmpty()) {
        message += QStringLiteral(" (") + hint + QStringLiteral(")");
    }
    if (!requestId.isEmpty()) {
        message += QStringLiteral(" [requestId %1]").arg(requestId);
    }
    return message;
}

/**
 * 读取响应中的 errorCode。
 * @param object 响应 JSON 对象。
 * @return 错误码字符串，缺失时返回空串。
 */
QString readErrorCode(const QJsonObject &object)
{
    const QJsonValue value = object.value(QStringLiteral("errorCode"));

    // 1. 官方协议里 errorCode 是字符串，数字形式仅作兼容处理
    if (value.isDouble()) {
        return QString::number(static_cast<qint64>(value.toDouble()));
    }
    return value.toString();
}

/**
 * 向表单请求体追加一个字段。
 * @param body 请求体缓冲区。
 * @param key 字段名。
 * @param value 字段值，按 UTF-8 百分号编码后写入。
 * @return 无返回值。
 */
void appendFormField(QByteArray *body, const char *key, const QString &value)
{
    if (!body->isEmpty()) {
        body->append('&');
    }
    body->append(key);
    body->append('=');
    body->append(QUrl::toPercentEncoding(value));
}

}  // namespace

QString youdaoLanguageCode(const QString &canonicalKey)
{
    return languageCodes().value(canonicalKey);
}

QByteArray buildYoudaoFormBody(const YoudaoFormRequest &request)
{
    // 1. 签名 input 取全部 q 字段的无分隔符连接串
    const QString signInput = youdaoSignInput(request.queries);
    const QString sign = youdaoSignature(request.appKey,
                                         signInput,
                                         request.salt,
                                         request.curtime,
                                         request.appSecret);

    // 2. 批量文本以多个同名 q 字段承载，译文数组按此顺序一一对应
    QByteArray body;
    for (const QString &query : request.queries) {
        appendFormField(&body, "q", query);
    }

    // 3. 其余鉴权与语言参数，signType 固定为 v3
    appendFormField(&body, "from", request.from);
    appendFormField(&body, "to", request.to);
    appendFormField(&body, "appKey", request.appKey);
    appendFormField(&body, "salt", request.salt);
    appendFormField(&body, "sign", sign);
    appendFormField(&body, "signType", QStringLiteral("v3"));
    appendFormField(&body, "curtime", request.curtime);
    return body;
}

bool parseYoudaoTranslations(const QByteArray &body, QStringList *translations, QString *error)
{
    if (!translations) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    translations->clear();

    // 1. 响应必须是 JSON 对象，否则多半是网关返回的错误页
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("youdao response is not valid JSON: %1")
                         .arg(QString::fromUtf8(body.left(200)));
        }
        return false;
    }
    const QJsonObject object = document.object();

    // 2. errorCode 非 "0" 一律视为业务失败，错误信息带上错误码与排查提示
    const QString errorCode = readErrorCode(object);
    if (errorCode != QStringLiteral("0")) {
        if (error) {
            *error = describeError(errorCode, object.value(QStringLiteral("requestId")).toString());
        }
        return false;
    }

    // 3. 小语种场景下 query 字段可能缺省，成功判定只看 translation 数组
    const QJsonArray array = object.value(QStringLiteral("translation")).toArray();
    if (array.isEmpty()) {
        if (error) {
            *error = QStringLiteral("youdao response missing translation array");
        }
        return false;
    }
    translations->reserve(array.size());
    for (const QJsonValue &item : array) {
        translations->append(item.toString());
    }
    return true;
}

}  // namespace markshot::translate_youdao

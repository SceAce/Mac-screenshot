#include "baidu_translate_request.h"

#include "translate_language_code.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>

namespace markshot::translate_baidu {
namespace {

/**
 * 读取规范语言键到百度语言代码的映射表。
 * @return 语言代码映射表。
 */
const QHash<QString, QString> &languageCodes()
{
    static const QHash<QString, QString> codes = {
        {QStringLiteral("zh-Hans"), QStringLiteral("zh")},
        {QStringLiteral("zh-Hant"), QStringLiteral("cht")},
        {QStringLiteral("yue"), QStringLiteral("yue")},
        {QStringLiteral("en"), QStringLiteral("en")},
        {QStringLiteral("ja"), QStringLiteral("jp")},
        {QStringLiteral("ko"), QStringLiteral("kor")},
        {QStringLiteral("fr"), QStringLiteral("fra")},
        {QStringLiteral("de"), QStringLiteral("de")},
        {QStringLiteral("ru"), QStringLiteral("ru")},
        {QStringLiteral("es"), QStringLiteral("spa")},
        {QStringLiteral("pt"), QStringLiteral("pt")},
        {QStringLiteral("it"), QStringLiteral("it")},
        {QStringLiteral("ar"), QStringLiteral("ara")},
        {QStringLiteral("hi"), QStringLiteral("hi")},
        {QStringLiteral("th"), QStringLiteral("th")},
        {QStringLiteral("vi"), QStringLiteral("vie")},
        {QStringLiteral("id"), QStringLiteral("id")},
        {QStringLiteral("ms"), QStringLiteral("may")},
        {QStringLiteral("tr"), QStringLiteral("tr")},
        {QStringLiteral("nl"), QStringLiteral("nl")},
        {QStringLiteral("pl"), QStringLiteral("pl")},
    };
    return codes;
}

/**
 * 读取常见错误码的中文说明。
 * @param code 平台返回的错误码。
 * @return 错误码说明，未收录时返回空串。
 */
QString errorCodeDescription(const QString &code)
{
    static const QHash<QString, QString> descriptions = {
        {QStringLiteral("52001"), QStringLiteral("请求超时，可减少单次文本量后重试")},
        {QStringLiteral("52002"), QStringLiteral("系统错误")},
        {QStringLiteral("52003"), QStringLiteral("未授权用户，确认 appId 与服务开通状态")},
        {QStringLiteral("54000"), QStringLiteral("必填参数为空")},
        {QStringLiteral("54001"), QStringLiteral("签名错误")},
        {QStringLiteral("54003"), QStringLiteral("访问频率受限")},
        {QStringLiteral("54004"), QStringLiteral("账户余额不足")},
        {QStringLiteral("54005"), QStringLiteral("长 query 请求频繁")},
        {QStringLiteral("58000"), QStringLiteral("客户端 IP 非法")},
        {QStringLiteral("58001"), QStringLiteral("译文语言方向不支持")},
        {QStringLiteral("58002"), QStringLiteral("服务已关闭")},
        {QStringLiteral("90107"), QStringLiteral("认证未通过或未生效")},
    };
    return descriptions.value(code);
}

/**
 * 向请求体追加一个表单字段。
 * @param body 请求体缓冲区。
 * @param name 字段名。
 * @param value 未编码的字段值。
 * @return 无返回值。
 */
void appendFormField(QByteArray *body, const char *name, const QString &value)
{
    if (!body->isEmpty()) {
        body->append('&');
    }
    body->append(name);
    body->append('=');

    // 1. 手工百分号编码，避免 QUrlQuery 在不同调用路径下漏编码或重复编码
    QByteArray encoded = QUrl::toPercentEncoding(value);

    // 2. 表单规则用加号表示空格，原文里的百分号此时已编码成 %25，替换不会误伤
    encoded.replace("%20", "+");
    body->append(encoded);
}

/**
 * 读取 JSON 字段的字符串取值。
 *
 * 文档把 error_code 标为整数，服务端实际返回带引号的字符串，两种形态都要能解析。
 *
 * @param object JSON 对象。
 * @param key 字段名。
 * @return 去除首尾空白后的字符串，字段缺失时为空串。
 */
QString jsonFieldAsString(const QJsonObject &object, const QString &key)
{
    return object.value(key).toVariant().toString().trimmed();
}

}  // namespace

QString baiduLanguageCode(const QString &canonicalKey)
{
    return languageCodes().value(canonicalKey);
}

bool resolveBaiduTargetLanguage(const QString &targetLanguage, QString *languageCode, QString *error)
{
    if (!languageCode) {
        if (error) {
            *error = QStringLiteral("language output target is missing");
        }
        return false;
    }

    // 1. 先把自由文本归一化为规范键，识别失败说明目标语言本身写错了
    const QString canonicalKey = markshot::translate_common::canonicalLanguageKey(targetLanguage);
    if (canonicalKey.isEmpty()) {
        if (error) {
            *error = QStringLiteral("unrecognized target language: %1").arg(targetLanguage.trimmed());
        }
        return false;
    }

    // 2. 再映射为百度语言代码，映射缺失说明该语言不在插件支持范围内
    const QString code = baiduLanguageCode(canonicalKey);
    if (code.isEmpty()) {
        if (error) {
            *error = QStringLiteral("baidu translate does not support target language: %1")
                         .arg(markshot::translate_common::canonicalLanguageDisplayName(canonicalKey));
        }
        return false;
    }
    *languageCode = code;
    return true;
}

QString joinBaiduQuery(const QVector<markshot::plugin::TranslateSegment> &segments)
{
    QStringList lines;
    lines.reserve(segments.size());
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        lines.append(segment.text);
    }

    // 1. 平台按换行切分多段文本，返回的 trans_result 与切分结果一一对应
    return lines.join(QLatin1Char('\n'));
}

QByteArray buildBaiduFormBody(const BaiduTranslateForm &form)
{
    QByteArray body;

    // 1. 字段顺序不影响签名校验，此处按官方示例排列便于比对抓包
    appendFormField(&body, "q", form.query);
    appendFormField(&body, "from", form.from);
    appendFormField(&body, "to", form.to);
    appendFormField(&body, "appid", form.appId);
    appendFormField(&body, "salt", form.salt);
    appendFormField(&body, "sign", form.sign);
    return body;
}

bool parseBaiduTranslateResponse(const QByteArray &body, QStringList *translations, QString *error)
{
    if (!translations) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    translations->clear();

    // 1. 网关异常时可能返回 HTML 或空体，先确认响应是 JSON 对象
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (!document.isObject()) {
        if (error) {
            *error = QStringLiteral("baidu response is not valid JSON: %1")
                         .arg(QString::fromUtf8(body.left(200)));
        }
        return false;
    }
    const QJsonObject object = document.object();

    // 2. trans_result 非空是判定成功的可靠条件，部分失败响应同样不带 error_code
    const QJsonArray results = object.value(QStringLiteral("trans_result")).toArray();
    if (!results.isEmpty()) {
        for (const QJsonValue &item : results) {
            translations->append(item.toObject().value(QStringLiteral("dst")).toString());
        }
        return true;
    }

    // 3. 失败时把错误码与错误描述一并带出，签名错误再补一句排查方向
    const QString code = jsonFieldAsString(object, QStringLiteral("error_code"));
    const QString message = jsonFieldAsString(object, QStringLiteral("error_msg"));
    if (!code.isEmpty()) {
        QString text = QStringLiteral("baidu translate error %1").arg(code);
        if (!message.isEmpty()) {
            text += QStringLiteral(": %1").arg(message);
        }
        const QString description = errorCodeDescription(code);
        if (!description.isEmpty()) {
            text += QStringLiteral(" (%1)").arg(description);
        }
        if (code == QStringLiteral("54001")) {
            text += QStringLiteral("；检查 appId/appKey 与签名拼接顺序");
        }
        if (error) {
            *error = text;
        }
        return false;
    }

    if (error) {
        *error = QStringLiteral("baidu response missing trans_result: %1")
                     .arg(QString::fromUtf8(body.left(200)));
    }
    return false;
}

}  // namespace markshot::translate_baidu

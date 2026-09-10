#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace markshot::translate_youdao {

/**
 * 有道翻译表单请求参数。
 *
 * queries 的顺序即表单中 q 字段的顺序，也决定响应 translation 数组的对应关系。
 */
struct YoudaoFormRequest {
    QString appKey;
    QString appSecret;
    QString from = QStringLiteral("auto");
    QString to;
    QStringList queries;
    QString salt;
    QString curtime;
};

/**
 * 把规范语言键映射为有道语言代码。
 *
 * 中文使用 zh-CHS 与 zh-CHT 而非 zh，其余语言与规范键同名。
 *
 * @param canonicalKey translate-common 归一化后的规范语言键。
 * @return 有道语言代码，不支持该语言时返回空串。
 */
QString youdaoLanguageCode(const QString &canonicalKey);

/**
 * 构造 application/x-www-form-urlencoded 请求体。
 *
 * 批量文本以多个同名 q 字段发送，签名由 appKey、全部 q 的连接串、salt、curtime
 * 与 appSecret 计算得出。
 *
 * @param request 表单请求参数。
 * @return 已按 UTF-8 百分号编码的请求体。
 */
QByteArray buildYoudaoFormBody(const YoudaoFormRequest &request);

/**
 * 解析有道翻译响应体。
 *
 * errorCode 为字符串类型，仅当其为 "0" 且 translation 非空时视为成功。
 *
 * @param body HTTP 响应体。
 * @param translations 输出译文列表，顺序与请求中的 q 字段一致。
 * @param error 输出错误信息，包含 errorCode 与常见错误码的排查提示。
 * @return 解析成功时返回 true。
 */
bool parseYoudaoTranslations(const QByteArray &body, QStringList *translations, QString *error);

}  // namespace markshot::translate_youdao

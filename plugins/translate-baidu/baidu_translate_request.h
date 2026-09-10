#pragma once

#include "markshot/translate_provider_plugin.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace markshot::translate_baidu {

/**
 * 百度翻译请求的表单参数。
 *
 * query 存放未编码的原文，sign 由调用方按同一份 query 计算后填入。
 */
struct BaiduTranslateForm {
    QString appId;
    QString query;
    QString from = QStringLiteral("auto");
    QString to;
    QString salt;
    QString sign;
};

/**
 * 把规范语言键映射为百度翻译语言代码。
 *
 * 百度的代码与 ISO 标记差异较大，例如日语为 jp、韩语为 kor、西班牙语为 spa。
 *
 * @param canonicalKey 规范语言键，例如 zh-Hans、ja。
 * @return 百度语言代码，未覆盖的语言返回空串。
 */
QString baiduLanguageCode(const QString &canonicalKey);

/**
 * 把用户配置的目标语言解析为百度语言代码。
 * @param targetLanguage 目标语言名称，允许英文名、中文名或 BCP-47 标记。
 * @param languageCode 输出百度语言代码。
 * @param error 输出错误信息。
 * @return 解析成功时返回 true。
 */
bool resolveBaiduTargetLanguage(const QString &targetLanguage, QString *languageCode, QString *error);

/**
 * 把多段文本连接成单个 q 参数。
 *
 * 分段必须已经过压平处理，段内换行会让译文与分段错位。
 *
 * @param segments 待翻译分段。
 * @return 用换行连接后的待翻译文本。
 */
QString joinBaiduQuery(const QVector<markshot::plugin::TranslateSegment> &segments);

/**
 * 构造 application/x-www-form-urlencoded 请求体。
 * @param form 表单参数，其中 query 为未编码原文。
 * @return 已按表单规则百分号编码的请求体。
 */
QByteArray buildBaiduFormBody(const BaiduTranslateForm &form);

/**
 * 解析百度翻译响应体。
 *
 * 判定成功的可靠条件是 trans_result 存在且非空，而不是 error_code 缺失。
 *
 * @param body HTTP 响应体。
 * @param translations 输出按段顺序排列的译文列表。
 * @param error 输出错误信息，包含 error_code 与 error_msg。
 * @return 取得译文时返回 true。
 */
bool parseBaiduTranslateResponse(const QByteArray &body, QStringList *translations, QString *error);

}  // namespace markshot::translate_baidu

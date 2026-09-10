#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace markshot::translate_tencent {

/**
 * 单批请求的字符预算。
 *
 * 接口参数说明写 6000 字符，请求限制页写 2000 字符，取保守值分批。
 */
inline constexpr int kTencentMaxBatchChars = 2000;

/** 接口版本号，出现在 X-TC-Version 请求头。 */
inline constexpr char kTencentApiVersion[] = "2018-03-21";

/** 批量翻译 Action 名称。 */
inline constexpr char kTencentBatchAction[] = "TextTranslateBatch";

/** 单段翻译 Action 名称，作为批量接口下线时的降级路径。 */
inline constexpr char kTencentTextAction[] = "TextTranslate";

/**
 * 解析后的接入点。
 *
 * host 必须与实际发送的 Host 请求头逐字节相同，否则签名校验失败。
 */
struct TencentEndpoint {
    QUrl url;
    QString host;
};

/**
 * 腾讯云返回的业务错误。
 */
struct TencentApiError {
    QString code;
    QString message;
    QString requestId;
};

/**
 * 把规范语言键映射为腾讯云语言代码。
 * @param canonicalKey 规范语言键，例如 zh-Hans。
 * @return 腾讯云语言代码，不支持时返回空串。
 */
QString tencentLanguageCode(const QString &canonicalKey);

/**
 * 解析接入点配置。
 *
 * 未显式给出协议时按腾讯云默认走 https，允许配置里带端口以便本地联调。
 *
 * @param endpoint 配置中的接入点。
 * @return 请求地址与签名用的主机名。
 */
TencentEndpoint resolveTencentEndpoint(const QString &endpoint);

/**
 * 构造 TextTranslateBatch 请求体。
 * @param texts 源文列表。
 * @param targetCode 腾讯云目标语言代码。
 * @param projectId 项目 ID。
 * @return 紧凑格式的 JSON 请求体。
 */
QByteArray buildBatchTranslatePayload(const QStringList &texts, const QString &targetCode, int projectId);

/**
 * 构造 TextTranslate 请求体。
 * @param text 源文。
 * @param targetCode 腾讯云目标语言代码。
 * @param projectId 项目 ID。
 * @return 紧凑格式的 JSON 请求体。
 */
QByteArray buildTextTranslatePayload(const QString &text, const QString &targetCode, int projectId);

/**
 * 解析响应体中的业务错误。
 *
 * 腾讯云在 HTTP 200 下也可能返回 Response.Error，只要该字段存在即视为失败。
 *
 * @param body HTTP 响应体。
 * @param apiError 输出错误码、描述与请求 ID。
 * @return 响应体中存在业务错误时返回 true。
 */
bool parseTencentApiError(const QByteArray &body, TencentApiError *apiError);

/**
 * 拼接业务错误的可读描述。
 * @param apiError 业务错误。
 * @return 含错误码、描述与请求 ID 的错误信息。
 */
QString formatTencentApiError(const TencentApiError &apiError);

/**
 * 判断错误码是否表示接口已不可调用。
 * @param code 业务错误码。
 * @return 需要降级为单段翻译时返回 true。
 */
bool isTencentActionUnavailable(const QString &code);

/**
 * 解析 TextTranslateBatch 响应。
 * @param body HTTP 响应体。
 * @param texts 输出译文列表。
 * @param error 输出错误信息。
 * @return 解析成功时返回 true。
 */
bool parseBatchTranslateResponse(const QByteArray &body, QStringList *texts, QString *error);

/**
 * 解析 TextTranslate 响应。
 * @param body HTTP 响应体。
 * @param text 输出译文。
 * @param error 输出错误信息。
 * @return 解析成功时返回 true。
 */
bool parseTextTranslateResponse(const QByteArray &body, QString *text, QString *error);

}  // namespace markshot::translate_tencent

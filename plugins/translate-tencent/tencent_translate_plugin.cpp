#include "tencent_translate_plugin.h"

#include "tencent_tc3_signer.h"
#include "tencent_translate_config.h"
#include "tencent_translate_request.h"
#include "translate_http_client.h"
#include "translate_language_code.h"
#include "translate_segment_batch.h"

#include <QDateTime>

namespace markshot::translate_tencent {
namespace {

using markshot::plugin::TranslateSegment;
using markshot::translate_common::canonicalLanguageKey;
using markshot::translate_common::chunkTranslateSegments;
using markshot::translate_common::flattenSegmentText;
using markshot::translate_common::sendTranslateHttpRequest;
using markshot::translate_common::TranslateHttpRequest;
using markshot::translate_common::TranslateHttpResponse;

/**
 * 把目标语言解析为腾讯云语言代码。
 * @param targetLanguage 目标语言名称，允许英文名、中文名或 BCP-47 标记。
 * @param targetCode 输出腾讯云语言代码。
 * @param error 输出错误信息。
 * @return 解析成功时返回 true。
 */
bool resolveTargetCode(const QString &targetLanguage, QString *targetCode, QString *error)
{
    // 1. 先归一化成规范键，兼容配置里写英文名或中文名的场景
    const QString canonicalKey = canonicalLanguageKey(targetLanguage);
    if (canonicalKey.isEmpty()) {
        if (error) {
            *error = QStringLiteral("unrecognized target language: %1").arg(targetLanguage.trimmed());
        }
        return false;
    }

    // 2. 规范键不在腾讯云语言表内时直接失败，不做近似替换
    const QString code = tencentLanguageCode(canonicalKey);
    if (code.isEmpty()) {
        if (error) {
            *error = QStringLiteral("tencent tmt does not support target language %1")
                         .arg(markshot::translate_common::canonicalLanguageDisplayName(canonicalKey));
        }
        return false;
    }
    if (targetCode) {
        *targetCode = code;
    }
    return true;
}

/**
 * 发送一次已签名的腾讯云 API 请求。
 * @param config 插件配置。
 * @param action 接口名称，填入 X-TC-Action 请求头。
 * @param payload JSON 请求体。
 * @param body 输出 HTTP 响应体。
 * @param error 输出错误信息。
 * @return 取得响应时返回 true。
 */
bool postTencentRequest(const TencentTranslateConfig &config,
                        const QByteArray &action,
                        const QByteArray &payload,
                        QByteArray *body,
                        QString *error)
{
    // 1. 解析接入点，签名用的主机名必须与实际发送的 Host 头一致
    const TencentEndpoint endpoint = resolveTencentEndpoint(config.endpoint);

    // 2. 时间戳同时用于签名与 X-TC-Timestamp，两者必须取同一个值
    Tc3SigningInput signingInput;
    signingInput.host = endpoint.host;
    signingInput.payload = payload;
    signingInput.secretId = config.secretId;
    signingInput.secretKey = config.secretKey;
    signingInput.timestamp = QDateTime::currentSecsSinceEpoch();

    // 3. Content-Type 取签名输入里的字面量，避免两处写法出现细微差异
    TranslateHttpRequest request;
    request.url = endpoint.url;
    request.contentType = signingInput.contentType;
    request.body = payload;
    request.timeoutMs = config.timeoutMs;
    request.headers = {
        {QByteArrayLiteral("Host"), endpoint.host.toUtf8()},
        {QByteArrayLiteral("Authorization"), tc3AuthorizationHeader(signingInput)},
        {QByteArrayLiteral("X-TC-Action"), action},
        {QByteArrayLiteral("X-TC-Version"), QByteArray(kTencentApiVersion)},
        {QByteArrayLiteral("X-TC-Timestamp"), QByteArray::number(signingInput.timestamp)},
        {QByteArrayLiteral("X-TC-Region"), config.region.toUtf8()},
    };

    TranslateHttpResponse response;
    if (!sendTranslateHttpRequest(request, &response, error)) {
        return false;
    }
    if (body) {
        *body = response.body;
    }
    return true;
}

/**
 * 逐段调用 TextTranslate 翻译一个批次。
 * @param config 插件配置。
 * @param targetCode 腾讯云目标语言代码。
 * @param batch 当前批次分段。
 * @param translatedTexts 按顺序追加译文。
 * @param error 输出错误信息。
 * @return 全部分段翻译成功时返回 true。
 */
bool translateSegmentsIndividually(const TencentTranslateConfig &config,
                                   const QString &targetCode,
                                   const QVector<TranslateSegment> &batch,
                                   QStringList *translatedTexts,
                                   QString *error)
{
    for (const TranslateSegment &segment : batch) {
        QByteArray body;
        const QByteArray payload =
            buildTextTranslatePayload(segment.text, targetCode, config.projectId);
        if (!postTencentRequest(config, QByteArray(kTencentTextAction), payload, &body, error)) {
            return false;
        }

        // 1. 单段接口没有可降级的下家，出现业务错误即整体失败
        TencentApiError apiError;
        if (parseTencentApiError(body, &apiError)) {
            if (error) {
                *error = formatTencentApiError(apiError);
            }
            return false;
        }

        QString text;
        if (!parseTextTranslateResponse(body, &text, error)) {
            return false;
        }
        translatedTexts->append(text);
    }
    return true;
}

/**
 * 翻译一个批次，必要时降级为逐段调用。
 * @param config 插件配置。
 * @param targetCode 腾讯云目标语言代码。
 * @param batch 当前批次分段。
 * @param useTextFallback 降级开关，一旦置位后续批次不再尝试批量接口。
 * @param translatedTexts 按顺序追加译文。
 * @param error 输出错误信息。
 * @return 批次翻译成功时返回 true。
 */
bool translateBatch(const TencentTranslateConfig &config,
                    const QString &targetCode,
                    const QVector<TranslateSegment> &batch,
                    bool *useTextFallback,
                    QStringList *translatedTexts,
                    QString *error)
{
    // 1. 已经降级过就不再重复探测批量接口，省掉一次注定失败的请求
    if (*useTextFallback) {
        return translateSegmentsIndividually(config, targetCode, batch, translatedTexts, error);
    }

    QStringList sourceTexts;
    sourceTexts.reserve(batch.size());
    for (const TranslateSegment &segment : batch) {
        sourceTexts.append(segment.text);
    }

    QByteArray body;
    const QByteArray payload = buildBatchTranslatePayload(sourceTexts, targetCode, config.projectId);
    if (!postTencentRequest(config, QByteArray(kTencentBatchAction), payload, &body, error)) {
        return false;
    }

    // 2. 批量接口已从文档下线，服务端返回下线或未知接口时改走单段接口
    TencentApiError apiError;
    if (parseTencentApiError(body, &apiError)) {
        if (isTencentActionUnavailable(apiError.code)) {
            *useTextFallback = true;
            return translateSegmentsIndividually(config, targetCode, batch, translatedTexts, error);
        }
        if (error) {
            *error = formatTencentApiError(apiError);
        }
        return false;
    }

    QStringList texts;
    if (!parseBatchTranslateResponse(body, &texts, error)) {
        return false;
    }

    // 3. 条数不符时拒绝回填，错位的译文比翻译失败更难被发现
    if (texts.size() != batch.size()) {
        if (error) {
            *error = QStringLiteral("tencent tmt returned %1 translations for %2 source texts")
                         .arg(texts.size())
                         .arg(batch.size());
        }
        return false;
    }
    translatedTexts->append(texts);
    return true;
}

}  // namespace

QString TencentTranslatePlugin::providerId() const
{
    return QStringLiteral("tencent-tmt");
}

QString TencentTranslatePlugin::displayName() const
{
    return QStringLiteral("Tencent Machine Translation");
}

bool TencentTranslatePlugin::isAvailable(QString *error) const
{
    return validateTencentTranslateConfig(readTencentTranslateConfig(), error);
}

bool TencentTranslatePlugin::translate(const QVector<TranslateSegment> &segments,
                                       const QString &targetLanguage,
                                       QVector<TranslateSegment> *translations,
                                       QString *error)
{
    // 1. 输出容器缺失时直接失败，避免调用方拿到未初始化的数据
    if (!translations) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    translations->clear();

    // 2. 没有分段时视为成功，省掉一次无意义的网络往返
    if (segments.isEmpty()) {
        return true;
    }

    // 3. 读取并校验凭据与接入点
    const TencentTranslateConfig config = readTencentTranslateConfig();
    if (!validateTencentTranslateConfig(config, error)) {
        return false;
    }

    // 4. 目标语言先归一化再映射到腾讯云语言代码
    QString targetCode;
    if (!resolveTargetCode(targetLanguage, &targetCode, error)) {
        return false;
    }

    // 5. 段内换行会让批量译文与分段错位，发送前统一压平
    QVector<TranslateSegment> flattened;
    flattened.reserve(segments.size());
    for (const TranslateSegment &segment : segments) {
        flattened.append({segment.id, flattenSegmentText(segment.text)});
    }

    // 6. 按字符预算分批，批次顺序与分段顺序一致因而译文可顺序累积
    const QVector<QVector<TranslateSegment>> batches =
        chunkTranslateSegments(flattened, kTencentMaxBatchChars);
    QStringList translatedTexts;
    translatedTexts.reserve(segments.size());
    bool useTextFallback = false;
    for (const QVector<TranslateSegment> &batch : batches) {
        if (!translateBatch(config, targetCode, batch, &useTextFallback, &translatedTexts, error)) {
            return false;
        }
    }

    // 7. 总条数不符时同样拒绝回填
    if (translatedTexts.size() != segments.size()) {
        if (error) {
            *error = QStringLiteral("tencent tmt returned %1 translations for %2 segments")
                         .arg(translatedTexts.size())
                         .arg(segments.size());
        }
        return false;
    }

    // 8. 按输入顺序输出，译文为空的段保留原文
    for (int index = 0; index < segments.size(); ++index) {
        const QString text = translatedTexts.at(index).trimmed();
        translations->append({segments.at(index).id, text.isEmpty() ? segments.at(index).text : text});
    }
    return true;
}

}  // namespace markshot::translate_tencent

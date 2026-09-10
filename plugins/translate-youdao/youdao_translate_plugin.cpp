#include "youdao_translate_plugin.h"

#include "translate_http_client.h"
#include "translate_language_code.h"
#include "translate_segment_batch.h"
#include "youdao_translate_config.h"
#include "youdao_translate_request.h"

#include <QDateTime>
#include <QStringList>
#include <QUrl>
#include <QUuid>

namespace markshot::translate_youdao {
namespace {

// 有道单次查询上限为 5000 字符，留出编码余量后按 4000 字符分批
constexpr int kMaxBatchChars = 4000;

/**
 * 把目标语言解析为有道语言代码。
 * @param targetLanguage 用户配置的目标语言名称。
 * @param code 输出有道语言代码。
 * @param error 输出错误信息。
 * @return 解析成功时返回 true。
 */
bool resolveTargetLanguage(const QString &targetLanguage, QString *code, QString *error)
{
    // 1. 先把英文名、中文名或 BCP-47 标记归一化为规范语言键
    const QString canonicalKey = markshot::translate_common::canonicalLanguageKey(targetLanguage);
    if (canonicalKey.isEmpty()) {
        if (error) {
            *error = QStringLiteral("unsupported target language: %1").arg(targetLanguage.trimmed());
        }
        return false;
    }

    // 2. 再映射为有道语言代码，中文需区分 zh-CHS 与 zh-CHT
    const QString languageCode = youdaoLanguageCode(canonicalKey);
    if (languageCode.isEmpty()) {
        if (error) {
            *error = QStringLiteral("youdao does not support target language: %1")
                         .arg(markshot::translate_common::canonicalLanguageDisplayName(canonicalKey));
        }
        return false;
    }
    if (code) {
        *code = languageCode;
    }
    return true;
}

/**
 * 请求单个批次并按顺序追加译文。
 * @param config 插件配置。
 * @param targetCode 有道目标语言代码。
 * @param batch 本批次分段，文本已压平。
 * @param translatedTexts 输出译文列表，按批次顺序追加。
 * @param error 输出错误信息。
 * @return 本批次翻译成功时返回 true。
 */
bool translateBatch(const YoudaoTranslateConfig &config,
                    const QString &targetCode,
                    const QVector<markshot::plugin::TranslateSegment> &batch,
                    QStringList *translatedTexts,
                    QString *error)
{
    // 1. salt 与 curtime 每批重新生成，复用同一组会被判定为重放请求
    YoudaoFormRequest formRequest;
    formRequest.appKey = config.appKey;
    formRequest.appSecret = config.appSecret;
    formRequest.to = targetCode;
    formRequest.salt = QUuid::createUuid().toString(QUuid::WithoutBraces);
    formRequest.curtime = QString::number(QDateTime::currentSecsSinceEpoch());
    formRequest.queries.reserve(batch.size());
    for (const markshot::plugin::TranslateSegment &segment : batch) {
        formRequest.queries.append(segment.text);
    }

    // 2. 表单编码提交，请求体内已按 UTF-8 做百分号编码
    markshot::translate_common::TranslateHttpRequest httpRequest;
    httpRequest.url = QUrl(config.endpoint);
    httpRequest.method = QByteArrayLiteral("POST");
    httpRequest.contentType = QByteArrayLiteral("application/x-www-form-urlencoded");
    httpRequest.body = buildYoudaoFormBody(formRequest);
    httpRequest.timeoutMs = config.timeoutMs;

    markshot::translate_common::TranslateHttpResponse httpResponse;
    if (!markshot::translate_common::sendTranslateHttpRequest(httpRequest, &httpResponse, error)) {
        return false;
    }

    QStringList batchTranslations;
    if (!parseYoudaoTranslations(httpResponse.body, &batchTranslations, error)) {
        return false;
    }

    // 3. 条数不一致说明服务端合并或拆分了文本，直接失败以免译文错位
    if (batchTranslations.size() != batch.size()) {
        if (error) {
            *error = QStringLiteral("youdao returned %1 translations for %2 segments")
                         .arg(batchTranslations.size())
                         .arg(batch.size());
        }
        return false;
    }
    translatedTexts->append(batchTranslations);
    return true;
}

}  // namespace

QString YoudaoTranslatePlugin::providerId() const
{
    return QStringLiteral("youdao-nmt");
}

QString YoudaoTranslatePlugin::displayName() const
{
    return QStringLiteral("Youdao Translate");
}

bool YoudaoTranslatePlugin::isAvailable(QString *error) const
{
    return validateYoudaoTranslateConfig(readYoudaoTranslateConfig(), error);
}

bool YoudaoTranslatePlugin::translate(const QVector<markshot::plugin::TranslateSegment> &segments,
                                      const QString &targetLanguage,
                                      QVector<markshot::plugin::TranslateSegment> *translations,
                                      QString *error)
{
    if (!translations) {
        if (error) {
            *error = QStringLiteral("translation output target is missing");
        }
        return false;
    }
    translations->clear();
    if (segments.isEmpty()) {
        return true;
    }

    // 1. 配置缺失时立即失败，错误信息指明缺哪个字段
    const YoudaoTranslateConfig config = readYoudaoTranslateConfig();
    if (!validateYoudaoTranslateConfig(config, error)) {
        return false;
    }

    // 2. 源语言固定为 auto，只需解析目标语言
    QString targetCode;
    if (!resolveTargetLanguage(targetLanguage, &targetCode, error)) {
        return false;
    }

    // 3. 段内换行会让批量译文与分段错位，发送前统一压平
    QVector<markshot::plugin::TranslateSegment> flattened;
    flattened.reserve(segments.size());
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        flattened.append({segment.id, markshot::translate_common::flattenSegmentText(segment.text)});
    }

    // 4. 按字符预算分批，逐批发起请求
    const QVector<QVector<markshot::plugin::TranslateSegment>> batches =
        markshot::translate_common::chunkTranslateSegments(flattened, kMaxBatchChars);
    QStringList translatedTexts;
    translatedTexts.reserve(segments.size());
    for (const QVector<markshot::plugin::TranslateSegment> &batch : batches) {
        if (!translateBatch(config, targetCode, batch, &translatedTexts, error)) {
            return false;
        }
    }
    if (translatedTexts.size() != segments.size()) {
        if (error) {
            *error = QStringLiteral("youdao returned %1 translations for %2 segments")
                         .arg(translatedTexts.size())
                         .arg(segments.size());
        }
        return false;
    }

    // 5. 按输入顺序回填，id 保持不变，译文为空的段保留原文
    for (int index = 0; index < segments.size(); ++index) {
        const QString text = translatedTexts.at(index).trimmed();
        translations->append({segments.at(index).id, text.isEmpty() ? segments.at(index).text : text});
    }
    return true;
}

}  // namespace markshot::translate_youdao

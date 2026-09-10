#include "baidu_translate_plugin.h"

#include "baidu_translate_config.h"
#include "baidu_translate_request.h"
#include "baidu_translate_signer.h"

#include "translate_http_client.h"
#include "translate_segment_batch.h"

#include <QHash>
#include <QUrl>

namespace markshot::translate_baidu {
namespace {

// 单批字符预算，低于官方建议的 2000 字符上限，给换行分隔符与编码膨胀留出余量
constexpr int kMaxBatchChars = 1800;

/**
 * 发送单批分段并解析译文。
 * @param config 插件配置。
 * @param batch 已压平的待翻译分段。
 * @param targetCode 百度语言代码。
 * @param translated 输出分段 id 到译文的映射。
 * @param error 输出错误信息。
 * @return 该批翻译成功时返回 true。
 */
bool translateBatch(const BaiduTranslateConfig &config,
                    const QVector<markshot::plugin::TranslateSegment> &batch,
                    const QString &targetCode,
                    QHash<int, QString> *translated,
                    QString *error)
{
    // 1. 先用未编码的原文连接出 q 并计算签名，编码只发生在构造请求体时
    BaiduTranslateForm form;
    form.appId = config.appId;
    form.query = joinBaiduQuery(batch);
    form.to = targetCode;
    form.salt = generateBaiduSalt();
    form.sign = baiduSignature(config.appId, form.query, form.salt, config.appKey);

    // 2. 平台只接受表单编码的 POST 请求
    markshot::translate_common::TranslateHttpRequest request;
    request.url = QUrl(config.endpoint);
    request.method = QByteArrayLiteral("POST");
    request.contentType = QByteArrayLiteral("application/x-www-form-urlencoded");
    request.body = buildBaiduFormBody(form);
    request.timeoutMs = config.timeoutMs;

    markshot::translate_common::TranslateHttpResponse response;
    if (!markshot::translate_common::sendTranslateHttpRequest(request, &response, error)) {
        return false;
    }

    QStringList results;
    if (!parseBaiduTranslateResponse(response.body, &results, error)) {
        return false;
    }

    // 3. 条数不一致说明服务端的切分与请求不同，继续回填会让整批译文错位
    if (results.size() != batch.size()) {
        if (error) {
            *error = QStringLiteral("baidu returned %1 translated segments for %2 requested segments")
                         .arg(results.size())
                         .arg(batch.size());
        }
        return false;
    }

    for (int index = 0; index < batch.size(); ++index) {
        translated->insert(batch.at(index).id, results.at(index).trimmed());
    }
    return true;
}

}  // namespace

QString BaiduTranslatePlugin::providerId() const
{
    return QStringLiteral("baidu-fanyi");
}

QString BaiduTranslatePlugin::displayName() const
{
    return QStringLiteral("Baidu Translate");
}

bool BaiduTranslatePlugin::isAvailable(QString *error) const
{
    return validateBaiduTranslateConfig(readBaiduTranslateConfig(), error);
}

bool BaiduTranslatePlugin::translate(const QVector<markshot::plugin::TranslateSegment> &segments,
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

    // 1. 配置不完整时直接返回，避免用空凭据发起必然失败的请求
    const BaiduTranslateConfig config = readBaiduTranslateConfig();
    if (!validateBaiduTranslateConfig(config, error)) {
        return false;
    }

    // 2. 目标语言归一化后映射为百度语言代码，源语言固定为自动识别
    QString targetCode;
    if (!resolveBaiduTargetLanguage(targetLanguage, &targetCode, error)) {
        return false;
    }

    // 3. 逐段压平，段内换行会破坏译文与分段的对应关系；压平后为空的段不参与请求
    QVector<markshot::plugin::TranslateSegment> pending;
    pending.reserve(segments.size());
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        const QString flattened = markshot::translate_common::flattenSegmentText(segment.text);
        if (!flattened.isEmpty()) {
            pending.append({segment.id, flattened});
        }
    }

    // 4. 按字符预算分批，任意一批失败即整体失败，避免只回填半数译文
    QHash<int, QString> translated;
    const QVector<QVector<markshot::plugin::TranslateSegment>> batches =
        markshot::translate_common::chunkTranslateSegments(pending, kMaxBatchChars);
    for (const QVector<markshot::plugin::TranslateSegment> &batch : batches) {
        if (!translateBatch(config, batch, targetCode, &translated, error)) {
            return false;
        }
    }

    // 5. 按输入顺序回填，译文缺失或为空的段保留原文
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        const QString text = translated.value(segment.id);
        translations->append({segment.id, text.isEmpty() ? segment.text : text});
    }
    return true;
}

}  // namespace markshot::translate_baidu

#include "openai_translate_plugin.h"

#include "openai_translate_config.h"
#include "openai_translation_parser.h"
#include "translate_http_client.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace markshot::translate_openai {
namespace {

/**
 * 拼接 chat/completions 端点。
 * @param apiBase API 根地址。
 * @return chat/completions 完整地址。
 */
QUrl chatCompletionsUrl(QString apiBase)
{
    while (apiBase.endsWith(QLatin1Char('/'))) {
        apiBase.chop(1);
    }
    return QUrl(apiBase + QStringLiteral("/chat/completions"));
}

/**
 * 构造 OpenAI-compatible 请求体。
 * @param config 翻译配置。
 * @param segments 源文分段。
 * @param targetLanguage 目标语言。
 * @return JSON 请求体。
 */
QByteArray requestPayload(const OpenAiTranslateConfig &config,
                          const QVector<markshot::plugin::TranslateSegment> &segments,
                          const QString &targetLanguage)
{
    QJsonArray segmentArray;
    for (const markshot::plugin::TranslateSegment &segment : segments) {
        segmentArray.append(QJsonObject{{QStringLiteral("id"), segment.id},
                                        {QStringLiteral("text"), segment.text}});
    }

    const QJsonObject userPrompt{
        {QStringLiteral("target_language"), targetLanguage.trimmed().isEmpty()
             ? QStringLiteral("Simplified Chinese")
             : targetLanguage.trimmed()},
        {QStringLiteral("instructions"),
         QJsonArray{QStringLiteral("Translate each segment into target_language."),
                    QStringLiteral("Return JSON exactly as {\"translations\":[{\"id\":0,\"text\":\"...\"}]} "
                                   "with no markdown."),
                    QStringLiteral("Do not add explanations.")}},
        {QStringLiteral("segments"), segmentArray}};

    QJsonObject payload = config.extraBody.toObject();
    payload.insert(QStringLiteral("model"), config.model);
    payload.insert(QStringLiteral("temperature"), config.temperature);
    payload.insert(QStringLiteral("messages"),
                   QJsonArray{QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                          {QStringLiteral("content"), config.systemPrompt}},
                              QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("content"),
                                           QString::fromUtf8(QJsonDocument(userPrompt).toJson(QJsonDocument::Compact))}}});
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

/**
 * 从 chat/completions 响应体提取 message.content。
 * @param body HTTP 响应体。
 * @param content 输出模型回复正文。
 * @param error 输出错误信息。
 * @return 提取成功时返回 true。
 */
bool extractMessageContent(const QByteArray &body, QString *content, QString *error)
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    const QJsonArray choices = document.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        if (error) {
            *error = QStringLiteral("llm response missing choices");
        }
        return false;
    }
    const QString value = choices.at(0)
                              .toObject()
                              .value(QStringLiteral("message"))
                              .toObject()
                              .value(QStringLiteral("content"))
                              .toString();
    if (value.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("llm response missing message content");
        }
        return false;
    }
    if (content) {
        *content = value;
    }
    return true;
}

/**
 * 同步发送 HTTP 翻译请求。
 * @param config 翻译配置。
 * @param payload JSON 请求体。
 * @param body 输出响应体。
 * @param error 输出错误信息。
 * @return 请求成功时返回 true。
 */
bool postTranslationRequest(const OpenAiTranslateConfig &config,
                            const QByteArray &payload,
                            QByteArray *body,
                            QString *error)
{
    markshot::translate_common::TranslateHttpRequest request;
    request.url = chatCompletionsUrl(config.apiBase);
    request.contentType = QByteArrayLiteral("application/json");
    request.headers.append({QByteArrayLiteral("Authorization"), "Bearer " + config.apiKey.toUtf8()});
    request.body = payload;
    request.timeoutMs = config.timeoutMs;

    markshot::translate_common::TranslateHttpResponse response;
    if (!markshot::translate_common::sendTranslateHttpRequest(request, &response, error)) {
        return false;
    }

    // 1. chat/completions 用 HTTP 状态码表达失败，非 2xx 一律视为调用出错
    if (response.httpStatus < 200 || response.httpStatus >= 300) {
        if (error) {
            *error = QStringLiteral("llm http %1: %2")
                         .arg(response.httpStatus)
                         .arg(QString::fromUtf8(response.body.left(500)));
        }
        return false;
    }
    if (body) {
        *body = response.body;
    }
    return true;
}

}  // namespace

QString OpenAiTranslatePlugin::providerId() const
{
    return QStringLiteral("openai-compatible");
}

QString OpenAiTranslatePlugin::displayName() const
{
    return QStringLiteral("OpenAI Compatible");
}

bool OpenAiTranslatePlugin::isAvailable(QString *error) const
{
    return validateOpenAiTranslateConfig(readOpenAiTranslateConfig(), error);
}

bool OpenAiTranslatePlugin::translate(const QVector<markshot::plugin::TranslateSegment> &segments,
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

    const OpenAiTranslateConfig config = readOpenAiTranslateConfig();
    if (!validateOpenAiTranslateConfig(config, error)) {
        return false;
    }

    QByteArray body;
    if (!postTranslationRequest(config, requestPayload(config, segments, targetLanguage), &body, error)) {
        return false;
    }

    QString content;
    if (!extractMessageContent(body, &content, error)) {
        return false;
    }
    QHash<int, QString> translatedById;
    if (!parseTranslationContent(content, &translatedById, error)) {
        return false;
    }

    for (const markshot::plugin::TranslateSegment &segment : segments) {
        const QString text = translatedById.value(segment.id).trimmed();
        translations->append({segment.id, text.isEmpty() ? segment.text : text});
    }
    return true;
}

}  // namespace markshot::translate_openai

#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>

namespace markshot::translate_common {

/**
 * 同步 HTTP 请求描述。
 *
 * method 仅支持 GET 与 POST，覆盖三家云翻译服务的全部调用形态。
 */
struct TranslateHttpRequest {
    QUrl url;
    QByteArray method = QByteArrayLiteral("POST");
    QByteArray contentType;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    int timeoutMs = 15000;
};

/**
 * 同步 HTTP 响应内容。
 */
struct TranslateHttpResponse {
    int httpStatus = 0;
    QByteArray body;
};

/**
 * 在当前线程同步发送 HTTP 请求。
 *
 * 云翻译服务普遍在 HTTP 200 下用响应体表达业务错误，因此只要拿到响应体就返回
 * true，由调用方按各自协议判定成败；仅连接失败、超时等传输层问题返回 false。
 *
 * @param request 请求描述。
 * @param response 输出响应状态码与响应体。
 * @param error 输出传输层错误信息。
 * @return 取得响应时返回 true。
 */
bool sendTranslateHttpRequest(const TranslateHttpRequest &request,
                              TranslateHttpResponse *response,
                              QString *error);

}  // namespace markshot::translate_common

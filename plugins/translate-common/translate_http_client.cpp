#include "translate_http_client.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace markshot::translate_common {

bool sendTranslateHttpRequest(const TranslateHttpRequest &request,
                              TranslateHttpResponse *response,
                              QString *error)
{
    if (!response) {
        if (error) {
            *error = QStringLiteral("http response target is missing");
        }
        return false;
    }

    // 1. 组装请求头，Content-Type 必须与签名时使用的字面量保持一致
    QNetworkAccessManager network;
    QNetworkRequest networkRequest(request.url);
    if (!request.contentType.isEmpty()) {
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, request.contentType);
    }
    for (const QPair<QByteArray, QByteArray> &header : request.headers) {
        networkRequest.setRawHeader(header.first, header.second);
    }

    // 2. 用局部事件循环把异步请求转成同步调用，插件本身运行在工作线程
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QNetworkReply *reply = request.method == QByteArrayLiteral("GET")
                               ? network.get(networkRequest)
                               : network.post(networkRequest, request.body);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&loop, reply] {
        reply->abort();
        loop.quit();
    });
    timeoutTimer.start(request.timeoutMs);
    loop.exec();

    const bool timedOut = !timeoutTimer.isActive();
    timeoutTimer.stop();
    const QByteArray body = reply->readAll();
    const QNetworkReply::NetworkError replyError = reply->error();
    const QString replyErrorText = reply->errorString();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if (timedOut) {
        if (error) {
            *error = QStringLiteral("request timed out after %1 ms").arg(request.timeoutMs);
        }
        return false;
    }

    // 3. 云翻译服务常在非 2xx 响应里放业务错误码，拿到响应体就交给调用方判定
    if (replyError != QNetworkReply::NoError && body.isEmpty()) {
        if (error) {
            *error = QStringLiteral("http %1: %2").arg(httpStatus).arg(replyErrorText);
        }
        return false;
    }

    response->httpStatus = httpStatus;
    response->body = body;
    return true;
}

}  // namespace markshot::translate_common

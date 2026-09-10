#include "baidu_translate_config.h"
#include "baidu_translate_plugin.h"
#include "baidu_translate_request.h"
#include "baidu_translate_signer.h"

#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

#include <memory>

using namespace markshot::translate_baidu;

namespace {

class EnvGuard {
public:
    /**
     * 保存并设置环境变量。
     * @param name 环境变量名。
     * @param value 临时变量值。
     */
    EnvGuard(QByteArray name, QByteArray value)
        : m_name(std::move(name))
        , m_hadValue(qEnvironmentVariableIsSet(m_name.constData()))
        , m_oldValue(qgetenv(m_name.constData()))
    {
        qputenv(m_name.constData(), value);
    }

    ~EnvGuard()
    {
        if (m_hadValue) {
            qputenv(m_name.constData(), m_oldValue);
        } else {
            qunsetenv(m_name.constData());
        }
    }

private:
    QByteArray m_name;
    bool m_hadValue = false;
    QByteArray m_oldValue;
};

class MockBaiduServer final : public QTcpServer {
public:
    /**
     * 创建 mock 百度翻译服务。
     * @param responseBody 固定响应体。
     */
    explicit MockBaiduServer(QByteArray responseBody)
        : m_responseBody(std::move(responseBody))
    {
        connect(this, &QTcpServer::newConnection, this, [this] { handleConnection(); });
    }

    /**
     * 启动本地监听。
     * @return 启动成功时返回 true。
     */
    bool start()
    {
        return listen(QHostAddress::LocalHost, 0);
    }

    /**
     * 读取 mock 服务的翻译接口地址。
     * @return 翻译接口完整地址。
     */
    QString endpoint() const
    {
        return QStringLiteral("http://127.0.0.1:%1/api/trans/vip/translate").arg(serverPort());
    }

    /**
     * 读取最近一次收到的请求体。
     * @return HTTP 请求体。
     */
    QByteArray requestBody() const { return m_requestBody; }

    /**
     * 读取收到的请求次数。
     * @return 请求次数。
     */
    int requestCount() const { return m_requestCount; }

private:
    /**
     * 处理新连接并回放固定响应。
     * @return 无返回值。
     */
    void handleConnection()
    {
        QTcpSocket *socket = nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
            buffer->append(socket->readAll());

            // 1. 先等待请求头完整，再按 Content-Length 判断请求体是否收齐
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const QByteArray header = buffer->left(headerEnd);
            const QByteArray marker = QByteArrayLiteral("Content-Length:");
            const int lengthPos = header.indexOf(marker);
            int contentLength = 0;
            if (lengthPos >= 0) {
                const int lineEnd = header.indexOf('\n', lengthPos);
                contentLength = header.mid(lengthPos + marker.size(),
                                           lineEnd < 0 ? -1 : lineEnd - lengthPos - marker.size())
                                    .trimmed()
                                    .toInt();
            }
            if (buffer->size() < headerEnd + 4 + contentLength) {
                return;
            }

            // 2. 记录请求体供断言，再回放固定响应并关闭连接
            m_requestBody = buffer->mid(headerEnd + 4, contentLength);
            ++m_requestCount;
            const QByteArray response =
                QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n")
                + "Content-Length: " + QByteArray::number(m_responseBody.size())
                + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + m_responseBody;
            socket->write(response);
            socket->disconnectFromHost();
        });
    }

    QByteArray m_responseBody;
    QByteArray m_requestBody;
    int m_requestCount = 0;
};

/**
 * 写入临时 Mark Shot 配置。
 * @param endpoint 翻译接口地址。
 * @return 临时配置文件，创建失败时返回空指针。
 */
QTemporaryFile *writeTempConfig(const QString &endpoint)
{
    auto *file = new QTemporaryFile();
    if (!file->open()) {
        delete file;
        return nullptr;
    }
    QJsonObject baidu;
    baidu.insert(QStringLiteral("appId"), QStringLiteral("test-app-id"));
    baidu.insert(QStringLiteral("appKey"), QStringLiteral("test-app-key"));
    baidu.insert(QStringLiteral("endpoint"), endpoint);
    baidu.insert(QStringLiteral("timeoutMs"), 5000);
    QJsonObject translation;
    translation.insert(QStringLiteral("baidu"), baidu);
    QJsonObject root;
    root.insert(QStringLiteral("translation"), translation);
    file->write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file->flush();
    return file;
}

/**
 * 构造 trans_result 响应体。
 * @param pairs 原文与译文组成的列表。
 * @return JSON 响应体。
 */
QByteArray makeTransResult(const QVector<QPair<QString, QString>> &pairs)
{
    QJsonArray results;
    for (const QPair<QString, QString> &pair : pairs) {
        results.append(QJsonObject{{QStringLiteral("src"), pair.first},
                                   {QStringLiteral("dst"), pair.second}});
    }
    const QJsonObject root{{QStringLiteral("from"), QStringLiteral("en")},
                           {QStringLiteral("to"), QStringLiteral("zh")},
                           {QStringLiteral("trans_result"), results}};
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

}  // namespace

class TranslateBaiduPluginTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证官方示例的签名原文与签名值。
     * @return 无返回值。
     */
    void signsOfficialSample()
    {
        const QString appId = QStringLiteral("2015063000000001");
        const QString query = QStringLiteral("apple");
        const QString salt = QStringLiteral("65478");
        const QString appKey = QStringLiteral("1234567890");
        QCOMPARE(baiduSignatureSource(appId, query, salt, appKey),
                 QStringLiteral("2015063000000001apple654781234567890"));
        QCOMPARE(baiduSignature(appId, query, salt, appKey),
                 QStringLiteral("a1a7461d92e5194c5cae3182b5b24de1"));
    }

    /**
     * 验证含中文与特殊字符的待翻译文本参与签名时不被 URL 编码。
     * @return 无返回值。
     */
    void keepsQueryRawInSignatureSource()
    {
        const QString query = QStringLiteral("你好 世界 & apple=1+2");
        const QString source = baiduSignatureSource(QStringLiteral("app"),
                                                    query,
                                                    QStringLiteral("42"),
                                                    QStringLiteral("key"));
        QCOMPARE(source, QStringLiteral("app") + query + QStringLiteral("42key"));
        QVERIFY(source.contains(query));
        QVERIFY(!source.contains(QLatin1Char('%')));
    }

    /**
     * 验证规范语言键到百度语言代码的映射。
     * @return 无返回值。
     */
    void mapsCanonicalLanguageKeys()
    {
        QCOMPARE(baiduLanguageCode(QStringLiteral("zh-Hans")), QStringLiteral("zh"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("zh-Hant")), QStringLiteral("cht"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("ja")), QStringLiteral("jp"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("ko")), QStringLiteral("kor"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("fr")), QStringLiteral("fra"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("es")), QStringLiteral("spa"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("ar")), QStringLiteral("ara"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("vi")), QStringLiteral("vie"));
        QCOMPARE(baiduLanguageCode(QStringLiteral("ms")), QStringLiteral("may"));
        QVERIFY(baiduLanguageCode(QStringLiteral("sv")).isEmpty());

        QString code;
        QString error;
        QVERIFY2(resolveBaiduTargetLanguage(QStringLiteral("Japanese"), &code, &error), qPrintable(error));
        QCOMPARE(code, QStringLiteral("jp"));
        QVERIFY2(resolveBaiduTargetLanguage(QStringLiteral("繁体中文"), &code, &error), qPrintable(error));
        QCOMPARE(code, QStringLiteral("cht"));
        QVERIFY(!resolveBaiduTargetLanguage(QStringLiteral("Klingon"), &code, &error));
        QVERIFY(!error.isEmpty());
    }

    /**
     * 验证表单请求体的编码规则。
     * @return 无返回值。
     */
    void encodesFormBody()
    {
        BaiduTranslateForm form;
        form.appId = QStringLiteral("app");
        form.query = QStringLiteral("你好\nworld cup");
        form.to = QStringLiteral("zh");
        form.salt = QStringLiteral("42");
        form.sign = QStringLiteral("abc");

        const QByteArray body = buildBaiduFormBody(form);
        QVERIFY(body.startsWith("q="));
        QVERIFY(body.contains("%E4%BD%A0%E5%A5%BD"));
        QVERIFY(body.contains("%0A"));
        QVERIFY(body.contains("world+cup"));
        QVERIFY(body.contains("&from=auto&to=zh&appid=app&salt=42&sign=abc"));
    }

    /**
     * 验证插件可走通一次成功翻译。
     * @return 无返回值。
     */
    void translatesViaMockServer()
    {
        MockBaiduServer server(makeTransResult({{QStringLiteral("hello"), QStringLiteral("你好")},
                                                {QStringLiteral("world"), QStringLiteral("世界")}}));
        QVERIFY(server.start());

        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        BaiduTranslatePlugin plugin;
        QString error;
        QVERIFY2(plugin.isAvailable(&error), qPrintable(error));

        QVector<markshot::plugin::TranslateSegment> translations;
        QVERIFY2(plugin.translate({{3, QStringLiteral("hello")}, {5, QStringLiteral("world")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error),
                 qPrintable(error));

        QCOMPARE(translations.size(), 2);
        QCOMPARE(translations.at(0).id, 3);
        QCOMPARE(translations.at(0).text, QStringLiteral("你好"));
        QCOMPARE(translations.at(1).id, 5);
        QCOMPARE(translations.at(1).text, QStringLiteral("世界"));

        const QByteArray body = server.requestBody();
        QCOMPARE(server.requestCount(), 1);
        QVERIFY(body.contains("q=hello%0Aworld"));
        QVERIFY(body.contains("from=auto"));
        QVERIFY(body.contains("to=zh"));
        QVERIFY(body.contains("appid=test-app-id"));
        QVERIFY(body.contains("salt="));
        QVERIFY(body.contains("sign="));
    }

    /**
     * 验证平台错误码被带回错误信息。
     * @return 无返回值。
     */
    void reportsBaiduErrorCode()
    {
        MockBaiduServer server(QByteArrayLiteral("{\"error_code\":\"54001\",\"error_msg\":\"Invalid Sign\"}"));
        QVERIFY(server.start());

        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        BaiduTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY(error.contains(QStringLiteral("54001")));
        QVERIFY(error.contains(QStringLiteral("Invalid Sign")));
    }

    /**
     * 验证译文条数与请求段数不一致时整体失败。
     * @return 无返回值。
     */
    void rejectsSegmentCountMismatch()
    {
        MockBaiduServer server(makeTransResult({{QStringLiteral("hello"), QStringLiteral("你好")}}));
        QVERIFY(server.start());

        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        BaiduTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}, {2, QStringLiteral("world")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY(error.contains(QStringLiteral("1")));
        QVERIFY(error.contains(QStringLiteral("2")));
        QVERIFY(translations.isEmpty());
    }

    /**
     * 验证缺少凭据时报出可操作的错误信息。
     * @return 无返回值。
     */
    void reportsMissingCredentials()
    {
        BaiduTranslateConfig empty;
        QString error;
        QVERIFY(!validateBaiduTranslateConfig(empty, &error));
        QVERIFY(error.contains(QStringLiteral("appId")));
        QVERIFY(error.contains(QStringLiteral("MARK_SHOT_BAIDU_APP_ID")));

        empty.appId = QStringLiteral("app");
        QVERIFY(!validateBaiduTranslateConfig(empty, &error));
        QVERIFY(error.contains(QStringLiteral("appKey")));
        QVERIFY(error.contains(QStringLiteral("BAIDU_TRANSLATE_APP_KEY")));
    }
};

QTEST_GUILESS_MAIN(TranslateBaiduPluginTest)
#include "translate_baidu_plugin_test.moc"

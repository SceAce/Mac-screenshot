#include "tencent_tc3_signer.h"
#include "tencent_translate_config.h"
#include "tencent_translate_plugin.h"
#include "tencent_translate_request.h"

#include <QtTest/QtTest>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

#include <memory>

using namespace markshot::translate_tencent;

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

class MockTencentServer final : public QTcpServer {
public:
    /**
     * 创建腾讯云 API 的本地 mock 服务。
     * @param responses 按调用顺序返回的响应体列表。
     */
    explicit MockTencentServer(QList<QByteArray> responses)
        : m_responses(std::move(responses))
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
     * 读取供插件配置使用的接入点。
     * @return 带 http 协议前缀的本地接入点。
     */
    QString endpoint() const
    {
        return QStringLiteral("http://127.0.0.1:%1").arg(serverPort());
    }

    /**
     * 读取收到的全部请求头。
     * @return 按调用顺序排列的请求头文本。
     */
    QList<QByteArray> requestHeaders() const { return m_requestHeaders; }

    /**
     * 读取收到的全部请求体。
     * @return 按调用顺序排列的请求体。
     */
    QList<QByteArray> requestBodies() const { return m_requestBodies; }

private:
    /**
     * 解析请求头中的 Content-Length。
     * @param header 请求头文本。
     * @return 请求体字节数，缺失时为 0。
     */
    static int contentLength(const QByteArray &header)
    {
        const QByteArray marker = QByteArrayLiteral("Content-Length:");
        const int markerPos = header.indexOf(marker);
        if (markerPos < 0) {
            return 0;
        }
        const int lineEnd = header.indexOf('\n', markerPos);
        return header
            .mid(markerPos + marker.size(), lineEnd < 0 ? -1 : lineEnd - markerPos - marker.size())
            .trimmed()
            .toInt();
    }

    /**
     * 处理新连接并按顺序回放响应。
     * @return 无返回值。
     */
    void handleConnection()
    {
        QTcpSocket *socket = nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
            buffer->append(socket->readAll());

            // 1. 请求头未接收完整时继续等待
            const int headerEnd = buffer->indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const QByteArray header = buffer->left(headerEnd);
            const int bodySize = contentLength(header);
            if (buffer->size() < headerEnd + 4 + bodySize) {
                return;
            }

            // 2. 记录请求，请求头保留末行换行以便按整行匹配 Action
            m_requestHeaders.append(buffer->left(headerEnd + 2));
            m_requestBodies.append(buffer->mid(headerEnd + 4, bodySize));

            // 3. 按调用顺序取出响应体，用完后返回空对象便于暴露多余的调用
            const QByteArray responseBody =
                m_responses.isEmpty() ? QByteArrayLiteral("{}") : m_responses.takeFirst();
            socket->write(QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n")
                          + "Content-Length: " + QByteArray::number(responseBody.size())
                          + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + responseBody);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }

    QList<QByteArray> m_responses;
    QList<QByteArray> m_requestHeaders;
    QList<QByteArray> m_requestBodies;
};

/**
 * 写入指向 mock 服务的临时 Mark Shot 配置。
 * @param endpoint mock 服务接入点。
 * @return 临时配置文件，创建失败时为空指针。
 */
QTemporaryFile *writeTempConfig(const QString &endpoint)
{
    auto *file = new QTemporaryFile();
    if (!file->open()) {
        delete file;
        return nullptr;
    }
    QJsonObject tencent;
    tencent.insert(QStringLiteral("secretId"), QStringLiteral("AKIDTESTSECRETID"));
    tencent.insert(QStringLiteral("secretKey"), QStringLiteral("TESTSECRETKEY"));
    tencent.insert(QStringLiteral("region"), QStringLiteral("ap-guangzhou"));
    tencent.insert(QStringLiteral("endpoint"), endpoint);
    tencent.insert(QStringLiteral("projectId"), 0);
    tencent.insert(QStringLiteral("timeoutMs"), 5000);

    QJsonObject translation;
    translation.insert(QStringLiteral("tencent"), tencent);
    QJsonObject root;
    root.insert(QStringLiteral("translation"), translation);
    file->write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file->flush();
    return file;
}

/**
 * 构造 TextTranslateBatch 的成功响应体。
 * @param texts 译文列表。
 * @return JSON 响应体。
 */
QByteArray batchResponse(const QStringList &texts)
{
    QJsonArray targetTextList;
    for (const QString &text : texts) {
        targetTextList.append(text);
    }
    QJsonObject response;
    response.insert(QStringLiteral("TargetTextList"), targetTextList);
    response.insert(QStringLiteral("Source"), QStringLiteral("en"));
    response.insert(QStringLiteral("Target"), QStringLiteral("zh"));
    response.insert(QStringLiteral("RequestId"), QStringLiteral("req-batch"));
    QJsonObject root;
    root.insert(QStringLiteral("Response"), response);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * 构造 TextTranslate 的成功响应体。
 * @param text 译文。
 * @return JSON 响应体。
 */
QByteArray textResponse(const QString &text)
{
    QJsonObject response;
    response.insert(QStringLiteral("TargetText"), text);
    response.insert(QStringLiteral("Source"), QStringLiteral("en"));
    response.insert(QStringLiteral("Target"), QStringLiteral("zh"));
    response.insert(QStringLiteral("RequestId"), QStringLiteral("req-text"));
    QJsonObject root;
    root.insert(QStringLiteral("Response"), response);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * 构造业务错误响应体。
 * @param code 错误码。
 * @param message 错误描述。
 * @return JSON 响应体。
 */
QByteArray errorResponse(const QString &code, const QString &message)
{
    QJsonObject errorObject;
    errorObject.insert(QStringLiteral("Code"), code);
    errorObject.insert(QStringLiteral("Message"), message);
    QJsonObject response;
    response.insert(QStringLiteral("Error"), errorObject);
    response.insert(QStringLiteral("RequestId"), QStringLiteral("req-error"));
    QJsonObject root;
    root.insert(QStringLiteral("Response"), response);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

/**
 * 从请求头文本中读取指定字段的取值。
 *
 * Qt 会把自定义请求头名归一化成 X-Tc-Action 这样的形式，HTTP 头名本就大小写不敏感，
 * 因此按小写名逐行匹配。
 *
 * @param header 请求头文本，每行以换行结尾。
 * @param name 请求头名称。
 * @return 去除首尾空白的字段取值，未找到时为空。
 */
QByteArray headerValue(const QByteArray &header, const QByteArray &name)
{
    const QByteArray lowered = header.toLower();
    const QByteArray needle = name.toLower() + ":";
    int lineStart = 0;
    while (lineStart < lowered.size()) {
        const int newlinePos = lowered.indexOf('\n', lineStart);
        const int lineStop = newlinePos < 0 ? lowered.size() : newlinePos;
        if (lowered.mid(lineStart, lineStop - lineStart).startsWith(needle)) {
            return header.mid(lineStart + needle.size(), lineStop - lineStart - needle.size()).trimmed();
        }
        if (newlinePos < 0) {
            break;
        }
        lineStart = newlinePos + 1;
    }
    return {};
}

/**
 * 构造用于签名用例的固定输入。
 * @return 签名输入。
 */
Tc3SigningInput sampleSigningInput()
{
    Tc3SigningInput input;
    input.host = QStringLiteral("tmt.tencentcloudapi.com");
    input.payload = QByteArrayLiteral("{\"Source\":\"auto\",\"Target\":\"zh\"}");
    input.secretId = QStringLiteral("AKIDEXAMPLE");
    input.secretKey = QStringLiteral("SECRETKEYEXAMPLE");
    input.timestamp = 1551113065;
    return input;
}

}  // namespace

class TranslateTencentPluginTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证三级派生密钥与腾讯云官方示例一致。
     * @return 无返回值。
     */
    void derivesSigningKeysFromOfficialSample()
    {
        const Tc3DerivedKeys keys = tc3DerivedKeys(QString(32, QLatin1Char('*')),
                                                   QStringLiteral("2019-02-25"),
                                                   QStringLiteral("cvm"));
        QCOMPARE(keys.secretDate.toHex(),
                 QByteArrayLiteral("da98fb70dcf6b112dc21038d1eeeb3a95c74b4dcb12c1131f864f6066bd02be0"));
        QCOMPARE(keys.secretService.toHex(),
                 QByteArrayLiteral("8d70cbefb03939f929db64d32dc2ba89b1095620119fe3e050e2b18c5bd2752f"));
        QCOMPARE(keys.secretSigning.toHex(),
                 QByteArrayLiteral("b596b923aad85185e2d1f6659d2a062e0a86731226e021e61bfe06f7ed05f5af"));
    }

    /**
     * 验证凭证范围里的日期按 UTC 计算。
     * @return 无返回值。
     */
    void usesUtcDateForCredentialScope()
    {
        // 1551113065 在 UTC 是 2019-02-25 16:44，在东八区已跨到 2019-02-26
        QCOMPARE(tc3UtcDate(1551113065), QStringLiteral("2019-02-25"));
        QCOMPARE(tc3CredentialScope(1551113065, QStringLiteral("tmt")),
                 QStringLiteral("2019-02-25/tmt/tc3_request"));
    }

    /**
     * 验证规范请求串的文本布局，含两处空行。
     * @return 无返回值。
     */
    void buildsCanonicalRequestLayout()
    {
        const Tc3SigningInput input = sampleSigningInput();
        const QByteArray expected =
            QByteArrayLiteral("POST\n"
                              "/\n"
                              "\n"
                              "content-type:application/json; charset=utf-8\n"
                              "host:tmt.tencentcloudapi.com\n"
                              "\n"
                              "content-type;host\n")
            + QCryptographicHash::hash(input.payload, QCryptographicHash::Sha256).toHex();
        QCOMPARE(tc3CanonicalRequest(input), expected);
    }

    /**
     * 验证待签名串的四行布局。
     * @return 无返回值。
     */
    void buildsStringToSignLayout()
    {
        const Tc3SigningInput input = sampleSigningInput();
        const QByteArray canonicalRequest = tc3CanonicalRequest(input);
        const QByteArray expected =
            QByteArrayLiteral("TC3-HMAC-SHA256\n1551113065\n2019-02-25/tmt/tc3_request\n")
            + QCryptographicHash::hash(canonicalRequest, QCryptographicHash::Sha256).toHex();
        QCOMPARE(tc3StringToSign(input, canonicalRequest), expected);
    }

    /**
     * 验证 Authorization 头的格式与签名的确定性。
     * @return 无返回值。
     */
    void buildsDeterministicAuthorizationHeader()
    {
        const Tc3SigningInput input = sampleSigningInput();
        const QByteArray header = tc3AuthorizationHeader(input);
        const QByteArray prefix =
            QByteArrayLiteral("TC3-HMAC-SHA256 Credential=AKIDEXAMPLE/2019-02-25/tmt/tc3_request, "
                              "SignedHeaders=content-type;host, Signature=");
        QVERIFY2(header.startsWith(prefix), header.constData());

        const QByteArray signature = header.mid(prefix.size());
        QCOMPARE(signature.size(), 64);
        QCOMPARE(signature, tc3Signature(input, tc3StringToSign(input, tc3CanonicalRequest(input))));
        QCOMPARE(header, tc3AuthorizationHeader(input));
    }

    /**
     * 验证规范语言键到腾讯云语言代码的映射。
     * @return 无返回值。
     */
    void mapsCanonicalLanguageKeys()
    {
        QCOMPARE(tencentLanguageCode(QStringLiteral("zh-Hans")), QStringLiteral("zh"));
        QCOMPARE(tencentLanguageCode(QStringLiteral("zh-Hant")), QStringLiteral("zh-TW"));
        QCOMPARE(tencentLanguageCode(QStringLiteral("en")), QStringLiteral("en"));
        QVERIFY(tencentLanguageCode(QStringLiteral("nl")).isEmpty());
        QVERIFY(tencentLanguageCode(QStringLiteral("yue")).isEmpty());
    }

    /**
     * 验证不支持的目标语言在翻译前即被拒绝。
     * @return 无返回值。
     */
    void rejectsUnsupportedTargetLanguage()
    {
        MockTencentServer server(QList<QByteArray>{});
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        TencentTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}},
                                  QStringLiteral("Dutch"),
                                  &translations,
                                  &error));
        QVERIFY2(error.contains(QStringLiteral("does not support")), qPrintable(error));
        QVERIFY(server.requestBodies().isEmpty());
    }

    /**
     * 验证批量接口的成功路径与译文回填。
     * @return 无返回值。
     */
    void translatesViaBatchAction()
    {
        MockTencentServer server({batchResponse({QStringLiteral("你好"), QStringLiteral("世界")})});
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        TencentTranslatePlugin plugin;
        QString error;
        QVERIFY2(plugin.isAvailable(&error), qPrintable(error));

        QVector<markshot::plugin::TranslateSegment> translations;
        QVERIFY2(plugin.translate({{3, QStringLiteral("hello")}, {9, QStringLiteral("world")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error),
                 qPrintable(error));

        QCOMPARE(translations.size(), 2);
        QCOMPARE(translations.at(0).id, 3);
        QCOMPARE(translations.at(0).text, QStringLiteral("你好"));
        QCOMPARE(translations.at(1).id, 9);
        QCOMPARE(translations.at(1).text, QStringLiteral("世界"));

        QCOMPARE(server.requestBodies().size(), 1);
        const QByteArray body = server.requestBodies().first();
        QVERIFY2(body.contains("\"SourceTextList\""), body.constData());
        QVERIFY2(body.contains("\"Target\":\"zh\""), body.constData());
        QVERIFY2(body.contains("\"Source\":\"auto\""), body.constData());

        const QByteArray header = server.requestHeaders().first();
        QCOMPARE(headerValue(header, "X-TC-Action"), QByteArrayLiteral("TextTranslateBatch"));
        QCOMPARE(headerValue(header, "X-TC-Version"), QByteArrayLiteral("2018-03-21"));
        QCOMPARE(headerValue(header, "X-TC-Region"), QByteArrayLiteral("ap-guangzhou"));

        // 实际发送的 Content-Type 与 Host 必须与签名时使用的字面量一致
        QCOMPARE(headerValue(header, "Content-Type"),
                 QByteArrayLiteral("application/json; charset=utf-8"));
        QCOMPARE(headerValue(header, "Host"),
                 QStringLiteral("127.0.0.1:%1").arg(server.serverPort()).toUtf8());
        QCOMPARE(header.toLower().count(QByteArrayLiteral("host:")), 1);
        QVERIFY2(headerValue(header, "Authorization")
                     .startsWith("TC3-HMAC-SHA256 Credential=AKIDTESTSECRETID/"),
                 header.constData());
        QVERIFY2(headerValue(header, "Authorization")
                     .contains(", SignedHeaders=content-type;host, Signature="),
                 header.constData());
    }

    /**
     * 验证批量接口下线时降级为逐段调用。
     * @return 无返回值。
     */
    void fallsBackToTextTranslateWhenBatchOffline()
    {
        MockTencentServer server({errorResponse(QStringLiteral("ActionOffline"),
                                                QStringLiteral("action is offline")),
                                  textResponse(QStringLiteral("你好")),
                                  textResponse(QStringLiteral("世界"))});
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        TencentTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY2(plugin.translate({{1, QStringLiteral("hello")}, {2, QStringLiteral("world")}},
                                  QStringLiteral("zh-CN"),
                                  &translations,
                                  &error),
                 qPrintable(error));

        QCOMPARE(translations.size(), 2);
        QCOMPARE(translations.at(0).id, 1);
        QCOMPARE(translations.at(0).text, QStringLiteral("你好"));
        QCOMPARE(translations.at(1).id, 2);
        QCOMPARE(translations.at(1).text, QStringLiteral("世界"));

        const QList<QByteArray> headers = server.requestHeaders();
        QCOMPARE(headers.size(), 3);
        QCOMPARE(headerValue(headers.at(0), "X-TC-Action"), QByteArrayLiteral("TextTranslateBatch"));
        QCOMPARE(headerValue(headers.at(1), "X-TC-Action"), QByteArrayLiteral("TextTranslate"));
        QCOMPARE(headerValue(headers.at(2), "X-TC-Action"), QByteArrayLiteral("TextTranslate"));
        QVERIFY2(server.requestBodies().at(1).contains("\"SourceText\""),
                 server.requestBodies().at(1).constData());
    }

    /**
     * 验证译文条数与分段数不一致时拒绝回填。
     * @return 无返回值。
     */
    void rejectsMismatchedTranslationCount()
    {
        MockTencentServer server({batchResponse({QStringLiteral("你好")})});
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        TencentTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}, {2, QStringLiteral("world")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY2(error.contains(QStringLiteral("1 translations for 2")), qPrintable(error));
        QVERIFY(translations.isEmpty());
    }

    /**
     * 验证业务错误信息带上错误码与请求 ID。
     * @return 无返回值。
     */
    void reportsApiErrorDetails()
    {
        MockTencentServer server({errorResponse(QStringLiteral("AuthFailure.SignatureFailure"),
                                                QStringLiteral("signature mismatch"))});
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        TencentTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY2(error.contains(QStringLiteral("AuthFailure.SignatureFailure")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("signature mismatch")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("req-error")), qPrintable(error));
    }

    /**
     * 验证缺少凭据时给出可操作的错误信息。
     * @return 无返回值。
     */
    void reportsMissingCredentials()
    {
        std::unique_ptr<QTemporaryFile> config(new QTemporaryFile());
        QVERIFY(config->open());
        config->write(QByteArrayLiteral("{\"translation\":{\"tencent\":{}}}"));
        config->flush();

        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());
        EnvGuard secretIdGuard(QByteArrayLiteral("TENCENTCLOUD_SECRET_ID"), QByteArray());
        EnvGuard secretKeyGuard(QByteArrayLiteral("TENCENTCLOUD_SECRET_KEY"), QByteArray());
        EnvGuard markShotIdGuard(QByteArrayLiteral("MARK_SHOT_TENCENT_SECRET_ID"), QByteArray());
        EnvGuard markShotKeyGuard(QByteArrayLiteral("MARK_SHOT_TENCENT_SECRET_KEY"), QByteArray());

        TencentTranslatePlugin plugin;
        QString error;
        QVERIFY(!plugin.isAvailable(&error));
        QVERIFY2(error.contains(QStringLiteral("secretId")), qPrintable(error));
        QVERIFY2(error.contains(QStringLiteral("TENCENTCLOUD_SECRET_ID")), qPrintable(error));
    }

    /**
     * 验证接入点解析出的主机名与请求地址。
     * @return 无返回值。
     */
    void resolvesEndpointHostAndUrl()
    {
        const TencentEndpoint defaultEndpoint =
            resolveTencentEndpoint(QStringLiteral("tmt.tencentcloudapi.com"));
        QCOMPARE(defaultEndpoint.host, QStringLiteral("tmt.tencentcloudapi.com"));
        QCOMPARE(defaultEndpoint.url.toString(), QStringLiteral("https://tmt.tencentcloudapi.com/"));

        const TencentEndpoint localEndpoint =
            resolveTencentEndpoint(QStringLiteral("http://127.0.0.1:9000/"));
        QCOMPARE(localEndpoint.host, QStringLiteral("127.0.0.1:9000"));
        QCOMPARE(localEndpoint.url.toString(), QStringLiteral("http://127.0.0.1:9000/"));
    }

    /**
     * 验证配置默认值与超时区间截断。
     * @return 无返回值。
     */
    void readsConfigDefaults()
    {
        std::unique_ptr<QTemporaryFile> file(new QTemporaryFile());
        QVERIFY(file->open());
        file->write(QByteArrayLiteral(
            "{\"translation\":{\"tencent\":{\"secretId\":\"id\",\"secretKey\":\"key\",\"timeoutMs\":10}}}"));
        file->flush();
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), file->fileName().toUtf8());
        EnvGuard regionGuard(QByteArrayLiteral("TENCENTCLOUD_REGION"), QByteArray());
        EnvGuard markShotRegionGuard(QByteArrayLiteral("MARK_SHOT_TENCENT_REGION"), QByteArray());

        const TencentTranslateConfig config = readTencentTranslateConfig();
        QCOMPARE(config.region, QStringLiteral("ap-guangzhou"));
        QCOMPARE(config.endpoint, QStringLiteral("tmt.tencentcloudapi.com"));
        QCOMPARE(config.projectId, 0);
        QCOMPARE(config.timeoutMs, 1000);

        QString error;
        QVERIFY2(validateTencentTranslateConfig(config, &error), qPrintable(error));
    }
};

QTEST_GUILESS_MAIN(TranslateTencentPluginTest)
#include "translate_tencent_plugin_test.moc"

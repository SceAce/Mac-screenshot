#include "youdao_translate_plugin.h"
#include "youdao_translate_request.h"
#include "youdao_translate_signer.h"

#include <QtTest/QtTest>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QUrlQuery>

#include <memory>

using namespace markshot::translate_youdao;

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

class MockYoudaoServer final : public QTcpServer {
public:
    /**
     * 创建 mock 有道翻译服务。
     * @param responseBody 固定响应体。
     */
    explicit MockYoudaoServer(QByteArray responseBody)
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
     * 读取翻译接口地址。
     * @return mock 服务的 api 端点。
     */
    QString endpoint() const
    {
        return QStringLiteral("http://127.0.0.1:%1/api").arg(serverPort());
    }

    /**
     * 读取收到的请求体。
     * @return HTTP 请求体。
     */
    QByteArray requestBody() const { return m_requestBody; }

private:
    /**
     * 处理新连接并按 Content-Length 收齐请求体。
     * @return 无返回值。
     */
    void handleConnection()
    {
        QTcpSocket *socket = nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            m_request += socket->readAll();
            const int headerEnd = m_request.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const QByteArray header = m_request.left(headerEnd);
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
            if (m_request.size() < headerEnd + 4 + contentLength) {
                return;
            }
            m_requestBody = m_request.mid(headerEnd + 4, contentLength);
            const QByteArray response = QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n")
                + "Content-Length: " + QByteArray::number(m_responseBody.size())
                + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + m_responseBody;
            socket->write(response);
            socket->disconnectFromHost();
        });
    }

    QByteArray m_responseBody;
    QByteArray m_request;
    QByteArray m_requestBody;
};

/**
 * 写入临时 Mark Shot 配置。
 * @param endpoint 有道翻译接口地址。
 * @return 临时配置文件，创建失败时返回空指针。
 */
QTemporaryFile *writeTempConfig(const QString &endpoint)
{
    auto *file = new QTemporaryFile();
    if (!file->open()) {
        delete file;
        return nullptr;
    }
    QJsonObject youdao;
    youdao.insert(QStringLiteral("appKey"), QStringLiteral("test-app-key"));
    youdao.insert(QStringLiteral("appSecret"), QStringLiteral("test-app-secret"));
    youdao.insert(QStringLiteral("endpoint"), endpoint);
    youdao.insert(QStringLiteral("timeoutMs"), 5000);
    QJsonObject translation;
    translation.insert(QStringLiteral("youdao"), youdao);
    QJsonObject root;
    root.insert(QStringLiteral("translation"), translation);
    file->write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file->flush();
    return file;
}

/**
 * 构造成功响应体。
 * @param translations 译文列表。
 * @return JSON 响应体。
 */
QByteArray successResponse(const QStringList &translations)
{
    QJsonArray array;
    for (const QString &translation : translations) {
        array.append(translation);
    }
    QJsonObject root;
    root.insert(QStringLiteral("errorCode"), QStringLiteral("0"));
    root.insert(QStringLiteral("translation"), array);
    root.insert(QStringLiteral("l"), QStringLiteral("EN2zh-CHS"));
    root.insert(QStringLiteral("requestId"), QStringLiteral("mock-request-id"));
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

}  // namespace

class TranslateYoudaoPluginTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证短文本的签名 input 保持原样。
     * @return 无返回值。
     */
    void keepsShortSignInput()
    {
        QCOMPARE(youdaoSignInput(QStringLiteral("hello")), QStringLiteral("hello"));

        // 恰好 20 个字符时仍不截断
        const QString boundary = QStringLiteral("12345678901234567890");
        QCOMPARE(boundary.length(), 20);
        QCOMPARE(youdaoSignInput(boundary), boundary);
    }

    /**
     * 验证超长文本按首尾各 10 个字符加长度截断。
     * @return 无返回值。
     */
    void truncatesLongSignInput()
    {
        const QString query = QStringLiteral("The quick brown fox jumps over the lazy dog");
        QCOMPARE(query.length(), 43);
        QCOMPARE(youdaoSignInput(query),
                 QStringLiteral("The quick ") + QStringLiteral("43") + query.right(10));

        // 21 个字符即触发截断
        const QString justOver = QStringLiteral("123456789012345678901");
        QCOMPARE(youdaoSignInput(justOver),
                 QStringLiteral("1234567890") + QStringLiteral("21") + QStringLiteral("2345678901"));
    }

    /**
     * 验证多段签名 input 取无分隔符连接后再截断。
     * @return 无返回值。
     */
    void joinsSegmentsWithoutSeparator()
    {
        const QStringList shortQueries{QStringLiteral("hello"), QStringLiteral("world")};
        QCOMPARE(youdaoSignInput(shortQueries), QStringLiteral("helloworld"));

        const QStringList longQueries{QStringLiteral("The quick brown fox "),
                                      QStringLiteral("jumps over the lazy dog")};
        const QString joined = QStringLiteral("The quick brown fox jumps over the lazy dog");
        QCOMPARE(youdaoSignInput(longQueries), youdaoSignInput(joined));
    }

    /**
     * 验证签名为稳定的 64 位小写十六进制且可复算。
     * @return 无返回值。
     */
    void producesStableLowercaseSignature()
    {
        const QString appKey = QStringLiteral("test-app-key");
        const QString signInput = QStringLiteral("hello");
        const QString salt = QStringLiteral("fixed-salt");
        const QString curtime = QStringLiteral("1700000000");
        const QString appSecret = QStringLiteral("test-app-secret");

        const QString signature = youdaoSignature(appKey, signInput, salt, curtime, appSecret);
        QCOMPARE(signature.length(), 64);
        QVERIFY(QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(signature).hasMatch());

        // 同样输入重复计算结果一致
        QCOMPARE(youdaoSignature(appKey, signInput, salt, curtime, appSecret), signature);

        // 与手工拼接后的 SHA256 摘要一致
        const QByteArray digest = QCryptographicHash::hash(
            QStringLiteral("test-app-keyhellofixed-salt1700000000test-app-secret").toUtf8(),
            QCryptographicHash::Sha256);
        QCOMPARE(signature, QString::fromLatin1(digest.toHex()));
    }

    /**
     * 验证规范语言键到有道语言代码的映射。
     * @return 无返回值。
     */
    void mapsLanguageCodes()
    {
        QCOMPARE(youdaoLanguageCode(QStringLiteral("zh-Hans")), QStringLiteral("zh-CHS"));
        QCOMPARE(youdaoLanguageCode(QStringLiteral("zh-Hant")), QStringLiteral("zh-CHT"));
        QCOMPARE(youdaoLanguageCode(QStringLiteral("en")), QStringLiteral("en"));
        QCOMPARE(youdaoLanguageCode(QStringLiteral("ja")), QStringLiteral("ja"));
        QVERIFY(youdaoLanguageCode(QStringLiteral("sv")).isEmpty());
    }

    /**
     * 验证不支持的目标语言直接失败。
     * @return 无返回值。
     */
    void rejectsUnsupportedTargetLanguage()
    {
        MockYoudaoServer server(successResponse({QStringLiteral("你好")}));
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        YoudaoTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}},
                                  QStringLiteral("Klingon"),
                                  &translations,
                                  &error));
        QVERIFY(error.contains(QStringLiteral("Klingon")));
    }

    /**
     * 验证插件可完成一次批量翻译并按 id 回填译文。
     * @return 无返回值。
     */
    void translatesViaMockServer()
    {
        MockYoudaoServer server(successResponse({QStringLiteral("你好"), QStringLiteral("世界")}));
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        YoudaoTranslatePlugin plugin;
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

        const QByteArray body = server.requestBody();
        QVERIFY(body.contains("q=hello"));
        QVERIFY(body.contains("q=world"));
        QVERIFY(body.contains("signType=v3"));
        QVERIFY(body.contains("from=auto"));
        QVERIFY(body.contains("to=zh-CHS"));
        QVERIFY(body.contains("appKey=test-app-key"));

        // 表单内两个同名 q 字段承载批量文本
        const QUrlQuery form(QString::fromUtf8(body));
        const QStringList queries = form.allQueryItemValues(QStringLiteral("q"), QUrl::FullyDecoded);
        QCOMPARE(queries, QStringList({QStringLiteral("hello"), QStringLiteral("world")}));

        // 签名由 appKey、全部 q 的连接串、salt、curtime 与 appSecret 计算得出
        const QString salt = form.queryItemValue(QStringLiteral("salt"), QUrl::FullyDecoded);
        const QString curtime = form.queryItemValue(QStringLiteral("curtime"), QUrl::FullyDecoded);
        QVERIFY(!salt.isEmpty());
        QVERIFY(!curtime.isEmpty());
        QCOMPARE(form.queryItemValue(QStringLiteral("sign"), QUrl::FullyDecoded),
                 youdaoSignature(QStringLiteral("test-app-key"),
                                 youdaoSignInput(queries),
                                 salt,
                                 curtime,
                                 QStringLiteral("test-app-secret")));
    }

    /**
     * 验证业务错误码被识别并写入错误信息。
     * @return 无返回值。
     */
    void reportsBusinessErrorCode()
    {
        MockYoudaoServer server(QByteArrayLiteral(
            "{\"requestId\":\"8fd3fd2c\",\"errorCode\":\"108\",\"l\":\"en2zh-CHS\"}"));
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        YoudaoTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY(error.contains(QStringLiteral("108")));
        QVERIFY(translations.isEmpty());
    }

    /**
     * 验证译文条数与分段数不一致时拒绝回填。
     * @return 无返回值。
     */
    void rejectsTranslationCountMismatch()
    {
        MockYoudaoServer server(successResponse({QStringLiteral("你好")}));
        QVERIFY(server.start());
        std::unique_ptr<QTemporaryFile> config(writeTempConfig(server.endpoint()));
        QVERIFY(config != nullptr);
        EnvGuard configGuard(QByteArrayLiteral("MARK_SHOT_CONFIG"), config->fileName().toUtf8());

        YoudaoTranslatePlugin plugin;
        QVector<markshot::plugin::TranslateSegment> translations;
        QString error;
        QVERIFY(!plugin.translate({{1, QStringLiteral("hello")}, {2, QStringLiteral("world")}},
                                  QStringLiteral("Simplified Chinese"),
                                  &translations,
                                  &error));
        QVERIFY(error.contains(QStringLiteral("1 translations for 2 segments")));
        QVERIFY(translations.isEmpty());
    }

    /**
     * 验证响应解析对空 translation 数组返回失败。
     * @return 无返回值。
     */
    void rejectsEmptyTranslationArray()
    {
        QStringList translations;
        QString error;
        QVERIFY(!parseYoudaoTranslations(QByteArrayLiteral("{\"errorCode\":\"0\",\"translation\":[]}"),
                                         &translations,
                                         &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TranslateYoudaoPluginTest)
#include "translate_youdao_plugin_test.moc"

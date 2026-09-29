#include "providers/translate/translate_trans_task.h"

#include "debug_log.h"

#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>

#include <QRegularExpression>

#include <utility>

namespace markshot::providers {
namespace {

struct TransRunResult {
    bool ok = false;
    QHash<int, QString> translations;
    QString error;
};

QString transExecutable()
{
    const QString discovered = QStandardPaths::findExecutable(QStringLiteral("trans"));
    if (!discovered.isEmpty()) {
        return discovered;
    }
#if defined(Q_OS_MACOS)
    for (const QString &candidate : {QStringLiteral("/opt/homebrew/bin/trans"),
                                     QStringLiteral("/usr/local/bin/trans")}) {
        if (QFileInfo(candidate).isExecutable()) {
            return candidate;
        }
    }
#endif
    return {};
}

QString targetCode(QString language)
{
    const QString normalized = language.trimmed().toLower();
    if (normalized == QStringLiteral("simplified chinese") || normalized == QStringLiteral("zh-cn")
        || normalized == QStringLiteral("zh")) return QStringLiteral("zh");
    if (normalized == QStringLiteral("traditional chinese") || normalized == QStringLiteral("zh-tw")) return QStringLiteral("zh-TW");
    if (normalized == QStringLiteral("english") || normalized == QStringLiteral("en")) return QStringLiteral("en");
    if (normalized == QStringLiteral("japanese") || normalized == QStringLiteral("ja")) return QStringLiteral("ja");
    if (normalized == QStringLiteral("korean") || normalized == QStringLiteral("ko")) return QStringLiteral("ko");
    if (normalized == QStringLiteral("french") || normalized == QStringLiteral("fr")) return QStringLiteral("fr");
    if (normalized == QStringLiteral("german") || normalized == QStringLiteral("de")) return QStringLiteral("de");
    if (normalized == QStringLiteral("spanish") || normalized == QStringLiteral("es")) return QStringLiteral("es");
    if (normalized == QStringLiteral("russian") || normalized == QStringLiteral("ru")) return QStringLiteral("ru");
    return language.trimmed();
}

QProcessEnvironment proxyEnvironment()
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    // GUI-launched apps can expose only an upper/lower-case variant. Keep both
    // forms so curl/libproxy used by trans sees the configured proxy reliably.
    for (const QString &base : {QStringLiteral("http_proxy"),
                                QStringLiteral("https_proxy"),
                                QStringLiteral("all_proxy"),
                                QStringLiteral("no_proxy")}) {
        const QString upper = base.toUpper();
        const QString lowerValue = environment.value(base);
        const QString upperValue = environment.value(upper);
        if (lowerValue.isEmpty() && !upperValue.isEmpty()) environment.insert(base, upperValue);
        if (upperValue.isEmpty() && !lowerValue.isEmpty()) environment.insert(upper, lowerValue);
    }
#if defined(Q_OS_MACOS)
    // Applications started from Finder do not inherit the shell's proxy
    // variables. Read the active macOS proxy once and expose it to trans.
    QProcess scutil;
    scutil.start(QStringLiteral("/usr/sbin/scutil"), {QStringLiteral("--proxy")});
    if (scutil.waitForFinished(1000)) {
        const QString proxy = QString::fromUtf8(scutil.readAllStandardOutput());
        const auto value = [&proxy](const QString &key) {
            const QRegularExpression expression(
                QStringLiteral("^\\s*%1\\s*:\\s*(.+)$").arg(QRegularExpression::escape(key)),
                QRegularExpression::MultilineOption);
            const QRegularExpressionMatch match = expression.match(proxy);
            return match.hasMatch() ? match.captured(1).trimmed() : QString();
        };
        const auto enabled = [&value](const QString &key) { return value(key) == QStringLiteral("1"); };
        const QString httpHost = value(QStringLiteral("HTTPProxy"));
        const QString httpPort = value(QStringLiteral("HTTPPort"));
        const QString httpsHost = value(QStringLiteral("HTTPSProxy"));
        const QString httpsPort = value(QStringLiteral("HTTPSPort"));
        const QString socksHost = value(QStringLiteral("SOCKSProxy"));
        const QString socksPort = value(QStringLiteral("SOCKSPort"));
        if (environment.value(QStringLiteral("http_proxy")).isEmpty() && enabled(QStringLiteral("HTTPEnable"))
            && !httpHost.isEmpty() && !httpPort.isEmpty()) {
            environment.insert(QStringLiteral("http_proxy"), QStringLiteral("http://%1:%2").arg(httpHost, httpPort));
        }
        if (environment.value(QStringLiteral("https_proxy")).isEmpty() && enabled(QStringLiteral("HTTPSEnable"))
            && !httpsHost.isEmpty() && !httpsPort.isEmpty()) {
            environment.insert(QStringLiteral("https_proxy"), QStringLiteral("http://%1:%2").arg(httpsHost, httpsPort));
        }
        if (environment.value(QStringLiteral("all_proxy")).isEmpty() && enabled(QStringLiteral("SOCKSEnable"))
            && !socksHost.isEmpty() && !socksPort.isEmpty()) {
            environment.insert(QStringLiteral("all_proxy"), QStringLiteral("socks5://%1:%2").arg(socksHost, socksPort));
        }
        for (const QString &base : {QStringLiteral("http_proxy"), QStringLiteral("https_proxy"), QStringLiteral("all_proxy")}) {
            const QString upper = base.toUpper();
            if (environment.value(upper).isEmpty() && !environment.value(base).isEmpty()) {
                environment.insert(upper, environment.value(base));
            }
        }
    }
#endif
    return environment;
}

TransRunResult runTrans(const QVector<TranslateSourceSegment> &segments,
                        const QString &language,
                        int timeoutMs)
{
    const QString executable = transExecutable();
    if (executable.isEmpty()) return {false, {}, QStringLiteral("trans executable not found")};
    TransRunResult result;
    // An empty source before ':' asks translate-shell to detect the source;
    // using the literal "auto" is interpreted as a language code by some engines.
    const QString route = QStringLiteral(":%1").arg(targetCode(language));
    for (const TranslateSourceSegment &segment : segments) {
        QProcess process;
        process.setProcessEnvironment(proxyEnvironment());
        process.setProgram(executable);
        process.setArguments({QStringLiteral("-no-ansi"), QStringLiteral("-brief"),
                              QStringLiteral("-no-warn"), route, segment.text});
        process.start();
        if (!process.waitForStarted(5000)) return {false, {}, QStringLiteral("trans failed to start: %1").arg(process.errorString())};
        const int processTimeout = timeoutMs > 0 ? timeoutMs : 60000;
        if (!process.waitForFinished(processTimeout)) {
            process.kill();
            process.waitForFinished(1000);
            return {false, {}, QStringLiteral("trans timed out")};
        }
        QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        // translate-shell may prefix auto-detection with a line such as
        // "car: zh" even in brief mode; keep only the translated text.
        const QStringList lines = output.split(QRegularExpression(QStringLiteral("\\r?\\n")));
        if (lines.size() > 1
            && QRegularExpression(QStringLiteral("^[A-Za-z_-]+\\s*:\\s*[A-Za-z_-]+$"))
                   .match(lines.first().trimmed())
                   .hasMatch()) {
            output = lines.mid(1).join(QLatin1Char('\n')).trimmed();
        }
        const QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || output.isEmpty()) {
            return {false, {}, error.isEmpty() ? QStringLiteral("trans returned no translation") : error.left(500)};
        }
        result.translations.insert(segment.id, output);
    }
    result.ok = true;
    return result;
}

}  // namespace

TranslateTransTask::TranslateTransTask(QByteArray inputJson,
                                       QString targetLanguage,
                                       QObject *parent)
    : ProviderTask(QStringLiteral("trans (translate-shell)"), parent)
    , m_inputJson(std::move(inputJson))
    , m_targetLanguage(std::move(targetLanguage))
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this] {
        m_watcher.disconnect(this);
        emitFinished({false, TaskError::Timeout, {}, QByteArrayLiteral("trans timed out"), {}});
    });
    connect(&m_watcher, &QFutureWatcher<TaskResult>::finished, this, [this] {
        m_timeoutTimer.stop();
        emitFinished(m_watcher.result());
    });
}

bool TranslateTransTask::available()
{
    return !transExecutable().isEmpty();
}

void TranslateTransTask::start(int timeoutMs)
{
    QString inputLanguage;
    const QVector<TranslateSourceSegment> segments = translateSegmentsFromInputJson(m_inputJson, &inputLanguage);
    if (segments.isEmpty()) {
        emitFinished({false, TaskError::Failed, {}, QByteArrayLiteral("no source text"), {}});
        return;
    }
    if (m_targetLanguage.trimmed().isEmpty()) m_targetLanguage = inputLanguage.isEmpty() ? QStringLiteral("Simplified Chinese") : inputLanguage;
    if (timeoutMs > 0) m_timeoutTimer.start(timeoutMs);
    const QString targetLanguage = m_targetLanguage;
    m_watcher.setFuture(QtConcurrent::run([segments, targetLanguage, timeoutMs] {
        const TransRunResult run = runTrans(segments, targetLanguage, timeoutMs);
        if (!run.ok) {
            debugLog("translation", "trans fallback failed: %s", run.error.toUtf8().constData());
            return TaskResult{false, TaskError::Failed, {}, run.error.toUtf8(), {}};
        }
        return TaskResult{true, TaskError::None,
                          translateTokensJson(segments, run.translations, QStringLiteral("trans")), {}, {}};
    }));
}

void TranslateTransTask::cancel()
{
    m_timeoutTimer.stop();
    m_watcher.disconnect(this);
}

}  // namespace markshot::providers

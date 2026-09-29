#include "providers/translate/translate_trans_task.h"

#include <QtTest/QtTest>

class TranslateTransTaskTest final : public QObject {
    Q_OBJECT

private slots:
    void translatesThroughSystemTrans()
    {
        if (!markshot::providers::TranslateTransTask::available()) {
            QSKIP("translate-shell (trans) is not installed");
        }
        const QByteArray input = QByteArrayLiteral(
            "{\"targetLanguage\":\"Simplified Chinese\",\"tokens\":["
            "{\"text\":\"screenshot window\",\"box\":[0,0,100,20],\"line\":0,\"index\":0}]}");
        markshot::providers::TranslateTransTask task(input, QStringLiteral("Simplified Chinese"));
        task.start(30000);
        const auto result = task.waitForResult();
        QVERIFY2(result.ok, result.errorOutput.constData());
        QVERIFY2(result.output.contains(QStringLiteral("截图窗口").toUtf8()), result.output.constData());
    }
};

QTEST_GUILESS_MAIN(TranslateTransTaskTest)
#include "translate_trans_task_test.moc"

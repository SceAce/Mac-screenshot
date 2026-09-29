#include "providers/ocr/ocr_provider_factory.h"
#include "providers/ocr/ocr_tesseract_task.h"
#if defined(Q_OS_MACOS)
#include "providers/ocr/ocr_vision_task.h"
#endif
#include "providers/provider_task.h"

#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTemporaryDir>
#include <QTemporaryFile>

using namespace markshot::providers;

class OcrProviderFactoryTest : public QObject {
    Q_OBJECT

private slots:
    /**
     * 验证 tesseract TSV 输出转换为标准 tokens JSON。
     * @return 无返回值。
     */
    void convertsTsvToTokensJson()
    {
        const QString tsv = QStringLiteral(
            "level\tpage_num\tblock_num\tpar_num\tline_num\tword_num\tleft\ttop\twidth\theight\tconf\ttext\n"
            "1\t1\t0\t0\t0\t0\t0\t0\t100\t50\t-1\t\n"
            "5\t1\t1\t1\t1\t1\t10\t5\t40\t12\t95.5\tHello\n"
            "5\t1\t1\t1\t1\t2\t55\t5\t40\t12\t90.0\tworld\n"
            "5\t1\t1\t1\t2\t1\t10\t25\t60\t12\t88.0\tsecond\n");

        const QJsonObject root =
            QJsonDocument::fromJson(OcrTesseractTask::tsvToTokensJson(tsv)).object();
        QCOMPARE(root.value(QStringLiteral("backend")).toString(), QStringLiteral("tesseract"));

        const QJsonArray tokens = root.value(QStringLiteral("tokens")).toArray();
        QCOMPARE(tokens.size(), 3);
        const QJsonObject first = tokens.at(0).toObject();
        QCOMPARE(first.value(QStringLiteral("text")).toString(), QStringLiteral("Hello"));
        QCOMPARE(first.value(QStringLiteral("line")).toInt(), 0);
        QCOMPARE(first.value(QStringLiteral("index")).toInt(), 0);
        const QJsonObject second = tokens.at(1).toObject();
        QCOMPARE(second.value(QStringLiteral("index")).toInt(), 1);
        const QJsonObject third = tokens.at(2).toObject();
        QCOMPARE(third.value(QStringLiteral("line")).toInt(), 1);
        QCOMPARE(third.value(QStringLiteral("index")).toInt(), 0);
    }

    /**
     * 验证显式 provider 偏好的解析结果。
     * @return 无返回值。
     */
    void resolvesExplicitProviders()
    {
        OcrTaskRequest request;
        request.commandLine = QStringLiteral("my-ocr {image}");
        QCOMPARE(resolvedOcrProviderName(request), QStringLiteral("custom command"));

        request.commandLine.clear();
        request.provider = QStringLiteral("helper");
        QCOMPARE(resolvedOcrProviderName(request), QStringLiteral("helper (mark-shot-ocr)"));

        request.provider = QStringLiteral("builtin");
#if defined(Q_OS_MACOS)
        QCOMPARE(resolvedOcrProviderName(request), QStringLiteral("builtin (Apple Vision)"));
#else
        QCOMPARE(resolvedOcrProviderName(request), QStringLiteral("builtin (tesseract)"));
#endif
    }

    /**
     * 验证旧 venv 存在时 auto 链保持 helper。
     * @return 无返回值。
     */
    void autoPrefersLegacyVenv()
    {
        QTemporaryFile pythonStub;
        QVERIFY(pythonStub.open());
        qputenv("MARK_SHOT_OCR_PYTHON", pythonStub.fileName().toUtf8());

        OcrTaskRequest request;
        request.provider = QStringLiteral("auto");
        QCOMPARE(resolvedOcrProviderName(request), QStringLiteral("helper (legacy venv)"));

        qunsetenv("MARK_SHOT_OCR_PYTHON");
    }

    /**
     * 验证显式 builtin 请求创建当前平台的内置 OCR 任务。
     * @return 无返回值。
     */
    void createsBuiltinTask()
    {
        OcrTaskRequest request;
        request.provider = QStringLiteral("builtin");
        request.imagePath = QStringLiteral("/nonexistent.png");
        ProviderTask *task = createOcrTask(request);
#if defined(Q_OS_MACOS)
        QVERIFY(qobject_cast<OcrVisionTask *>(task) != nullptr);
#else
        QVERIFY(qobject_cast<OcrTesseractTask *>(task) != nullptr);
#endif
        delete task;
    }

#if defined(Q_OS_MACOS)
    /**
     * 验证 Apple Vision 能对生成的清晰文字图片返回标准 token JSON。
     * @return 无返回值。
     */
    void visionRecognizesGeneratedText()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString imagePath = directory.filePath(QStringLiteral("vision-ocr.png"));

        QImage image(1200, 300, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        QPainter painter(&image);
        QFont font(QStringLiteral("Helvetica"), 96, QFont::Bold);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(image.rect(), Qt::AlignCenter, QStringLiteral("MARK SHOT 123"));
        painter.end();
        QVERIFY(image.save(imagePath));

        OcrVisionTask task(imagePath);
        // Vision may need a longer one-time initialization on a cold macOS session.
        task.start(120000);
        const TaskResult result = task.waitForResult();
        QVERIFY2(result.ok, result.errorOutput.constData());

        const QJsonObject root = QJsonDocument::fromJson(result.output).object();
        QCOMPARE(root.value(QStringLiteral("backend")).toString(), QStringLiteral("apple-vision"));
        const QJsonArray tokens = root.value(QStringLiteral("tokens")).toArray();
        QVERIFY(!tokens.isEmpty());
        QVERIFY(!tokens.first().toObject().value(QStringLiteral("text")).toString().isEmpty());
    }
#endif
};

QTEST_MAIN(OcrProviderFactoryTest)
#include "ocr_provider_factory_test.moc"

#import <CoreGraphics/CoreGraphics.h>
#import <Vision/Vision.h>

#include "providers/ocr/ocr_vision_task.h"

#include <QImage>
#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace markshot::providers {
namespace {

TaskResult recognizeWithVision(const QString &imagePath)
{
    @autoreleasepool {
        QImage image(imagePath);
        if (image.isNull()) {
            return {false, TaskError::Failed, {}, QByteArrayLiteral("cannot load image for Apple Vision OCR"), {}};
        }
        image = image.convertToFormat(QImage::Format_RGBA8888);

        VNRecognizeTextRequest *request = [[VNRecognizeTextRequest alloc] init];
        request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        request.usesLanguageCorrection = YES;
        if (@available(macOS 13.0, *)) {
            request.automaticallyDetectsLanguage = YES;
        }

        QByteArray encodedImage;
        QBuffer encodedBuffer(&encodedImage);
        encodedBuffer.open(QIODevice::WriteOnly);
        image.save(&encodedBuffer, "PNG");
        NSData *visionImageData = [NSData dataWithBytes:encodedImage.constData()
                                                     length:static_cast<NSUInteger>(encodedImage.size())];
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithData:visionImageData options:@{}];
        NSError *error = nil;
        const BOOL succeeded = [handler performRequests:@[request] error:&error];
        if (!succeeded) {
            const QString message = error
                ? QString::fromNSString(error.localizedDescription)
                : QStringLiteral("Apple Vision OCR failed");
            return {false, TaskError::Failed, {}, message.toUtf8(), {}};
        }

        struct RecognizedLine {
            QString text;
            QRectF rect;
            qreal confidence = 0.0;
        };
        QVector<RecognizedLine> lines;
        for (VNRecognizedTextObservation *observation in request.results) {
            VNRecognizedText *candidate = [[observation topCandidates:1] firstObject];
            if (!candidate || candidate.string.length == 0) {
                continue;
            }
            const CGRect normalized = observation.boundingBox;
            const QRectF rect(normalized.origin.x * image.width(),
                              (1.0 - CGRectGetMaxY(normalized)) * image.height(),
                              normalized.size.width * image.width(),
                              normalized.size.height * image.height());
            if (rect.width() <= 0.0 || rect.height() <= 0.0) {
                continue;
            }
            lines.append({QString::fromNSString(candidate.string), rect, candidate.confidence});
        }

        std::stable_sort(lines.begin(), lines.end(), [](const RecognizedLine &left,
                                                        const RecognizedLine &right) {
            const qreal tolerance = std::max(left.rect.height(), right.rect.height()) * 0.5;
            if (qAbs(left.rect.top() - right.rect.top()) > tolerance) {
                return left.rect.top() < right.rect.top();
            }
            return left.rect.left() < right.rect.left();
        });

        QJsonArray tokens;
        for (int line = 0; line < lines.size(); ++line) {
            const RecognizedLine &recognized = lines.at(line);
            tokens.append(QJsonObject{
                {QStringLiteral("text"), recognized.text},
                {QStringLiteral("box"),
                 QJsonArray{recognized.rect.x(),
                            recognized.rect.y(),
                            recognized.rect.width(),
                            recognized.rect.height()}},
                {QStringLiteral("line"), line},
                {QStringLiteral("index"), 0},
                {QStringLiteral("confidence"), recognized.confidence},
            });
        }
        const QJsonObject root{{QStringLiteral("backend"), QStringLiteral("apple-vision")},
                               {QStringLiteral("tokens"), tokens},
                               {QStringLiteral("errors"), QJsonArray()}};
        return {true,
                TaskError::None,
                QJsonDocument(root).toJson(QJsonDocument::Compact),
                {},
                {}};
    }
}

} // namespace

OcrVisionTask::OcrVisionTask(QString imagePath, QObject *parent)
    : ProviderTask(QStringLiteral("Apple Vision"), parent)
    , m_imagePath(std::move(imagePath))
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this] {
        m_watcher.disconnect(this);
        emitFinished({false, TaskError::Timeout, {}, {}, {}});
    });
    connect(&m_watcher, &QFutureWatcher<TaskResult>::finished, this, [this] {
        m_timeoutTimer.stop();
        emitFinished(m_watcher.result());
    });
}

bool OcrVisionTask::available()
{
    if (@available(macOS 10.15, *)) {
        return true;
    }
    return false;
}

void OcrVisionTask::start(int timeoutMs)
{
    if (!available()) {
        emitFinished({false, TaskError::StartFailed, {}, QByteArrayLiteral("Apple Vision OCR is unavailable"), {}});
        return;
    }
    if (timeoutMs > 0) {
        m_timeoutTimer.start(timeoutMs);
    }
    const QString imagePath = m_imagePath;
    m_watcher.setFuture(QtConcurrent::run([imagePath] { return recognizeWithVision(imagePath); }));
}

void OcrVisionTask::cancel()
{
    m_timeoutTimer.stop();
    m_watcher.disconnect(this);
}

} // namespace markshot::providers

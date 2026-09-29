#pragma once

#include "providers/provider_task.h"

#include <QFutureWatcher>
#include <QTimer>

namespace markshot::providers {

/**
 * Built-in macOS OCR powered by Apple's Vision framework.
 *
 * Recognition runs on a worker thread and emits the same token JSON contract
 * used by the plugin, Tesseract, and helper backends.
 */
class OcrVisionTask final : public ProviderTask {
    Q_OBJECT

public:
    explicit OcrVisionTask(QString imagePath, QObject *parent = nullptr);

    void start(int timeoutMs) override;
    void cancel() override;

    static bool available();

private:
    QString m_imagePath;
    QFutureWatcher<TaskResult> m_watcher;
    QTimer m_timeoutTimer;
};

} // namespace markshot::providers

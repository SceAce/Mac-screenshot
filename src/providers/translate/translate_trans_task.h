#pragma once

#include "providers/provider_task.h"
#include "providers/translate/translate_segments.h"

#include <QFutureWatcher>
#include <QTimer>

namespace markshot::providers {

/**
 * translate-shell (`trans`) based translation task.
 *
 * This is the no-API-key fallback for auto provider selection. The child
 * process receives the application's environment, including proxy variables.
 */
class TranslateTransTask final : public ProviderTask {
    Q_OBJECT

public:
    TranslateTransTask(QByteArray inputJson,
                       QString targetLanguage,
                       QObject *parent = nullptr);

    void start(int timeoutMs) override;
    void cancel() override;

    static bool available();

private:
    QByteArray m_inputJson;
    QString m_targetLanguage;
    QFutureWatcher<TaskResult> m_watcher;
    QTimer m_timeoutTimer;
};

}  // namespace markshot::providers

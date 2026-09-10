#pragma once

#include "display_capture/display_capture_target.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QKeyEvent;
class QVBoxLayout;

namespace markshot::display_capture {

class DisplayCapturePicker final : public QWidget {
    Q_OBJECT

public:
    explicit DisplayCapturePicker(QWidget *parent = nullptr);
    void setTargets(const QVector<Target> &targets);

signals:
    void copyRequested(int index);
    void editRequested(int index);
    void saveRequested(int index);
    void dismissed();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QVBoxLayout *m_cardLayout = nullptr;
};

}  // namespace markshot::display_capture

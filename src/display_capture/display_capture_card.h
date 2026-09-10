#pragma once

#include "display_capture/display_capture_target.h"

#include <QFrame>

class QLabel;
class QPushButton;
class QEnterEvent;
class QResizeEvent;

namespace markshot::display_capture {

class DisplayCaptureCard final : public QFrame {
    Q_OBJECT

public:
    explicit DisplayCaptureCard(int index, const Target &target, QWidget *parent = nullptr);

signals:
    void copyRequested(int index);
    void editRequested(int index);
    void saveRequested(int index);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateActionBarGeometry();
    QPushButton *createActionButton(const QString &text);

    int m_index = -1;
    QFrame *m_actionBar = nullptr;
};

}  // namespace markshot::display_capture

#include "display_capture/display_capture_picker.h"

#include "display_capture/display_capture_card.h"
#include "ui/i18n.h"
#include "ui/theme.h"

#include <QFont>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

namespace markshot::display_capture {
namespace {

/**
 * 移除布局中的所有子项。
 * @param layout 需要清空的布局。
 * @return 无返回值。
 */
void clearLayout(QLayout *layout)
{
    if (!layout) {
        return;
    }
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

}  // namespace

/**
 * 创建显示器快速截取选择器。
 * @param parent 父级窗口。
 */
DisplayCapturePicker::DisplayCapturePicker(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("displayCapturePicker"));
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral(
        "QWidget#displayCapturePicker {"
        " background: rgba(226, 232, 240, 235);"
        " border: 1px solid rgba(15, 23, 42, 45);"
        " border-radius: 18px;"
        "}"
        "QFrame#displayCaptureCard {"
        " background: rgba(248, 250, 252, 210);"
        " border: 1px solid rgba(15, 23, 42, 18);"
        " border-radius: 12px;"
        "}"
        "QFrame#displayCaptureCard:hover {"
        " background: rgba(255, 255, 255, 238);"
        " border-color: rgba(20, 184, 166, 130);"
        "}"
        "QFrame#displayCaptureActionBar {"
        " background: rgba(8, 13, 19, 210);"
        " border: 1px solid rgba(255, 255, 255, 34);"
        " border-radius: 12px;"
        "}"
        "QPushButton#displayCaptureActionButton {"
        " background: rgba(248, 250, 252, 232);"
        " border: none;"
        " border-radius: 8px;"
        " color: #0F172A;"
        " font-weight: 800;"
        " padding: 4px 8px;"
        "}"
        "QPushButton#displayCaptureActionButton:hover {"
        " background: rgba(204, 251, 241, 245);"
        " color: #0F766E;"
        "}"
        "QLabel#displayCaptureTitle {"
        " color: #0F172A;"
        " font-weight: 800;"
        "}"
        "QLabel#displayCaptureSubtitle {"
        " color: #334155;"
        " font-weight: 600;"
        "}"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);
    m_cardLayout = rootLayout;
}

/**
 * 设置显示器捕获目标。
 * @param targets 可选择的显示器目标列表。
 * @return 无返回值。
 */
void DisplayCapturePicker::setTargets(const QVector<Target> &targets)
{
    clearLayout(m_cardLayout);

    for (int i = 0; i < targets.size(); ++i) {
        auto *card = new DisplayCaptureCard(i, targets.at(i), this);
        connect(card, &DisplayCaptureCard::copyRequested, this, &DisplayCapturePicker::copyRequested);
        connect(card, &DisplayCaptureCard::editRequested, this, &DisplayCapturePicker::editRequested);
        connect(card, &DisplayCaptureCard::saveRequested, this, &DisplayCapturePicker::saveRequested);
        m_cardLayout->addWidget(card);
    }

    if (targets.isEmpty()) {
        auto *empty = new QLabel(MS_TR("No displays found"), this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
        m_cardLayout->addWidget(empty);
    }

    adjustSize();
}

/**
 * 处理选择器键盘输入。
 * @param event 键盘事件。
 * @return 无返回值。
 */
void DisplayCapturePicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit dismissed();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

}  // namespace markshot::display_capture

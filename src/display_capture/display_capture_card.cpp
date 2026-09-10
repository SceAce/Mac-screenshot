#include "display_capture/display_capture_card.h"

#include "ui/i18n.h"
#include "ui/theme.h"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QPixmap>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace markshot::display_capture {
namespace {

/**
 * 生成显示器卡片缩略图。
 * @param image 捕获到的屏幕缩略图。
 * @param targetSize 缩略图目标尺寸。
 * @return 可用于 QLabel 的像素图。
 */
QPixmap thumbnailPixmap(const QImage &image, QSize targetSize)
{
    QPixmap pixmap(targetSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(148, 163, 184, 80));
    painter.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(targetSize)), 6.0, 6.0);

    if (!image.isNull()) {
        const QImage scaled = image.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const QRect source((scaled.width() - targetSize.width()) / 2,
                           (scaled.height() - targetSize.height()) / 2,
                           targetSize.width(),
                           targetSize.height());
        painter.drawImage(QRect(QPoint(0, 0), targetSize), scaled.copy(source));
    }

    painter.setPen(QPen(QColor(255, 255, 255, 55), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(0.5, 0.5, targetSize.width() - 1.0, targetSize.height() - 1.0), 6.0, 6.0);
    return pixmap;
}

}  // namespace

/**
 * 创建显示器快照卡片。
 * @param index 目标在选择器列表中的索引。
 * @param target 显示器截取目标。
 * @param parent 父级窗口。
 */
DisplayCaptureCard::DisplayCaptureCard(int index, const Target &target, QWidget *parent)
    : QFrame(parent)
    , m_index(index)
{
    setObjectName(QStringLiteral("displayCaptureCard"));
    setMouseTracking(true);
    setMinimumHeight(96);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *cardLayout = new QHBoxLayout(this);
    cardLayout->setContentsMargins(10, 8, 12, 8);
    cardLayout->setSpacing(12);

    auto *thumbnail = new QLabel(this);
    thumbnail->setFixedSize(126, 72);
    thumbnail->setPixmap(thumbnailPixmap(target.thumbnail.isNull() ? target.image : target.thumbnail,
                                         thumbnail->size()));
    thumbnail->setScaledContents(false);
    cardLayout->addWidget(thumbnail);

    auto *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(6);
    auto *title = new QLabel(target.title, this);
    title->setObjectName(QStringLiteral("displayCaptureTitle"));
    title->setFont(markshot::theme::uiFont(11, QFont::DemiBold));
    auto *subtitle = new QLabel(target.subtitle, this);
    subtitle->setObjectName(QStringLiteral("displayCaptureSubtitle"));
    subtitle->setFont(markshot::theme::uiFont(9, QFont::Medium));
    textLayout->addStretch(1);
    textLayout->addWidget(title);
    textLayout->addWidget(subtitle);
    textLayout->addStretch(1);
    cardLayout->addLayout(textLayout, 1);

    m_actionBar = new QFrame(this);
    m_actionBar->setObjectName(QStringLiteral("displayCaptureActionBar"));
    m_actionBar->setVisible(false);
    auto *actionLayout = new QHBoxLayout(m_actionBar);
    actionLayout->setContentsMargins(6, 5, 6, 5);
    actionLayout->setSpacing(6);

    QPushButton *copyButton = createActionButton(MS_TR("Copy"));
    QPushButton *editButton = createActionButton(MS_TR("Edit"));
    QPushButton *saveButton = createActionButton(MS_TR("Save"));
    actionLayout->addWidget(copyButton);
    actionLayout->addWidget(editButton);
    actionLayout->addWidget(saveButton);

    connect(copyButton, &QPushButton::clicked, this, [this] { emit copyRequested(m_index); });
    connect(editButton, &QPushButton::clicked, this, [this] { emit editRequested(m_index); });
    connect(saveButton, &QPushButton::clicked, this, [this] { emit saveRequested(m_index); });
}

/**
 * 处理鼠标进入卡片事件。
 * @param event 鼠标进入事件。
 * @return 无返回值。
 */
void DisplayCaptureCard::enterEvent(QEnterEvent *event)
{
    if (m_actionBar) {
        m_actionBar->show();
        m_actionBar->raise();
    }
    QFrame::enterEvent(event);
}

/**
 * 处理鼠标离开卡片事件。
 * @param event 鼠标离开事件。
 * @return 无返回值。
 */
void DisplayCaptureCard::leaveEvent(QEvent *event)
{
    if (m_actionBar) {
        m_actionBar->hide();
    }
    QFrame::leaveEvent(event);
}

/**
 * 处理卡片尺寸变化事件。
 * @param event 尺寸变化事件。
 * @return 无返回值。
 */
void DisplayCaptureCard::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    updateActionBarGeometry();
}

/**
 * 更新悬浮操作栏位置。
 * @return 无返回值。
 */
void DisplayCaptureCard::updateActionBarGeometry()
{
    if (!m_actionBar) {
        return;
    }
    m_actionBar->adjustSize();
    const QSize barSize = m_actionBar->sizeHint();
    m_actionBar->setGeometry(width() - barSize.width() - 12,
                             height() - barSize.height() - 10,
                             barSize.width(),
                             barSize.height());
}

/**
 * 创建卡片悬浮操作按钮。
 * @param text 按钮文本。
 * @return 按钮实例。
 */
QPushButton *DisplayCaptureCard::createActionButton(const QString &text)
{
    auto *button = new QPushButton(text, m_actionBar);
    button->setObjectName(QStringLiteral("displayCaptureActionButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMinimumWidth(58);
    button->setMinimumHeight(28);
    return button;
}

}  // namespace markshot::display_capture

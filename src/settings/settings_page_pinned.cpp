#include "settings/settings_page_pinned.h"

#include "settings/settings_ui_helpers.h"
#include "ui/i18n.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace markshot::settings {

SettingsPagePinned::SettingsPagePinned(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = createSettingsPageLayout(this);

    QFrame *windowCard = createSettingsCard(MS_TR("Pinned Image"),
                                            MS_TR("Control pinned image window behavior."),
                                            this);
    QFormLayout *windowForm = settingsCardForm(windowCard);
    m_alwaysOnTop = addSwitchRow(windowForm,
                                 MS_TR("Always on Top"),
                                 MS_TR("Keep pinned images above normal windows when the platform supports it."));
    m_textSelectionCopy = addSwitchRow(windowForm,
                                       MS_TR("Text Selection Copy"),
                                       MS_TR("Allow selecting OCR or translated text with the mouse."));
    m_borderEnabled = addSwitchRow(windowForm,
                                   MS_TR("Pinned Border"),
                                   MS_TR("Draw a border around pinned images."));
    m_borderColor = new QPushButton;
    m_borderColor->setCursor(Qt::PointingHandCursor);
    windowForm->addRow(MS_TR("Border Color"), m_borderColor);
    connect(m_borderColor, &QPushButton::clicked, this, [this] {
        const QColor selected = QColorDialog::getColor(m_borderColorValue, this, MS_TR("Border Color"));
        if (!selected.isValid()) {
            return;
        }
        m_borderColorValue = selected;
        updateBorderColorButton();
    });
    m_borderWidth = addDoubleRow(windowForm, MS_TR("Border Width"), 1.0, 12.0, 1);
    m_borderWidth->setSuffix(QStringLiteral(" px"));
    layout->addWidget(windowCard);

    QFrame *ocrCard = createSettingsCard(MS_TR("OCR and Translation"),
                                         MS_TR("Configure automatic text recognition for pinned images."),
                                         this);
    QFormLayout *ocrForm = settingsCardForm(ocrCard);
    m_ocrEnabled = addSwitchRow(ocrForm, MS_TR("OCR Enabled"), MS_TR("Enable OCR actions for pinned images."));
    m_autoOcr = addSwitchRow(ocrForm, MS_TR("Auto OCR"), MS_TR("Recognize text automatically after pinning."));
    m_ocrBackend = addTextRow(ocrForm, MS_TR("OCR Backend"), QStringLiteral("rapidocr"));
    m_ocrCommand = addTextRow(ocrForm, MS_TR("OCR Command"), QStringLiteral("mark-shot-ocr {image}"));
    m_ocrTimeoutMs = addSpinRow(ocrForm, MS_TR("OCR Timeout"), 1000, 300000, QStringLiteral(" ms"));
    m_autoTranslate = addSwitchRow(ocrForm,
                                   MS_TR("Auto Translate"),
                                   MS_TR("Translate automatically after OCR completes."));
    m_targetLanguage = addTextRow(ocrForm, MS_TR("Target Language"), MS_TR("Simplified Chinese"));
    m_translationCommand = addTextRow(ocrForm, MS_TR("Translation Command"), QStringLiteral("mark-shot-translate {input}"));
    m_translationTimeoutMs = addSpinRow(ocrForm, MS_TR("Translation Timeout"), 1000, 300000, QStringLiteral(" ms"));
    layout->addWidget(ocrCard);
    layout->addStretch();
}

void SettingsPagePinned::setConfig(const SettingsConfig &config)
{
    m_alwaysOnTop->setChecked(config.pinned.alwaysOnTop);
    m_textSelectionCopy->setChecked(config.pinned.textSelectionCopyEnabled);
    m_borderEnabled->setChecked(config.pinned.borderEnabled);
    m_borderColorValue = config.pinned.borderColor;
    updateBorderColorButton();
    m_borderWidth->setValue(config.pinned.borderWidth);
    m_ocrEnabled->setChecked(config.pinned.ocrEnabled);
    m_autoOcr->setChecked(config.pinned.autoOcr);
    m_ocrBackend->setText(config.pinned.ocrBackend);
    m_ocrCommand->setText(config.pinned.ocrCommand);
    m_ocrTimeoutMs->setValue(config.pinned.ocrTimeoutMs);
    m_autoTranslate->setChecked(config.pinned.autoTranslateAfterOcr);
    m_targetLanguage->setText(config.pinned.translationTargetLanguage);
    m_translationCommand->setText(config.pinned.translationCommand);
    m_translationTimeoutMs->setValue(config.pinned.translationTimeoutMs);
}

void SettingsPagePinned::updateConfig(SettingsConfig *config) const
{
    if (!config) {
        return;
    }

    config->pinned.alwaysOnTop = m_alwaysOnTop->isChecked();
    config->pinned.textSelectionCopyEnabled = m_textSelectionCopy->isChecked();
    config->pinned.borderEnabled = m_borderEnabled->isChecked();
    config->pinned.borderColor = m_borderColorValue;
    config->pinned.borderWidth = m_borderWidth->value();
    config->pinned.ocrEnabled = m_ocrEnabled->isChecked();
    config->pinned.autoOcr = m_autoOcr->isChecked();
    config->pinned.ocrBackend = m_ocrBackend->text().trimmed();
    config->pinned.ocrCommand = m_ocrCommand->text().trimmed();
    config->pinned.ocrTimeoutMs = m_ocrTimeoutMs->value();
    config->pinned.autoTranslateAfterOcr = m_autoTranslate->isChecked();
    config->pinned.translationTargetLanguage = m_targetLanguage->text().trimmed();
    config->pinned.translationCommand = m_translationCommand->text().trimmed();
    config->pinned.translationTimeoutMs = m_translationTimeoutMs->value();
}

void SettingsPagePinned::updateBorderColorButton()
{
    const QString colorName = m_borderColorValue.isValid()
        ? m_borderColorValue.name(QColor::HexRgb).toUpper()
        : QStringLiteral("#5EEAD4");
    m_borderColor->setText(colorName);
    m_borderColor->setStyleSheet(colorButtonStyleSheet(m_borderColorValue));
}

}  // namespace markshot::settings

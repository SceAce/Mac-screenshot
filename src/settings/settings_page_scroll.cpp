#include "settings/settings_page_scroll.h"

#include "settings/settings_ui_helpers.h"
#include "ui/i18n.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QSpinBox>
#include <QVBoxLayout>

namespace markshot::settings {

SettingsPageScroll::SettingsPageScroll(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = createSettingsPageLayout(this);

    QFrame *scrollCard = createSettingsCard(MS_TR("Scroll Capture"),
                                            MS_TR("Configure the floating frame and preview panel used for scrolling screenshots."),
                                            this);
    QFormLayout *form = settingsCardForm(scrollCard);
    m_frameEnabled = addSwitchRow(form,
                                  MS_TR("Capture Frame"),
                                  MS_TR("Show an outer frame around the scrolling capture region."));
    m_frameGap = addSpinRow(form, MS_TR("Frame Gap"), 0, 256, QStringLiteral(" px"));
    m_previewGap = addSpinRow(form, MS_TR("Preview Gap"), 0, 512, QStringLiteral(" px"));
    m_hidePreviewDuringCapture = addSwitchRow(form,
                                              MS_TR("Hide Preview During Capture"),
                                              MS_TR("Hide the preview panel while each frame is captured."));
    layout->addWidget(scrollCard);
    layout->addStretch();
}

void SettingsPageScroll::setConfig(const SettingsConfig &config)
{
    m_frameEnabled->setChecked(config.scroll.frameEnabled);
    m_frameGap->setValue(config.scroll.frameGap);
    m_previewGap->setValue(config.scroll.previewGap);
    m_hidePreviewDuringCapture->setChecked(config.scroll.hidePreviewDuringCapture);
}

void SettingsPageScroll::updateConfig(SettingsConfig *config) const
{
    if (!config) {
        return;
    }

    config->scroll.frameEnabled = m_frameEnabled->isChecked();
    config->scroll.frameGap = m_frameGap->value();
    config->scroll.previewGap = m_previewGap->value();
    config->scroll.hidePreviewDuringCapture = m_hidePreviewDuringCapture->isChecked();
}

}  // namespace markshot::settings

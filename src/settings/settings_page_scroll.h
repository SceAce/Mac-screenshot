#pragma once

#include "settings/settings_config.h"

#include <QWidget>

class QCheckBox;
class QSpinBox;

namespace markshot::settings {

class SettingsPageScroll final : public QWidget {
public:
    /// @brief 创建滚动截图设置页。
    /// @param parent 父控件。
    explicit SettingsPageScroll(QWidget *parent = nullptr);

    /// @brief 将配置加载到页面控件。
    /// @param config 设置配置。
    void setConfig(const SettingsConfig &config);

    /// @brief 将页面控件值写回配置。
    /// @param config 需要更新的设置配置。
    void updateConfig(SettingsConfig *config) const;

private:
    QCheckBox *m_frameEnabled = nullptr;
    QSpinBox *m_frameGap = nullptr;
    QSpinBox *m_previewGap = nullptr;
    QCheckBox *m_hidePreviewDuringCapture = nullptr;
};

}  // namespace markshot::settings

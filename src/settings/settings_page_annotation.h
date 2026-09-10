#pragma once

#include "settings/settings_config.h"

#include <QColor>
#include <QWidget>

class QComboBox;
class QPushButton;
class QSpinBox;

namespace markshot::settings {

class SettingsPageAnnotation final : public QWidget {
public:
    /// @brief 创建标注设置页。
    /// @param parent 父控件。
    explicit SettingsPageAnnotation(QWidget *parent = nullptr);

    /// @brief 将配置加载到页面控件。
    /// @param config 设置配置。
    void setConfig(const SettingsConfig &config);

    /// @brief 将页面控件值写回配置。
    /// @param config 需要更新的设置配置。
    void updateConfig(SettingsConfig *config) const;

private:
    /// @brief 初始化工具下拉框的选项。
    /// @param combo 目标下拉框。
    void populateToolCombo(QComboBox *combo);

    /// @brief 设置工具下拉框的当前值。
    /// @param combo 目标下拉框。
    /// @param tool 工具枚举值。
    void setToolComboValue(QComboBox *combo, ShotWindow::Tool tool);

    /// @brief 读取工具下拉框的当前值。
    /// @param combo 目标下拉框。
    /// @return 当前工具枚举值。
    ShotWindow::Tool toolComboValue(QComboBox *combo) const;

    /// @brief 刷新颜色按钮外观。
    void updateColorButton();

    QComboBox *m_normalTool = nullptr;
    QComboBox *m_fullscreenTool = nullptr;
    QComboBox *m_fileTool = nullptr;
    QPushButton *m_colorButton = nullptr;
    QSpinBox *m_toolbarIconSize = nullptr;
    QSpinBox *m_toolbarFontSize = nullptr;
    QColor m_defaultColor;
};

}  // namespace markshot::settings

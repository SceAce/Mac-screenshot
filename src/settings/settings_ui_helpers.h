#pragma once

#include <QColor>
#include <QString>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QFrame;
class QKeySequenceEdit;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;
class QVBoxLayout;
class QWidget;

namespace markshot::settings {

/// @brief 创建设置页根布局。
/// @param parent 父控件。
/// @return 垂直布局。
QVBoxLayout *createSettingsPageLayout(QWidget *parent);

/// @brief 创建设置分组卡片。
/// @param title 分组标题。
/// @param description 分组描述。
/// @param parent 父控件。
/// @return 卡片控件。
QFrame *createSettingsCard(const QString &title, const QString &description, QWidget *parent);

/// @brief 获取卡片中的表单布局。
/// @param card 卡片控件。
/// @return 表单布局。
QFormLayout *settingsCardForm(QFrame *card);

/// @brief 添加开关表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param description 控件说明。
/// @return 复选框控件。
QCheckBox *addSwitchRow(QFormLayout *form, const QString &label, const QString &description = {});

/// @brief 添加文本输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param placeholder 占位文本。
/// @return 文本输入控件。
QLineEdit *addTextRow(QFormLayout *form, const QString &label, const QString &placeholder = {});

/// @brief 添加多行文本输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param placeholder 占位文本。
/// @return 多行文本输入控件。
QPlainTextEdit *addPlainTextRow(QFormLayout *form, const QString &label, const QString &placeholder = {});

/// @brief 添加数值输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param minimum 最小值。
/// @param maximum 最大值。
/// @param suffix 数值后缀。
/// @return 数值输入控件。
QSpinBox *addSpinRow(QFormLayout *form, const QString &label, int minimum, int maximum, const QString &suffix = {});

/// @brief 添加小数输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @param minimum 最小值。
/// @param maximum 最大值。
/// @param decimals 小数位数。
/// @return 小数输入控件。
QDoubleSpinBox *addDoubleRow(QFormLayout *form, const QString &label, double minimum, double maximum, int decimals);

/// @brief 添加下拉选择表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @return 下拉框控件。
QComboBox *addComboRow(QFormLayout *form, const QString &label);

/// @brief 添加快捷键输入表单项。
/// @param form 表单布局。
/// @param label 标签文本。
/// @return 快捷键输入控件。
QKeySequenceEdit *addShortcutRow(QFormLayout *form, const QString &label);

/// @brief 创建颜色选择按钮样式。
/// @param color 当前颜色。
/// @return 按钮样式表。
QString colorButtonStyleSheet(const QColor &color);

}  // namespace markshot::settings

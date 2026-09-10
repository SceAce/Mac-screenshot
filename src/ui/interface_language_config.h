#pragma once

#include "ui/i18n.h"

#include <QJsonObject>
#include <QString>

namespace markshot::ui {

enum class UiLanguageMode {
    System,
    English,
    Chinese,
};

/// @brief 从字符串解析界面语言模式。
/// @param raw 配置中的语言模式文本。
/// @return 解析后的界面语言模式。
UiLanguageMode uiLanguageModeFromString(const QString &raw);

/// @brief 返回界面语言模式的配置名称。
/// @param mode 界面语言模式。
/// @return 可写入配置文件的名称。
QString uiLanguageModeName(UiLanguageMode mode);

/// @brief 从应用配置根对象读取界面语言模式。
/// @param root 应用配置根对象。
/// @return 配置的界面语言模式，缺失或无效时返回 System。
UiLanguageMode uiLanguageModeFromConfigRoot(const QJsonObject &root);

/// @brief 将界面语言模式转换为实际界面语言。
/// @param mode 界面语言模式。
/// @return 当前应使用的界面语言。
markshot::i18n::Language languageForUiLanguageMode(UiLanguageMode mode);

}  // namespace markshot::ui

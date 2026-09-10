#include "toolbar_appearance_config.h"

#include "config_value.h"

#include <QJsonValue>

#include <algorithm>
#include <optional>

namespace {

constexpr int kMinIconSize = 12;
constexpr int kMaxIconSize = 48;
constexpr int kMinFontSize = 8;
constexpr int kMaxFontSize = 24;
constexpr int kMinButtonSize = 24;
constexpr int kMaxButtonSize = 64;

/// @brief 从字符串解析整数尺寸。
/// @param text 原始尺寸文本。
/// @return 解析成功时返回整数尺寸。
std::optional<int> intFromSizeText(QString text)
{
    text = text.trimmed().toLower();
    if (text.endsWith(QStringLiteral("px"))) {
        text.chop(2);
        text = text.trimmed();
    }

    bool ok = false;
    const int value = text.toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}

/// @brief 从 JSON 值解析整数尺寸。
/// @param value JSON 尺寸值。
/// @return 解析成功时返回整数尺寸。
std::optional<int> numericSizeValue(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toInt();
    }
    if (value.isString()) {
        return intFromSizeText(value.toString());
    }
    return std::nullopt;
}

/// @brief 按预设名称解析工具栏图标尺寸。
/// @param value JSON 尺寸值。
/// @return 解析成功时返回图标尺寸。
std::optional<int> iconPresetValue(const QJsonValue &value)
{
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString key = markshot::config::normalizedKey(value.toString());
    if (key == QStringLiteral("tiny") || key == QStringLiteral("mini")) {
        return 16;
    }
    if (key == QStringLiteral("small") || key == QStringLiteral("sm")) {
        return 18;
    }
    if (key == QStringLiteral("middle") || key == QStringLiteral("medium") || key == QStringLiteral("normal")
        || key == QStringLiteral("default") || key == QStringLiteral("md")) {
        return 22;
    }
    if (key == QStringLiteral("large") || key == QStringLiteral("lg")) {
        return 28;
    }
    if (key == QStringLiteral("xlarge") || key == QStringLiteral("xl") || key == QStringLiteral("extralarge")) {
        return 34;
    }
    return std::nullopt;
}

/// @brief 按预设名称解析工具栏字体尺寸。
/// @param value JSON 尺寸值。
/// @return 解析成功时返回字体尺寸。
std::optional<int> fontPresetValue(const QJsonValue &value)
{
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString key = markshot::config::normalizedKey(value.toString());
    if (key == QStringLiteral("tiny") || key == QStringLiteral("mini")) {
        return 9;
    }
    if (key == QStringLiteral("small") || key == QStringLiteral("sm")) {
        return 10;
    }
    if (key == QStringLiteral("middle") || key == QStringLiteral("medium") || key == QStringLiteral("normal")
        || key == QStringLiteral("default") || key == QStringLiteral("md")) {
        return 11;
    }
    if (key == QStringLiteral("large") || key == QStringLiteral("lg")) {
        return 13;
    }
    if (key == QStringLiteral("xlarge") || key == QStringLiteral("xl") || key == QStringLiteral("extralarge")) {
        return 16;
    }
    return std::nullopt;
}

/// @brief 解析带范围限制的工具栏尺寸。
/// @param value JSON 尺寸值。
/// @param minimum 最小允许值。
/// @param maximum 最大允许值。
/// @param presetReader 预设值解析函数。
/// @return 解析成功时返回限制后的尺寸。
std::optional<int> boundedSizeValue(const QJsonValue &value,
                                    int minimum,
                                    int maximum,
                                    std::optional<int> (*presetReader)(const QJsonValue &))
{
    if (value.isUndefined()) {
        return std::nullopt;
    }
    if (const std::optional<int> numeric = numericSizeValue(value)) {
        return std::clamp(*numeric, minimum, maximum);
    }
    if (const std::optional<int> preset = presetReader(value)) {
        return std::clamp(*preset, minimum, maximum);
    }
    return std::nullopt;
}

/// @brief 从配置根对象中提取工具栏外观配置对象。
/// @param root 应用配置根对象。
/// @return 工具栏外观配置对象。
QJsonObject toolbarAppearanceObject(const QJsonObject &root)
{
    QJsonObject object = markshot::config::firstNonEmptyObjectValue(
        root,
        {QStringLiteral("toolbar"),
         QStringLiteral("toolBar"),
         QStringLiteral("toolbarAppearance"),
         QStringLiteral("screenshotToolbar"),
         QStringLiteral("screenshotTools")});
    if (!object.isEmpty()) {
        return object;
    }

    const QJsonObject annotation = markshot::config::objectValue(root, QStringLiteral("annotation"));
    return markshot::config::firstNonEmptyObjectValue(
        annotation,
        {QStringLiteral("toolbar"), QStringLiteral("toolBar"), QStringLiteral("toolbarAppearance")});
}

/// @brief 按图标和字体尺寸计算按钮边长。
/// @param iconSize 图标尺寸。
/// @param fontSize 字体尺寸。
/// @param minimum 默认最小按钮尺寸。
/// @return 计算后的按钮边长。
int buttonSizeFor(int iconSize, int fontSize, int minimum)
{
    int buttonSize = std::max(minimum, iconSize + 8);
    buttonSize = std::max(buttonSize, fontSize + 19);
    return std::clamp(buttonSize, kMinButtonSize, kMaxButtonSize);
}

}  // namespace

namespace markshot {

ToolbarAppearanceConfig defaultToolbarAppearanceConfig()
{
    return {};
}

ToolbarAppearanceConfig toolbarAppearanceFromConfigRoot(const QJsonObject &root)
{
    ToolbarAppearanceConfig config = defaultToolbarAppearanceConfig();
    const QJsonObject toolbar = toolbarAppearanceObject(root);
    if (toolbar.isEmpty()) {
        return config;
    }

    bool hasConfiguredAppearance = false;

    // 1. 读取图标尺寸，配置后同时作用于主工具栏和动作工具栏
    const QJsonValue iconSizeValue = config::valueForKeys(
        toolbar,
        {QStringLiteral("iconSize"),
         QStringLiteral("icon-size"),
         QStringLiteral("icon_size"),
         QStringLiteral("icon"),
         QStringLiteral("toolIconSize"),
         QStringLiteral("buttonIconSize")});
    if (const std::optional<int> iconSize =
            boundedSizeValue(iconSizeValue, kMinIconSize, kMaxIconSize, iconPresetValue)) {
        config.toolbarIconSize = *iconSize;
        config.actionToolbarIconSize = *iconSize;
        hasConfiguredAppearance = true;
    }

    // 2. 读取字体尺寸，用于工具栏及其附属属性面板控件
    const QJsonValue fontSizeValue = config::valueForKeys(
        toolbar,
        {QStringLiteral("fontSize"),
         QStringLiteral("font-size"),
         QStringLiteral("font_size"),
         QStringLiteral("font"),
         QStringLiteral("toolFontSize"),
         QStringLiteral("labelFontSize")});
    if (const std::optional<int> fontSize =
            boundedSizeValue(fontSizeValue, kMinFontSize, kMaxFontSize, fontPresetValue)) {
        config.fontSize = *fontSize;
        hasConfiguredAppearance = true;
    }

    // 3. 根据最终图标和字体尺寸扩展按钮边长，避免大图标被裁剪
    if (hasConfiguredAppearance) {
        config.toolbarButtonSize = buttonSizeFor(config.toolbarIconSize, config.fontSize, config.toolbarButtonSize);
        config.actionToolbarButtonSize =
            buttonSizeFor(config.actionToolbarIconSize, config.fontSize, config.actionToolbarButtonSize);
    }
    return config;
}

}  // namespace markshot

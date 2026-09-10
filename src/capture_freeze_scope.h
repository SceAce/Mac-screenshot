#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace markshot {

enum class CaptureFreezeScope {
    CursorScreen,
    AllScreens,
};

/// @brief 返回捕获冻结范围的内置默认值。
/// @return 默认冻结范围。
CaptureFreezeScope defaultCaptureFreezeScope();

/// @brief 将配置文本解析为捕获冻结范围。
/// @param value 配置中的字符串值。
/// @return 解析成功时返回冻结范围，否则返回空值。
std::optional<CaptureFreezeScope> captureFreezeScopeFromText(QString value);

/// @brief 从应用配置根对象解析捕获冻结范围。
/// @param root 应用配置根对象。
/// @return 配置的冻结范围，缺失或非法时返回默认值。
CaptureFreezeScope captureFreezeScopeFromConfigRoot(const QJsonObject &root);

/// @brief 从当前应用配置文件读取捕获冻结范围。
/// @return 配置的冻结范围。
CaptureFreezeScope configuredCaptureFreezeScope();

/// @brief 返回冻结范围的规范配置名称。
/// @param scope 冻结范围。
/// @return 可写入配置文件的字符串名称。
QString captureFreezeScopeName(CaptureFreezeScope scope);

}  // namespace markshot

#pragma once

#include <QString>

namespace markshot::translate_common {

/**
 * 把自由文本的语言名称归一化为规范语言键。
 *
 * 目标语言在配置中允许写成英文名、中文名或 BCP-47 标记，例如 Simplified
 * Chinese、简体中文、zh-CN 都归一化为 zh-Hans。各厂商再把规范键映射为自家代码。
 *
 * @param language 目标语言名称。
 * @return 规范语言键，无法识别时返回空串。
 */
QString canonicalLanguageKey(const QString &language);

/**
 * 读取规范语言键对应的英文展示名。
 * @param canonicalKey 规范语言键。
 * @return 英文展示名，未知键时返回原样输入。
 */
QString canonicalLanguageDisplayName(const QString &canonicalKey);

}  // namespace markshot::translate_common

#pragma once

#include <QString>

namespace markshot::shot {

/**
 * 返回常驻 KWin 脚本的插件名。
 * @return 固定插件名，用于重复加载前先卸载旧实例。
 */
QString kdePinnedKeepAbovePluginName();

/**
 * 判断窗口标题是否属于需要 KWin keepAbove 的钉图窗口。
 * @param title 窗口标题。
 * @return 钉图或 OCR 结果窗口标题时返回 true。
 */
bool isKdePinnedKeepAboveTitle(const QString &title);

/**
 * 生成设置或取消 keepAbove 的 KWin JavaScript。
 * @param title 当前钉图窗口标题。
 * @param alwaysOnTop 是否保持置顶。
 * @return KWin 脚本源码。
 */
QString kdePinnedKeepAboveScriptSource(const QString &title, bool alwaysOnTop);

/**
 * 通过 KWin Scripting 接口应用钉图置顶。
 * @param title 当前钉图窗口标题。
 * @param alwaysOnTop 是否保持置顶。
 * @return 脚本加载并执行成功时返回 true。
 */
bool applyKdePinnedWindowKeepAbove(const QString &title, bool alwaysOnTop);

}  // namespace markshot::shot

#pragma once

#include "display_capture/display_capture_target.h"

#include <QString>

namespace markshot::display_capture {

/**
 * 保存显示器快照目标图像。
 * @param target 显示器截取目标。
 * @param savedPath 输出保存后的路径。
 * @return 保存成功时返回 true，否则返回 false。
 */
bool saveDisplayCaptureTarget(const Target &target, QString *savedPath);

}  // namespace markshot::display_capture

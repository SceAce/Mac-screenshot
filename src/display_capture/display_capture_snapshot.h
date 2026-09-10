#pragma once

#include "display_capture/display_capture_target.h"

#include <QVector>

namespace markshot::display_capture {

/**
 * 按当前显示器布局捕获一次虚拟桌面并生成显示器目标。
 * @param includeCursor 是否在快照中包含鼠标指针。
 * @param error 输出错误信息。
 * @return 可显示和操作的显示器截取目标。
 */
QVector<Target> captureDisplayTargets(bool includeCursor, QString *error);

}  // namespace markshot::display_capture

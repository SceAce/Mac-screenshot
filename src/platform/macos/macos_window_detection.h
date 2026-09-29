#pragma once

#include "window_detection.h"

#include <QRect>
#include <QVector>

namespace markshot::macos {

/**
 * Enumerates visible macOS application windows in front-to-back order.
 *
 * Returned rectangles use the same global logical coordinate space as
 * QScreen::geometry() and CaptureResult::sourceGeometry. Mark Shot's own
 * windows and non-application desktop elements are excluded.
 */
QVector<WindowInfo> enumerateWindowInfos(const QRect &captureGeometry);

} // namespace markshot::macos

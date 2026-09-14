#pragma once

#include "screen_capture.h"

namespace markshot::macos {

// Compose in physical pixels at the highest captured scale, including displays
// left/above the primary display. sourceGeometry always remains in Qt points.
CaptureResult composeDisplayCaptures(const QVector<CaptureResult> &frames, const QRect &geometry);

} // namespace markshot::macos

#pragma once

#include "screen_capture.h"

class QScreen;
class QWidget;

namespace markshot::macos {

bool screenCapturePermissionGranted();
// Must be called on the GUI thread. Permission is requested only by a user action.
bool ensureScreenCapturePermission(QWidget *parent = nullptr);
void openScreenCaptureSettings();
CaptureResult captureScreen(const CaptureRequest &request);
void showCaptureOverlay(QWidget *widget, QScreen *screen);

} // namespace markshot::macos

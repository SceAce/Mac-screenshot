#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include "platform/macos/macos_window_detection.h"

#include "debug_log.h"

#include <QRect>

namespace markshot::macos {

QVector<WindowInfo> enumerateWindowInfos(const QRect &captureGeometry)
{
    @autoreleasepool {
        const CGWindowListOption options = kCGWindowListOptionOnScreenOnly
            | kCGWindowListExcludeDesktopElements;
        CFArrayRef windowList = CGWindowListCopyWindowInfo(options, kCGNullWindowID);
        if (!windowList) {
            markshot::debugLog("window-detection", "macOS window list is unavailable");
            return {};
        }

        NSArray<NSDictionary *> *windows = CFBridgingRelease(windowList);
        QVector<WindowInfo> result;
        result.reserve(static_cast<qsizetype>(windows.count));
        const pid_t ownPid = NSProcessInfo.processInfo.processIdentifier;

        // CoreGraphics returns windows from front to back. The shared hover
        // selector treats larger z-order values as being closer to the user.
        for (NSUInteger index = 0; index < windows.count; ++index) {
            NSDictionary *entry = windows[index];
            NSNumber *ownerPid = entry[(id)kCGWindowOwnerPID];
            NSNumber *layer = entry[(id)kCGWindowLayer];
            NSNumber *alpha = entry[(id)kCGWindowAlpha];
            NSDictionary *boundsDictionary = entry[(id)kCGWindowBounds];
            if (!ownerPid || ownerPid.intValue == ownPid || !boundsDictionary) {
                continue;
            }
            // Layer zero contains normal application windows. Excluding menu,
            // Dock, tooltip, and overlay layers prevents tiny false targets.
            if (!layer || layer.intValue != 0 || (alpha && alpha.doubleValue <= 0.01)) {
                continue;
            }

            CGRect bounds = CGRectZero;
            if (!CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)boundsDictionary, &bounds)) {
                continue;
            }
            const QRect rect(qRound(bounds.origin.x),
                             qRound(bounds.origin.y),
                             qRound(bounds.size.width),
                             qRound(bounds.size.height));
            if (rect.width() <= 1 || rect.height() <= 1
                || (!captureGeometry.isEmpty() && !rect.intersects(captureGeometry))) {
                continue;
            }

            result.append({rect, static_cast<int>(windows.count - index)});
        }

        markshot::debugLog("window-detection",
                           "macOS native enumerator returned windows=%d",
                           static_cast<int>(result.size()));
        return result;
    }
}

} // namespace markshot::macos

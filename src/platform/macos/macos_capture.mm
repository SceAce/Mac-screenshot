#import <AppKit/AppKit.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include "macos_capture.h"
#include "macos_capture_compositor.h"
#include "debug_log.h"
#include "ui/i18n.h"

#include <QDeadlineTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

namespace markshot::macos {
namespace {

struct CaptureState {
    std::atomic_bool finished{false};
    SCShareableContent *content = nil;
    QImage image;
    QString error;
};

// Completion handlers may outlive a timeout. Only shared state is captured by
// them, and the release/acquire pair publishes their result to the caller.
bool waitForCapture(const std::shared_ptr<CaptureState> &state)
{
    QEventLoop loop;
    QTimer timer;
    QDeadlineTimer deadline(10000);
    timer.setInterval(10);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        if (state->finished.load(std::memory_order_acquire) || deadline.hasExpired()) {
            loop.quit();
        }
    });
    timer.start();
    if (!state->finished.load(std::memory_order_acquire)) {
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
    return state->finished.load(std::memory_order_acquire);
}

QImage copyImage(CGImageRef source)
{
    if (!source) {
        return {};
    }
    QImage image(int(CGImageGetWidth(source)), int(CGImageGetHeight(source)),
                 QImage::Format_ARGB32_Premultiplied);
    if (image.isNull()) {
        return {};
    }
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef context = CGBitmapContextCreate(image.bits(), image.width(), image.height(),
                                                8, image.bytesPerLine(), colorSpace,
                                                kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(colorSpace);
    if (!context) {
        return {};
    }
    CGContextDrawImage(context, CGRectMake(0, 0, image.width(), image.height()), source);
    CGContextRelease(context);
    return image;
}

QRect displayGeometry(SCDisplay *display)
{
    // CoreGraphics and Qt screen geometry both use a top-left desktop origin.
    const CGRect bounds = CGDisplayBounds(display.displayID);
    return QRect(qRound(bounds.origin.x), qRound(bounds.origin.y),
                 qRound(bounds.size.width), qRound(bounds.size.height));
}

QString outputName(const QRect &geometry)
{
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry() == geometry) {
            return screen->name();
        }
    }
    return {};
}

} // namespace

CaptureResult captureScreen(const CaptureRequest &request)
{
    @autoreleasepool {
        if (!screenCapturePermissionGranted()) {
            return {{}, MS_TR("Screen Recording permission is required. Open Mark Shot.app and allow it in System Settings > Privacy & Security > Screen & System Audio Recording."), {}, request.sourceGeometry};
        }
        auto contentState = std::make_shared<CaptureState>();
        [SCShareableContent getShareableContentExcludingDesktopWindows:NO onScreenWindowsOnly:YES
            completionHandler:^(SCShareableContent *content, NSError *error) {
                contentState->content = content;
                contentState->error = error ? QString::fromNSString(error.localizedDescription) : QString();
                contentState->finished.store(true, std::memory_order_release);
            }];
        if (!waitForCapture(contentState)) {
            return {{}, MS_TR("Screen capture timed out. Check Screen Recording permission and try again."), {}, request.sourceGeometry};
        }
        if (!contentState->content) {
            return {{}, contentState->error.isEmpty() ? MS_TR("No displays are available for capture.") : contentState->error, {}, request.sourceGeometry};
        }
        SCShareableContent *content = contentState->content;
        QRect geometry = request.sourceGeometry;
        if (geometry.isEmpty()) {
            for (SCDisplay *display in content.displays) {
                const QRect bounds = displayGeometry(display);
                if (request.allOutputs) {
                    geometry = geometry.united(bounds);
                } else if ((!request.preferredOutputName.isEmpty()
                            && outputName(bounds) == request.preferredOutputName)
                           || (request.preferredOutputName.isEmpty() && display.displayID == CGMainDisplayID())) {
                    geometry = bounds;
                    break;
                }
            }
        }
        QVector<CaptureResult> frames;
        for (SCDisplay *display in content.displays) {
            const QRect bounds = displayGeometry(display);
            if (!bounds.intersects(geometry)) {
                continue;
            }
            NSMutableArray<SCWindow *> *excluded = [NSMutableArray array];
            if (request.hideOwnWindows) {
                for (SCWindow *window in content.windows) {
                    if (window.owningApplication.processID == NSProcessInfo.processInfo.processIdentifier) {
                        [excluded addObject:window];
                    }
                }
            }
            SCContentFilter *filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:excluded];
            SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
            const qreal scale = std::max(1.0, double(filter.pointPixelScale));
            config.width = size_t(std::lround(bounds.width() * scale));
            config.height = size_t(std::lround(bounds.height() * scale));
            config.showsCursor = request.includeCursor;
            config.capturesAudio = NO;
            config.colorSpaceName = kCGColorSpaceSRGB;
            auto state = std::make_shared<CaptureState>();
            [SCScreenshotManager captureImageWithFilter:filter configuration:config
                completionHandler:^(CGImageRef image, NSError *error) {
                    state->image = copyImage(image);
                    state->error = error ? QString::fromNSString(error.localizedDescription) : QString();
                    state->finished.store(true, std::memory_order_release);
                }];
            if (!waitForCapture(state)) {
                return {{}, MS_TR("Screen capture timed out. Check Screen Recording permission and try again."), {}, geometry};
            }
            if (state->image.isNull()) {
                return {{}, state->error.isEmpty() ? MS_TR("The display returned an empty screenshot.") : state->error, {}, geometry};
            }
            markshot::debugLog("macos-capture", "display=%u points=%dx%d pixels=%dx%d",
                               display.displayID, bounds.width(), bounds.height(),
                               state->image.width(), state->image.height());
            frames.append({state->image, {}, outputName(bounds), bounds, request.includeCursor});
        }
        return composeDisplayCaptures(frames, geometry);
    }
}

void showCaptureOverlay(QWidget *widget, QScreen *screen)
{
    if (!widget) {
        return;
    }
    if (screen) {
        widget->setGeometry(screen->geometry());
    }
    // A regular borderless window avoids creating a separate macOS fullscreen Space.
    widget->show();
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(widget->winId());
    NSWindow *window = view.window;
    window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
        | NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorStationary;
    window.level = NSStatusWindowLevel + 1;
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

} // namespace markshot::macos

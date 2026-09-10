#include "display_capture/display_capture_snapshot.h"

#include "capture_geometry.h"
#include "debug_log.h"
#include "screen_capture.h"
#include "ui/i18n.h"

#include <QGuiApplication>
#include <QScreen>

#include <algorithm>

namespace markshot::display_capture {
namespace {

/**
 * 计算全部显示器组成的虚拟桌面几何。
 * @return 虚拟桌面几何。
 */
QRect virtualScreensGeometry()
{
    QRect geometry;
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (!screen) {
            continue;
        }
        geometry = geometry.isNull() ? screen->geometry() : geometry.united(screen->geometry());
    }
    return geometry;
}

/**
 * 生成显示器卡片使用的轻量缩略图。
 * @param image 显示器完整截图。
 * @return 缩略图。
 */
QImage targetThumbnail(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    return image.scaled(QSize(360, 220), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

/**
 * 记录显示器快照中的屏幕缩放诊断信息。
 * @param screens 当前屏幕列表。
 * @return 无返回值。
 */
void logDisplayCaptureScreens(const QList<QScreen *> &screens)
{
    int index = 0;
    for (QScreen *screen : screens) {
        if (!screen) {
            markshot::debugLog("display-capture",
                               "【显示器快照】【缩放诊断】screen index=%d null=1",
                               index++);
            continue;
        }

        const QRect geometry = screen->geometry();
        const QRect available = screen->availableGeometry();
        markshot::debugLog("display-capture",
                           "【显示器快照】【缩放诊断】screen index=%d name=%s geom=%d,%d %dx%d "
                           "available=%d,%d %dx%d dpr=%.3f logical_dpi=%.3fx%.3f "
                           "physical_dpi=%.3fx%.3f refresh=%.3f",
                           index++,
                           screen->name().toUtf8().constData(),
                           geometry.x(), geometry.y(), geometry.width(), geometry.height(),
                           available.x(), available.y(), available.width(), available.height(),
                           screen->devicePixelRatio(),
                           screen->logicalDotsPerInchX(), screen->logicalDotsPerInchY(),
                           screen->physicalDotsPerInchX(), screen->physicalDotsPerInchY(),
                           screen->refreshRate());
    }
}

/**
 * 生成目标副标题。
 * @param geometry 目标全局逻辑几何。
 * @param image 目标图像。
 * @return 副标题文本。
 */
QString targetSubtitle(QRect geometry, const QImage &image)
{
    const QSize size = image.isNull() ? geometry.size() : image.size();
    return QStringLiteral("%1 x %2").arg(size.width()).arg(size.height());
}

/**
 * 构建完整虚拟桌面目标。
 * @param image 虚拟桌面截图。
 * @param geometry 图像对应的全局逻辑几何。
 * @return 截取目标。
 */
Target allDisplaysTarget(const QImage &image, QRect geometry)
{
    Target target;
    target.allOutputs = true;
    target.outputName = QStringLiteral("all-displays");
    target.title = MS_TR("All displays");
    target.subtitle = targetSubtitle(geometry, image);
    target.geometry = geometry;
    target.image = image;
    target.thumbnail = targetThumbnail(image);
    return target;
}

/**
 * 构建单个显示器目标。
 * @param screen 显示器对象。
 * @param image 显示器截图。
 * @param geometry 图像对应的全局逻辑几何。
 * @return 截取目标。
 */
Target screenTarget(QScreen *screen, const QImage &image, QRect geometry)
{
    Target target;
    target.screenName = screen ? screen->name() : QString();
    target.outputName = target.screenName.isEmpty() ? QStringLiteral("display") : target.screenName;
    target.title = target.screenName.isEmpty() ? MS_TR("Display") : target.screenName;
    target.subtitle = targetSubtitle(geometry, image);
    target.geometry = geometry;
    target.image = image;
    target.thumbnail = targetThumbnail(image);
    return target;
}

}  // namespace

QVector<Target> captureDisplayTargets(bool includeCursor, QString *error)
{
    QVector<Target> targets;
    const QList<QScreen *> screens = QGuiApplication::screens();
    const QRect virtualGeometry = virtualScreensGeometry();
    if (virtualGeometry.isEmpty()) {
        if (error) {
            *error = QStringLiteral("no virtual screen geometry available for display capture");
        }
        return targets;
    }

    markshot::debugLog("display-capture",
                       "【显示器快照】【缩放诊断】start include_cursor=%d screen_count=%d "
                       "platform=%s virtual=%d,%d %dx%d",
                       includeCursor ? 1 : 0,
                       static_cast<int>(screens.size()),
                       QGuiApplication::platformName().toUtf8().constData(),
                       virtualGeometry.x(), virtualGeometry.y(),
                       virtualGeometry.width(), virtualGeometry.height());
    logDisplayCaptureScreens(screens);

    CaptureRequest request;
    request.sourceGeometry = virtualGeometry;
    request.allOutputs = true;
    request.includeCursor = includeCursor;
    const CaptureResult capture = captureScreenFrame(request);
    if (capture.image.isNull()) {
        if (error) {
            *error = capture.error;
        }
        return targets;
    }

    const QRect frameGeometry = capture.sourceGeometry.isValid() && !capture.sourceGeometry.isEmpty()
        ? capture.sourceGeometry
        : virtualGeometry;
    markshot::debugLog("display-capture",
                       "【显示器快照】【缩放诊断】all-result frame_geom=%d,%d %dx%d "
                       "image=%dx%d scale=%.6fx%.6f",
                       frameGeometry.x(), frameGeometry.y(),
                       frameGeometry.width(), frameGeometry.height(),
                       capture.image.width(), capture.image.height(),
                       static_cast<qreal>(capture.image.width()) / std::max(1, frameGeometry.width()),
                       static_cast<qreal>(capture.image.height()) / std::max(1, frameGeometry.height()));
    if (screens.size() > 1) {
        targets.append(allDisplaysTarget(capture.image, frameGeometry));
    }

    for (QScreen *screen : screens) {
        if (!screen || screen->geometry().isEmpty()) {
            continue;
        }

        const QRect screenGeometry = screen->geometry().intersected(frameGeometry);
        if (screenGeometry.isEmpty()) {
            continue;
        }

        const QImage image = markshot::capture::cropFrameToRequest(capture.image, frameGeometry, screenGeometry);
        if (image.isNull()) {
            if (error) {
                *error = QStringLiteral("failed to crop shared display capture for screen %1").arg(screen->name());
            }
            targets.clear();
            return targets;
        }
        markshot::debugLog("display-capture",
                           "【显示器快照】【缩放诊断】crop screen=%s geom=%d,%d %dx%d "
                           "dpr=%.3f image=%dx%d scale=%.6fx%.6f",
                           screen->name().toUtf8().constData(),
                           screenGeometry.x(), screenGeometry.y(),
                           screenGeometry.width(), screenGeometry.height(),
                           screen->devicePixelRatio(),
                           image.width(), image.height(),
                           static_cast<qreal>(image.width()) / std::max(1, screenGeometry.width()),
                           static_cast<qreal>(image.height()) / std::max(1, screenGeometry.height()));
        targets.append(screenTarget(screen, image, screenGeometry));
    }

    return targets;
}

}  // namespace markshot::display_capture

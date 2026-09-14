#include "macos_capture_compositor.h"

#include <QPainter>

#include <algorithm>
#include <cmath>

namespace markshot::macos {

CaptureResult composeDisplayCaptures(const QVector<CaptureResult> &frames, const QRect &geometry)
{
    if (frames.isEmpty() || geometry.isEmpty()) {
        return {{}, QStringLiteral("No display intersects the capture region."), {}, geometry};
    }
    qreal scale = 1.0;
    for (const CaptureResult &frame : frames) {
        if (frame.image.isNull() || frame.sourceGeometry.isEmpty()) {
            return {{}, QStringLiteral("A display returned an empty screenshot."), {}, geometry};
        }
        scale = std::max({scale,
                         qreal(frame.image.width()) / frame.sourceGeometry.width(),
                         qreal(frame.image.height()) / frame.sourceGeometry.height()});
    }
    // Bound allocation for extreme virtual desktops before converting to int.
    const qreal width = std::ceil(geometry.width() * scale);
    const qreal height = std::ceil(geometry.height() * scale);
    if (width > 32768 || height > 32768 || width * height > 128 * 1024 * 1024) {
        return {{}, QStringLiteral("Capture region is too large. Capture one display at a time."), {}, geometry};
    }
    QImage image(QSize(int(width), int(height)), QImage::Format_ARGB32_Premultiplied);
    if (image.isNull()) {
        return {{}, QStringLiteral("Could not allocate the screenshot image."), {}, geometry};
    }
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    bool cursorIncluded = true;
    for (const CaptureResult &frame : frames) {
        const QRect overlap = frame.sourceGeometry.intersected(geometry);
        if (overlap.isEmpty()) {
            continue;
        }
        const qreal sx = qreal(frame.image.width()) / frame.sourceGeometry.width();
        const qreal sy = qreal(frame.image.height()) / frame.sourceGeometry.height();
        const QRectF source((overlap.x() - frame.sourceGeometry.x()) * sx,
                            (overlap.y() - frame.sourceGeometry.y()) * sy,
                            overlap.width() * sx, overlap.height() * sy);
        const int left = qRound((overlap.x() - geometry.x()) * scale);
        const int top = qRound((overlap.y() - geometry.y()) * scale);
        const int right = qRound((overlap.x() + overlap.width() - geometry.x()) * scale);
        const int bottom = qRound((overlap.y() + overlap.height() - geometry.y()) * scale);
        painter.drawImage(QRect(left, top, right - left, bottom - top), frame.image, source);
        cursorIncluded = cursorIncluded && frame.cursorIncluded;
    }
    painter.end();
    return {image, {}, frames.size() == 1 ? frames.first().outputName : QString(), geometry, cursorIncluded};
}

} // namespace markshot::macos

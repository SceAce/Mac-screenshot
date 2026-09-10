#pragma once

#include <QImage>
#include <QRect>
#include <QString>

namespace markshot::display_capture {

struct Target {
    bool allOutputs = false;
    QString screenName;
    QString outputName;
    QString title;
    QString subtitle;
    QRect geometry;
    QImage image;
    QImage thumbnail;
};

}  // namespace markshot::display_capture

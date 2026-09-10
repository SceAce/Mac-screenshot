#include "display_capture/display_capture_save.h"

#include "app_config_store.h"
#include "save_path_config.h"

#include <QDateTime>
#include <QJsonObject>
#include <QPoint>
#include <QRect>

namespace markshot::display_capture {
namespace {

/**
 * 构造显示器快照保存路径上下文。
 * @param target 显示器截取目标。
 * @return 保存路径上下文。
 */
markshot::SavePathContext savePathContext(const Target &target)
{
    markshot::SavePathContext context;
    context.timestamp = QDateTime::currentDateTime();
    context.selectionRect = QRect(QPoint(0, 0), target.image.size());
    context.sourceGeometry = target.geometry;
    context.imageSize = target.image.size();
    context.outputName = target.outputName.isEmpty() ? target.title : target.outputName;
    context.extension = QStringLiteral("png");
    return context;
}

/**
 * 生成显示器快照默认保存路径。
 * @param target 显示器截取目标。
 * @return 保存路径。
 */
QString defaultTargetSavePath(const Target &target)
{
    const markshot::SavePathContext context = savePathContext(target);
    bool ok = false;
    const QJsonObject root = markshot::readAppConfigRoot(&ok);
    return ok ? markshot::savePathFromConfigRoot(root, context) : markshot::defaultSavePath(context);
}

}  // namespace

bool saveDisplayCaptureTarget(const Target &target, QString *savedPath)
{
    if (target.image.isNull()) {
        return false;
    }

    const QString path = defaultTargetSavePath(target);
    if (!markshot::ensureSavePathDirectory(path) || !target.image.save(path, "PNG")) {
        return false;
    }

    if (savedPath) {
        *savedPath = path;
    }
    return true;
}

}  // namespace markshot::display_capture

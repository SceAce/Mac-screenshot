#include "save_path_config.h"

#include "config_value.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QStandardPaths>

namespace cfg = markshot::config;

namespace {

/**
 * 选择可写系统目录，系统未提供时回退到用户主目录。
 * @param location Qt 标准目录类型。
 * @return 可写目录路径。
 */
QString writableStandardDir(QStandardPaths::StandardLocation location)
{
    const QString path = QStandardPaths::writableLocation(location);
    return path.isEmpty() ? QDir::homePath() : path;
}

/**
 * 返回默认截图保存目录。
 * @return 默认截图保存目录路径。
 */
QString defaultSaveDirectory()
{
    return QDir(writableStandardDir(QStandardPaths::PicturesLocation)).filePath(QStringLiteral("mark-shot"));
}

/**
 * 返回应用配置目录。
 * @return 应用配置目录路径。
 */
QString appConfigDirectory()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!path.isEmpty()) {
        return path;
    }

    const QString genericConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!genericConfig.isEmpty()) {
        return QDir(genericConfig).filePath(QStringLiteral("mark-shot"));
    }
    return QDir::home().filePath(QStringLiteral(".config/mark-shot"));
}

/**
 * 返回应用数据目录。
 * @return 应用数据目录路径。
 */
QString appDataDirectory()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.isEmpty()) {
        return path;
    }

    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (!genericData.isEmpty()) {
        return QDir(genericData).filePath(QStringLiteral("mark-shot"));
    }
    return QDir::home().filePath(QStringLiteral(".local/share/mark-shot"));
}

/**
 * 将整数格式化为固定宽度文本。
 * @param value 待格式化整数。
 * @param width 输出宽度。
 * @return 左侧补零后的文本。
 */
QString paddedNumber(int value, int width)
{
    return QStringLiteral("%1").arg(value, width, 10, QLatin1Char('0'));
}

/**
 * 生成文件名安全文本。
 * @param value 原始文本。
 * @param fallback 文本为空时使用的备用值。
 * @return 可作为单个文件名片段使用的文本。
 */
QString safeFileComponent(QString value, const QString &fallback)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        value = fallback;
    }

    QString result;
    result.reserve(value.size());
    for (const QChar ch : value) {
        if (ch.unicode() < 0x20
            || ch.unicode() == 0x7f
            || ch == QLatin1Char('/')
            || ch == QLatin1Char('\\')
            || ch == QLatin1Char(':')
            || ch == QLatin1Char('*')
            || ch == QLatin1Char('?')
            || ch == QLatin1Char('"')
            || ch == QLatin1Char('<')
            || ch == QLatin1Char('>')
            || ch == QLatin1Char('|')) {
            result.append(QLatin1Char('-'));
        } else {
            result.append(ch);
        }
    }

    return result.trimmed().isEmpty() ? fallback : result.trimmed();
}

/**
 * 生成矩形几何文本。
 * @param rect 矩形。
 * @return `x,y widthxheight` 形式的几何文本。
 */
QString geometryText(const QRect &rect)
{
    return QStringLiteral("%1,%2 %3x%4")
        .arg(rect.x())
        .arg(rect.y())
        .arg(rect.width())
        .arg(rect.height());
}

/**
 * 写入矩形占位符。
 * @param values 占位符映射表。
 * @param prefix 占位符前缀。
 * @param rect 矩形。
 * @return 无返回值。
 */
void insertRectPlaceholders(QHash<QString, QString> *values, const QString &prefix, const QRect &rect)
{
    if (!values) {
        return;
    }

    values->insert(prefix + QStringLiteral(".x"), QString::number(rect.x()));
    values->insert(prefix + QStringLiteral(".y"), QString::number(rect.y()));
    values->insert(prefix + QStringLiteral(".width"), QString::number(rect.width()));
    values->insert(prefix + QStringLiteral(".height"), QString::number(rect.height()));
    values->insert(prefix + QStringLiteral(".right"), QString::number(rect.right()));
    values->insert(prefix + QStringLiteral(".bottom"), QString::number(rect.bottom()));
    values->insert(prefix + QStringLiteral(".geometry"), geometryText(rect));
}

/**
 * 构造保存路径模板占位符映射。
 * @param context 保存路径占位符上下文。
 * @return 占位符到文本值的映射表。
 */
QHash<QString, QString> savePathPlaceholderValues(const markshot::SavePathContext &context)
{
    const QDateTime timestamp = context.timestamp.isValid() ? context.timestamp : QDateTime::currentDateTime();
    const QDate date = timestamp.date();
    const QTime time = timestamp.time();
    const int twelveHour = time.hour() % 12 == 0 ? 12 : time.hour() % 12;
    const QString extension = safeFileComponent(context.extension.isEmpty() ? QStringLiteral("png") : context.extension,
                                                QStringLiteral("png"));

    QHash<QString, QString> values;
    values.insert(QStringLiteral("home"), QDir::homePath());
    values.insert(QStringLiteral("pictures"), writableStandardDir(QStandardPaths::PicturesLocation));
    values.insert(QStringLiteral("desktop"), writableStandardDir(QStandardPaths::DesktopLocation));
    values.insert(QStringLiteral("downloads"), writableStandardDir(QStandardPaths::DownloadLocation));
    values.insert(QStringLiteral("config"), appConfigDirectory());
    values.insert(QStringLiteral("data"), appDataDirectory());

    values.insert(QStringLiteral("timestamp"), QString::number(timestamp.toSecsSinceEpoch()));
    values.insert(QStringLiteral("timestamp.ms"), QString::number(timestamp.toMSecsSinceEpoch()));
    values.insert(QStringLiteral("yyyy"), paddedNumber(date.year(), 4));
    values.insert(QStringLiteral("yy"), paddedNumber(date.year() % 100, 2));
    values.insert(QStringLiteral("MM"), paddedNumber(date.month(), 2));
    values.insert(QStringLiteral("M"), QString::number(date.month()));
    values.insert(QStringLiteral("dd"), paddedNumber(date.day(), 2));
    values.insert(QStringLiteral("d"), QString::number(date.day()));
    values.insert(QStringLiteral("HH"), paddedNumber(time.hour(), 2));
    values.insert(QStringLiteral("hh"), paddedNumber(twelveHour, 2));
    values.insert(QStringLiteral("mm"), paddedNumber(time.minute(), 2));
    values.insert(QStringLiteral("ss"), paddedNumber(time.second(), 2));
    values.insert(QStringLiteral("zzz"), paddedNumber(time.msec(), 3));
    values.insert(QStringLiteral("date"),
                  values.value(QStringLiteral("yyyy"))
                      + values.value(QStringLiteral("MM"))
                      + values.value(QStringLiteral("dd")));
    values.insert(QStringLiteral("time"),
                  values.value(QStringLiteral("HH"))
                      + values.value(QStringLiteral("mm"))
                      + values.value(QStringLiteral("ss")));
    values.insert(QStringLiteral("datetime"),
                  values.value(QStringLiteral("date"))
                      + QLatin1Char('-')
                      + values.value(QStringLiteral("time")));

    insertRectPlaceholders(&values, QStringLiteral("selection"), context.selectionRect);
    insertRectPlaceholders(&values, QStringLiteral("source"), context.sourceGeometry);
    values.insert(QStringLiteral("image.width"), QString::number(context.imageSize.width()));
    values.insert(QStringLiteral("image.height"), QString::number(context.imageSize.height()));
    values.insert(QStringLiteral("name"), safeFileComponent(context.outputName, QStringLiteral("capture")));
    values.insert(QStringLiteral("ext"), extension);
    return values;
}

/**
 * 解析自定义日期时间占位符。
 * @param placeholder 占位符名称。
 * @param context 保存路径占位符上下文。
 * @param value 输出解析后的文本。
 * @return 成功解析时返回 true，否则返回 false。
 */
bool datetimeFormatPlaceholderValue(const QString &placeholder,
                                    const markshot::SavePathContext &context,
                                    QString *value)
{
    static const QString prefix = QStringLiteral("datetime:");
    if (!placeholder.startsWith(prefix)) {
        return false;
    }

    const QString format = placeholder.mid(prefix.size());
    if (format.isEmpty()) {
        return false;
    }

    const QDateTime timestamp = context.timestamp.isValid() ? context.timestamp : QDateTime::currentDateTime();
    if (value) {
        *value = timestamp.toString(format);
    }
    return true;
}

/**
 * 展开用户目录前缀。
 * @param path 原始路径。
 * @return 展开后的路径。
 */
QString expandHomePrefix(QString path)
{
    if (path == QStringLiteral("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QStringLiteral("~/")) || path.startsWith(QStringLiteral("~\\"))) {
        return QDir(QDir::homePath()).filePath(path.mid(2));
    }
    return path;
}

/**
 * 给目录模板追加默认文件名模板。
 * @param directoryTemplate 目录模板。
 * @return 带默认文件名的路径模板。
 */
QString withDefaultFileTemplate(QString directoryTemplate)
{
    if (directoryTemplate.trimmed().isEmpty()) {
        return {};
    }
    return QDir(directoryTemplate).filePath(QStringLiteral("mark-shot-{datetime}.png"));
}

/**
 * 从对象读取第一个字符串配置值。
 * @param object 配置对象。
 * @param keys 候选键名列表。
 * @return 读取到的字符串，未读取到时返回空字符串。
 */
QString stringValueForKeys(const QJsonObject &object, const QStringList &keys)
{
    const QJsonValue value = cfg::valueForKeys(object, keys);
    return value.isString() ? value.toString().trimmed() : QString();
}

/**
 * 从配置根对象读取保存目录模板。
 * @param root 应用配置根对象。
 * @return 配置的保存目录模板，未配置时返回空字符串。
 */
QString saveDirectoryTemplateFromConfigRoot(const QJsonObject &root)
{
    const QJsonObject save = cfg::firstObjectValue(root,
                                                   {QStringLiteral("save"),
                                                    QStringLiteral("saving"),
                                                    QStringLiteral("screenshotSave"),
                                                    QStringLiteral("screenshots")});
    return stringValueForKeys(save,
                              {QStringLiteral("directoryTemplate"),
                               QStringLiteral("dirTemplate"),
                               QStringLiteral("folderTemplate"),
                               QStringLiteral("directory"),
                               QStringLiteral("dir"),
                               QStringLiteral("folder")});
}

/**
 * 将展开路径规范化为最终 PNG 文件路径。
 * @param path 展开后的路径。
 * @param context 保存路径占位符上下文。
 * @return 规范化后的 PNG 文件路径。
 */
QString finalizedPngPath(QString path, const markshot::SavePathContext &context)
{
    path = expandHomePrefix(path.trimmed());
    if (path.isEmpty()) {
        return {};
    }

    if (path.endsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('\\'))) {
        const QDateTime timestamp = context.timestamp.isValid() ? context.timestamp : QDateTime::currentDateTime();
        path = QDir(path).filePath(QStringLiteral("mark-shot-%1.png")
                                       .arg(timestamp.toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    }

    if (QDir::isRelativePath(path)) {
        path = QDir(defaultSaveDirectory()).filePath(path);
    }

    path = QDir::cleanPath(path);
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".png");
    }
    return path;
}

}  // namespace

namespace markshot {

QString defaultSavePathTemplate()
{
    return QStringLiteral("{pictures}/mark-shot/mark-shot-{datetime}.png");
}

QString defaultSavePath(const SavePathContext &context)
{
    const QString expanded = expandedSavePathTemplate(defaultSavePathTemplate(), context);
    if (!expanded.isEmpty()) {
        return finalizedPngPath(expanded, context);
    }

    const QDateTime timestamp = context.timestamp.isValid() ? context.timestamp : QDateTime::currentDateTime();
    const QString filename = QStringLiteral("mark-shot-%1.png")
                                 .arg(timestamp.toString(QStringLiteral("yyyyMMdd-HHmmss")));
    return QDir(defaultSaveDirectory()).filePath(filename);
}

QString savePathTemplateFromConfigRoot(const QJsonObject &root)
{
    const QJsonObject save = cfg::firstObjectValue(root,
                                                   {QStringLiteral("save"),
                                                    QStringLiteral("saving"),
                                                    QStringLiteral("screenshotSave"),
                                                    QStringLiteral("screenshots")});
    const QString nestedTemplate =
        stringValueForKeys(save,
                           {QStringLiteral("pathTemplate"),
                            QStringLiteral("filePathTemplate"),
                            QStringLiteral("template"),
                            QStringLiteral("path"),
                            QStringLiteral("location")});
    if (!nestedTemplate.isEmpty()) {
        return nestedTemplate;
    }

    return stringValueForKeys(root,
                              {QStringLiteral("savePathTemplate"),
                               QStringLiteral("saveFilePathTemplate"),
                               QStringLiteral("screenshotPathTemplate"),
                               QStringLiteral("savePath")});
}

QString expandedSavePathTemplate(QString pathTemplate, const SavePathContext &context)
{
    pathTemplate = pathTemplate.trimmed();
    if (pathTemplate.isEmpty()) {
        return {};
    }

    const QHash<QString, QString> values = savePathPlaceholderValues(context);
    const QRegularExpression expression(QStringLiteral("\\{([^{}]+)\\}"));
    QRegularExpressionMatchIterator it = expression.globalMatch(pathTemplate);
    QString result;
    qsizetype lastIndex = 0;

    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        result += pathTemplate.mid(lastIndex, match.capturedStart() - lastIndex);

        const QString placeholder = match.captured(1);
        QString value;
        if (datetimeFormatPlaceholderValue(placeholder, context, &value)) {
            result += value;
        } else if (values.contains(placeholder)) {
            result += values.value(placeholder);
        } else {
            return {};
        }

        lastIndex = match.capturedEnd();
    }

    result += pathTemplate.mid(lastIndex);
    return result;
}

QString savePathFromConfigRoot(const QJsonObject &root, const SavePathContext &context)
{
    QString pathTemplate = savePathTemplateFromConfigRoot(root);
    if (pathTemplate.isEmpty()) {
        pathTemplate = withDefaultFileTemplate(saveDirectoryTemplateFromConfigRoot(root));
    }
    if (pathTemplate.isEmpty()) {
        return defaultSavePath(context);
    }

    const QString expanded = expandedSavePathTemplate(pathTemplate, context);
    if (expanded.isEmpty()) {
        return defaultSavePath(context);
    }

    const QString path = finalizedPngPath(expanded, context);
    return path.isEmpty() ? defaultSavePath(context) : path;
}

bool ensureSavePathDirectory(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QDir dir(QFileInfo(path).absolutePath());
    return dir.exists() || dir.mkpath(QStringLiteral("."));
}

}  // namespace markshot

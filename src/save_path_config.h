#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QRect>
#include <QSize>
#include <QString>

namespace markshot {

struct SavePathContext {
    QDateTime timestamp;
    QRect selectionRect;
    QRect sourceGeometry;
    QSize imageSize;
    QString outputName;
    QString extension = QStringLiteral("png");
};

/**
 * 返回默认保存路径模板。
 * @return 默认保存路径模板字符串。
 */
QString defaultSavePathTemplate();

/**
 * 根据上下文生成默认保存路径。
 * @param context 保存路径占位符上下文。
 * @return 默认 PNG 保存路径。
 */
QString defaultSavePath(const SavePathContext &context);

/**
 * 从配置根对象读取保存路径模板。
 * @param root 应用配置根对象。
 * @return 配置的保存路径模板，未配置时返回空字符串。
 */
QString savePathTemplateFromConfigRoot(const QJsonObject &root);

/**
 * 展开保存路径模板中的占位符。
 * @param pathTemplate 保存路径模板。
 * @param context 保存路径占位符上下文。
 * @return 展开后的路径；模板无效时返回空字符串。
 */
QString expandedSavePathTemplate(QString pathTemplate, const SavePathContext &context);

/**
 * 根据配置根对象和上下文生成最终保存路径。
 * @param root 应用配置根对象。
 * @param context 保存路径占位符上下文。
 * @return 最终 PNG 保存路径。
 */
QString savePathFromConfigRoot(const QJsonObject &root, const SavePathContext &context);

/**
 * 确保保存路径的父目录存在。
 * @param path 最终保存文件路径。
 * @return 父目录存在或创建成功时返回 true，否则返回 false。
 */
bool ensureSavePathDirectory(const QString &path);

}  // namespace markshot

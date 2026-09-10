#include "translate_config_source.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <algorithm>

namespace markshot::translate_common {
namespace {

/**
 * 读取去除空白后的环境变量。
 * @param env 环境变量集合。
 * @param name 变量名。
 * @return 变量值。
 */
QString envValue(const QProcessEnvironment &env, const QString &name)
{
    return env.value(name).trimmed();
}

/**
 * 追加配置文件候选路径。
 * @param paths 路径列表。
 * @param path 待追加路径。
 * @return 无返回值。
 */
void addConfigPath(QStringList *paths, const QString &path)
{
    const QString trimmed = path.trimmed();
    if (!trimmed.isEmpty() && !paths->contains(trimmed)) {
        paths->append(trimmed);
    }
}

/**
 * 追加配置目录下的 config.json 候选路径。
 * @param paths 路径列表。
 * @param dir 配置目录。
 * @return 无返回值。
 */
void addConfigDir(QStringList *paths, const QString &dir)
{
    if (!dir.trimmed().isEmpty()) {
        addConfigPath(paths, QDir(dir).filePath(QStringLiteral("config.json")));
    }
}

#if defined(Q_OS_WIN)
/**
 * 按 Windows 环境变量构造配置目录。
 * @param env 环境变量集合。
 * @param name 变量名。
 * @param relativePath 相对路径。
 * @return 配置目录，变量缺失时为空串。
 */
QString windowsConfigDir(const QProcessEnvironment &env,
                         const QString &name,
                         const QString &relativePath = QStringLiteral("mark-shot"))
{
    const QString root = envValue(env, name);
    return root.isEmpty() ? QString() : QDir(root).filePath(relativePath);
}
#endif

/**
 * 读取配置文件候选路径。
 * @param env 环境变量集合。
 * @return 候选配置文件路径列表。
 */
QStringList configPathCandidates(const QProcessEnvironment &env)
{
    QStringList paths;
    addConfigPath(&paths, envValue(env, QStringLiteral("MARK_SHOT_CONFIG")));

#if defined(Q_OS_WIN)
    addConfigDir(&paths, windowsConfigDir(env, QStringLiteral("LOCALAPPDATA")));
    addConfigDir(&paths, windowsConfigDir(env, QStringLiteral("APPDATA")));
    addConfigDir(&paths,
                 windowsConfigDir(env, QStringLiteral("USERPROFILE"), QStringLiteral("AppData/Local/mark-shot")));
#endif

    addConfigDir(&paths, QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

    const QString genericConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    addConfigDir(&paths, QDir(genericConfig).filePath(QStringLiteral("mark-shot")));
    addConfigDir(&paths, QDir::home().filePath(QStringLiteral(".config/mark-shot")));
    return paths;
}

}  // namespace

QJsonObject readTranslationConfig()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &path : configPathCandidates(env)) {
        QFile file(path);
        if (!QFileInfo::exists(path) || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        if (document.isObject()) {
            return document.object().value(QStringLiteral("translation")).toObject();
        }
    }
    return {};
}

TranslateConfigSource readTranslateConfigSource(const QString &vendorKey)
{
    TranslateConfigSource source;
    source.translation = readTranslationConfig();
    source.vendor = source.translation.value(vendorKey).toObject();
    return source;
}

QString configString(const TranslateConfigSource &source,
                     const QString &key,
                     const QStringList &envNames,
                     const QString &fallback)
{
    // 1. 厂商子节优先，便于同一份配置里并存多个翻译服务
    const QString vendorValue = source.vendor.value(key).toString().trimmed();
    if (!vendorValue.isEmpty()) {
        return vendorValue;
    }

    // 2. 公共 translation 节兜底，兼容只配置了单一服务的旧配置
    const QString sharedValue = source.translation.value(key).toString().trimmed();
    if (!sharedValue.isEmpty()) {
        return sharedValue;
    }

    // 3. 环境变量按候选顺序取首个非空值，凭据可不落盘
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const QString &name : envNames) {
        const QString value = envValue(env, name);
        if (!value.isEmpty()) {
            return value;
        }
    }
    return fallback.trimmed();
}

int configInt(const TranslateConfigSource &source,
              const QString &key,
              int fallback,
              int minimum,
              int maximum)
{
    int value = fallback;
    if (source.translation.contains(key)) {
        value = source.translation.value(key).toInt(value);
    }
    if (source.vendor.contains(key)) {
        value = source.vendor.value(key).toInt(value);
    }
    return std::clamp(value, minimum, maximum);
}

}  // namespace markshot::translate_common

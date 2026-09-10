#include "pinned_window/pinned_kde_keep_above.h"

#include "debug_log.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QStringList>

#ifdef MARK_SHOT_WITH_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#endif

namespace markshot::shot {
namespace {

constexpr const char *kPluginName = "mark-shot-pinned-keep-above";

/**
 * 判断当前进程是否运行在 KDE/Plasma 会话。
 * @return KDE/Plasma 会话返回 true。
 */
bool isKdeSession()
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString desktop =
        (env.value(QStringLiteral("XDG_CURRENT_DESKTOP")) + QLatin1Char(':')
         + env.value(QStringLiteral("XDG_SESSION_DESKTOP")) + QLatin1Char(':')
         + env.value(QStringLiteral("DESKTOP_SESSION")))
            .toLower();
    return desktop.contains(QStringLiteral("kde"))
        || desktop.contains(QStringLiteral("plasma"));
}

/**
 * 把字符串写成 JavaScript 单引号字面量。
 * @param value 原始字符串。
 * @return 可嵌入脚本的字面量。
 */
QString jsStringLiteral(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return QStringLiteral("'%1'").arg(escaped);
}

/**
 * 写入缓存目录中的 KWin 脚本文件。
 * @param source 脚本源码。
 * @return 成功时返回文件路径，失败时返回空字符串。
 */
QString writeScriptFile(const QString &source)
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty()) {
        return {};
    }

    QDir dir(cacheDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return {};
    }

    const QString path = dir.filePath(QStringLiteral("mark-shot-pinned-keep-above.js"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return {};
    }
    if (file.write(source.toUtf8()) != source.toUtf8().size()) {
        return {};
    }
    return path;
}

#ifdef MARK_SHOT_WITH_DBUS
/**
 * 卸载已加载的同名 KWin 脚本。
 * @param scripting Scripting 接口。
 * @return 无返回值。
 */
void unloadExistingScript(QDBusInterface &scripting)
{
    scripting.call(QStringLiteral("unloadScript"),
                   QString::fromLatin1(kPluginName));
}
#endif

}  // namespace

QString kdePinnedKeepAbovePluginName()
{
    return QString::fromLatin1(kPluginName);
}

bool isKdePinnedKeepAboveTitle(const QString &title)
{
    const QString normalized = title.trimmed();
    static const QStringList titles = {
        QStringLiteral("Pinned Mark Shot"),
        QStringLiteral("钉住的截图"),
        QStringLiteral("OCR Result"),
        QStringLiteral("OCR 结果"),
    };
    return titles.contains(normalized);
}

QString kdePinnedKeepAboveScriptSource(const QString &title, bool alwaysOnTop)
{
    const QString keepAbove = alwaysOnTop ? QStringLiteral("true") : QStringLiteral("false");
    const QString titleLiteral = jsStringLiteral(title.trimmed());
    const QString listenAdded = alwaysOnTop ? QStringLiteral("true") : QStringLiteral("false");
    return QStringLiteral(R"JS(
(function() {
    const targetTitle = %1;
    const keepAbove = %2;
    const listenAdded = %3;

    function value(object, key) {
        if (!object) {
            return null;
        }
        const candidate = object[key];
        if (typeof candidate === 'function') {
            return candidate.call(object);
        }
        return candidate;
    }

    function text(object, key) {
        const candidate = value(object, key);
        return typeof candidate === 'string' ? candidate : '';
    }

    function matches(window) {
        if (!window) {
            return false;
        }
        const resourceClass = text(window, 'resourceClass') || text(window, 'windowClass');
        const resourceName = text(window, 'resourceName');
        const caption = text(window, 'caption');
        const classText = (resourceClass + ' ' + resourceName).toLowerCase();
        if (classText.indexOf('mark-shot') < 0) {
            return false;
        }
        return caption === targetTitle;
    }

    function apply(window) {
        if (!matches(window)) {
            return;
        }
        window.keepAbove = keepAbove;
    }

    const windows = (workspace.windowList && workspace.windowList())
        || (workspace.clientList && workspace.clientList())
        || workspace.stackingOrder
        || [];
    for (let i = 0; i < windows.length; i += 1) {
        apply(windows[i]);
    }

    if (listenAdded) {
        const added = workspace.windowAdded || workspace.clientAdded;
        if (added && typeof added.connect === 'function') {
            added.connect(apply);
        }
    }
})();
)JS")
        .arg(titleLiteral, keepAbove, listenAdded);
}

bool applyKdePinnedWindowKeepAbove(const QString &title, bool alwaysOnTop)
{
#ifndef MARK_SHOT_WITH_DBUS
    Q_UNUSED(title)
    Q_UNUSED(alwaysOnTop)
    return false;
#else
    if (!isKdeSession() || !isKdePinnedKeepAboveTitle(title)) {
        return false;
    }

    const QString source = kdePinnedKeepAboveScriptSource(title, alwaysOnTop);
    const QString scriptPath = writeScriptFile(source);
    if (scriptPath.isEmpty()) {
        markshot::debugLog("pinned-window",
                           "【钉图】【KDE置顶】无法写入 KWin 脚本");
        return false;
    }

    QDBusInterface scripting(QStringLiteral("org.kde.KWin"),
                             QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting"),
                             QDBusConnection::sessionBus());
    if (!scripting.isValid()) {
        markshot::debugLog("pinned-window",
                           "【钉图】【KDE置顶】KWin Scripting 接口不可用");
        return false;
    }

    // 1. 先卸掉旧实例，避免重复监听 windowAdded
    unloadExistingScript(scripting);

    // 2. 加载并运行新脚本
    const QDBusReply<int> scriptId =
        scripting.call(QStringLiteral("loadScript"),
                       scriptPath,
                       QString::fromLatin1(kPluginName));
    if (!scriptId.isValid() || scriptId.value() < 0) {
        markshot::debugLog("pinned-window",
                           "【钉图】【KDE置顶】加载脚本失败: %s",
                           scriptId.error().message().toUtf8().constData());
        return false;
    }

    QDBusInterface script(QStringLiteral("org.kde.KWin"),
                          QStringLiteral("/Scripting/Script%1").arg(scriptId.value()),
                          QStringLiteral("org.kde.kwin.Script"),
                          QDBusConnection::sessionBus());
    const QDBusMessage runReply = script.call(QStringLiteral("run"));
    if (runReply.type() == QDBusMessage::ErrorMessage) {
        markshot::debugLog("pinned-window",
                           "【钉图】【KDE置顶】运行脚本失败: %s",
                           runReply.errorMessage().toUtf8().constData());
        unloadExistingScript(scripting);
        return false;
    }

    // 3. 取消置顶只需要一次性改写，随后卸掉脚本
    if (!alwaysOnTop) {
        unloadExistingScript(scripting);
    }
    return true;
#endif
}

}  // namespace markshot::shot

#include "screen_capture_internal.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>

/// @brief Enumerates the geometries of all open X11 windows.
/// @return A vector of rectangles representing the window geometries.
QVector<QRect> enumerateX11WindowGeometries()
{
    QVector<QRect> results;

#ifdef HAVE_XCB
    xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
    if (!connection || xcb_connection_has_error(connection)) {
        if (connection) {
            xcb_disconnect(connection);
        }
        return results;
    }

    const xcb_setup_t *setup = xcb_get_setup(connection);
    if (!setup) {
        xcb_disconnect(connection);
        return results;
    }

    xcb_screen_iterator_t screenIter = xcb_setup_roots_iterator(setup);
    if (!screenIter.data) {
        xcb_disconnect(connection);
        return results;
    }

    xcb_window_t root = screenIter.data->root;
    const QRect rootRect(0, 0, screenIter.data->width_in_pixels, screenIter.data->height_in_pixels);
    const X11WindowAtoms atoms = readX11WindowAtoms(connection);

    QVector<xcb_window_t> managedWindows =
        readX11WindowListProperty(connection, root, atoms.netClientListStacking);
    if (managedWindows.isEmpty()) {
        managedWindows = readX11WindowListProperty(connection, root, atoms.netClientList);
    }
    if (!managedWindows.isEmpty()) {
        for (xcb_window_t window : std::as_const(managedWindows)) {
            if (const std::optional<QRect> rect =
                    x11WindowFrameGeometry(connection, root, window, atoms)) {
                appendUniqueWindowRect(&results, rootRect, *rect);
            }
        }
        xcb_disconnect(connection);
        return results;
    }

    QVector<xcb_window_t> stack;
    stack.append(root);

    while (!stack.isEmpty()) {
        xcb_window_t parent = stack.takeLast();
        xcb_query_tree_cookie_t treeCookie = xcb_query_tree(connection, parent);
        xcb_query_tree_reply_t *treeReply = xcb_query_tree_reply(connection, treeCookie, nullptr);
        if (!treeReply) {
            continue;
        }

        int childCount = xcb_query_tree_children_length(treeReply);
        xcb_window_t *children = xcb_query_tree_children(treeReply);

        for (int i = 0; i < childCount; ++i) {
            xcb_window_t child = children[i];

            xcb_get_window_attributes_cookie_t attrCookie = xcb_get_window_attributes(connection, child);
            xcb_get_window_attributes_reply_t *attrReply = xcb_get_window_attributes_reply(connection, attrCookie, nullptr);
            if (!attrReply) {
                continue;
            }

            const bool isViewable = (attrReply->map_state == XCB_MAP_STATE_VIEWABLE);
            const bool isOverrideRedirect = attrReply->override_redirect != 0;
            std::free(attrReply);

            if (!isViewable || isOverrideRedirect || x11WindowIsHiddenOrIconic(connection, child, atoms)) {
                continue;
            }

            xcb_get_geometry_cookie_t geoCookie = xcb_get_geometry(connection, child);
            xcb_get_geometry_reply_t *geoReply = xcb_get_geometry_reply(connection, geoCookie, nullptr);
            if (!geoReply) {
                continue;
            }

            xcb_translate_coordinates_cookie_t transCookie = xcb_translate_coordinates(connection, child, root, 0, 0);
            xcb_translate_coordinates_reply_t *transReply = xcb_translate_coordinates_reply(connection, transCookie, nullptr);

            if (transReply) {
                int x = transReply->dst_x;
                int y = transReply->dst_y;
                int w = geoReply->width;
                int h = geoReply->height;
                std::free(transReply);

                appendUniqueWindowRect(&results, rootRect, QRect(x, y, w, h));
            }

            std::free(geoReply);
            stack.append(child);
        }

        std::free(treeReply);
    }

    xcb_disconnect(connection);
#endif

    return results;
}

/// @brief Enumerates the info of all open X11 windows.
/// @return A vector of WindowInfo with z-order based on _NET_CLIENT_LIST_STACKING order (bottom-to-top).
QVector<markshot::WindowInfo> enumerateX11WindowInfos()
{
    QVector<markshot::WindowInfo> results;
    const QVector<QRect> geometries = enumerateX11WindowGeometries();
    results.reserve(geometries.size());
    for (int i = 0; i < geometries.size(); ++i) {
        results.append(markshot::WindowInfo{geometries[i], i});
    }
    return results;
}

bool isGnomeWaylandSession()
{
#ifdef MARK_SHOT_WITH_DBUS
    // 判定结果在进程生命周期内不变；滚动捕获热路径每 tick 都会询问
    static const bool gnome = [] {
        if (!isWaylandSession()) {
            return false;
        }
        return desktopEnvironmentText().toLower().contains(QStringLiteral("gnome"));
    }();
    return gnome;
#else
    return false;
#endif
}

#ifdef MARK_SHOT_WITH_DBUS

namespace {

// GNOME helper 版本探测的缓存时长与失败退避时长
constexpr int kGnomeHelperProbeCacheMs = 5000;
constexpr int kGnomeHelperFailureBackoffMs = 30000;

/**
 * 读取带 TTL 与失败退避的 GNOME helper 版本缓存。
 * @param major 探测到的新版本号；空值表示只读缓存。
 * @return 缓存有效期内的版本号，过期、失败退避中或未探测时返回空值。
 */
std::optional<int> gnomeHelperVersionCache(std::optional<int> major)
{
    static QMutex mutex;
    static QElapsedTimer cacheTimer;
    static QElapsedTimer failureTimer;
    static int cachedVersion = 0;
    static bool failed = false;

    QMutexLocker locker(&mutex);
    if (major.has_value()) {
        cachedVersion = *major;
        // 1. 版本号为 0 表示扩展缺失，进入失败退避
        failed = cachedVersion <= 0;
        if (failed) {
            failureTimer.restart();
            cacheTimer.invalidate();
        } else {
            // 2. 成功探测刷新普通 TTL 缓存
            failureTimer.invalidate();
            cacheTimer.restart();
        }
        return major;
    }
    if (failed && failureTimer.isValid()
        && failureTimer.elapsed() < kGnomeHelperFailureBackoffMs) {
        return 0;
    }
    if (!cacheTimer.isValid() || cacheTimer.elapsed() >= kGnomeHelperProbeCacheMs) {
        return std::nullopt;
    }
    return cachedVersion;
}

/**
 * 从 D-Bus 应答解析 GNOME helper 主版本号。
 * @param reply Version 调用的应答。
 * @return 扩展不可用或应答异常时返回 0。
 */
int gnomeScrollHelperMajorVersionFromReply(const QDBusMessage &reply)
{
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
        return 0;
    }
    const QString version = reply.arguments().first().toString();
    bool ok = false;
    const int major = version.section(QLatin1Char('.'), 0, 0).toInt(&ok);
    return ok ? major : 0;
}

}  // namespace

/**
 * 探测 GNOME helper 主版本号并写入缓存。
 * @return 扩展不可用时返回 0，否则返回语义版本中的主版本号。
 */
int gnomeHelperVersionFromSession()
{
    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.gnome.Shell"),
        QStringLiteral("/org/gnome/Shell/Extensions/MarkShotScrollHelper"),
        QStringLiteral("org.gnome.Shell.Extensions.MarkShotScrollHelper"),
        QStringLiteral("Version"));
    const QDBusMessage reply = QDBusConnection::sessionBus().call(message, QDBus::Block, 3000);
    const int major = gnomeScrollHelperMajorVersionFromReply(reply);
    gnomeHelperVersionCache(major);
    return major;
}

#endif

bool hasGnomeScrollHelper()
{
#ifdef MARK_SHOT_WITH_DBUS
    // 1. 命中缓存时跳过 D-Bus 往返；扩展未安装时按失败退避避免逐帧探测
    if (const std::optional<int> cached = gnomeHelperVersionCache(std::nullopt)) {
        return *cached > 0;
    }

    // 2. 未命中缓存时探测一次并写回缓存
    return gnomeHelperVersionFromSession() > 0;
#else
    return false;
#endif
}

/// @brief 读取 GNOME Shell 滚动截图扩展的主版本号。
/// @return 扩展不可用时返回 0，否则返回语义版本中的主版本号。
int gnomeScrollHelperMajorVersion()
{
#ifdef MARK_SHOT_WITH_DBUS
    // 命中缓存时跳过探测，滚动捕获热路径不再逐帧做 D-Bus 往返
    if (const std::optional<int> cached = gnomeHelperVersionCache(std::nullopt)) {
        return *cached;
    }
    return gnomeHelperVersionFromSession();
#else
    return 0;
#endif
}

bool hasGnomeScrollPreviewHelper()
{
#ifdef MARK_SHOT_WITH_DBUS
    return gnomeScrollHelperMajorVersion() >= 3;
#else
    return false;
#endif
}

bool hasGnomeScrollOverlayHelper()
{
#ifdef MARK_SHOT_WITH_DBUS
    return gnomeScrollHelperMajorVersion() >= 5;
#else
    return false;
#endif
}

/// @brief Captures a screen area using the GNOME Shell extension scroll helper.
/// @param request The capture request specifying the target geometry.
/// @return The result of the capture operation.
CaptureResult captureWithGnomeScrollHelper(const CaptureRequest &request)
{
#ifdef MARK_SHOT_WITH_DBUS
    const QString tempDir = QFile::exists(QStringLiteral("/dev/shm"))
        ? QStringLiteral("/dev/shm")
        : QDir::tempPath();
    const QString tempPath = QStringLiteral("%1/mark-shot-scroll-frame-%2.png")
        .arg(tempDir, QUuid::createUuid().toString(QUuid::Id128));

    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.gnome.Shell"),
        QStringLiteral("/org/gnome/Shell/Extensions/MarkShotScrollHelper"),
        QStringLiteral("org.gnome.Shell.Extensions.MarkShotScrollHelper"),
        QStringLiteral("ScreenshotArea")
    );
    message << request.sourceGeometry.x()
            << request.sourceGeometry.y()
            << request.sourceGeometry.width()
            << request.sourceGeometry.height()
            << tempPath;

    QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QFile::remove(tempPath);
        return {{}, reply.errorMessage(), {}, request.sourceGeometry};
    }

    QList<QVariant> args = reply.arguments();
    if (args.size() < 2 || !args.at(0).toBool()) {
        QFile::remove(tempPath);
        return {{}, QStringLiteral("Failed to capture area via GNOME Shell extension"), {}, request.sourceGeometry};
    }

    QString actualPath = args.at(1).toString();
    QImage img(actualPath);
    if (img.isNull()) {
        return {{}, QStringLiteral("Failed to load captured frame from %1").arg(actualPath), {}, request.sourceGeometry};
    }

    QFile::remove(actualPath);
    return {img, {}, {}, request.sourceGeometry};
#else
    return {{}, QStringLiteral("GNOME scroll helper support was not enabled at build time"), {}, request.sourceGeometry};
#endif
}

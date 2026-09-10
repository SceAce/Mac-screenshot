#include "screen_capture_internal.h"

#ifdef HAVE_XCB

constexpr std::uint32_t kWmStateIconic = 3;

xcb_atom_t internX11Atom(xcb_connection_t *connection, const char *name)
{
    if (!connection || !name) {
        return XCB_ATOM_NONE;
    }

    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(connection, 0, static_cast<uint16_t>(std::strlen(name)), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    const xcb_atom_t atom = reply ? reply->atom : XCB_ATOM_NONE;
    std::free(reply);
    return atom;
}

X11WindowAtoms readX11WindowAtoms(xcb_connection_t *connection)
{
    X11WindowAtoms atoms;
    atoms.netClientListStacking = internX11Atom(connection, "_NET_CLIENT_LIST_STACKING");
    atoms.netClientList = internX11Atom(connection, "_NET_CLIENT_LIST");
    atoms.netWmState = internX11Atom(connection, "_NET_WM_STATE");
    atoms.netWmStateHidden = internX11Atom(connection, "_NET_WM_STATE_HIDDEN");
    atoms.netFrameExtents = internX11Atom(connection, "_NET_FRAME_EXTENTS");
    atoms.wmState = internX11Atom(connection, "WM_STATE");
    return atoms;
}

QVector<xcb_window_t> readX11WindowListProperty(xcb_connection_t *connection,
                                                xcb_window_t window,
                                                xcb_atom_t property)
{
    QVector<xcb_window_t> windows;
    if (!connection || property == XCB_ATOM_NONE) {
        return windows;
    }

    xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, 0, window, property, XCB_ATOM_WINDOW, 0, 4096);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply) {
        return windows;
    }

    if (reply->format == 32 && reply->type == XCB_ATOM_WINDOW) {
        const int count = xcb_get_property_value_length(reply) / static_cast<int>(sizeof(xcb_window_t));
        const xcb_window_t *values =
            static_cast<const xcb_window_t *>(xcb_get_property_value(reply));
        windows.reserve(count);
        for (int i = 0; i < count; ++i) {
            if (values[i] != XCB_WINDOW_NONE) {
                windows.append(values[i]);
            }
        }
    }

    std::free(reply);
    return windows;
}

QVector<xcb_atom_t> readX11AtomListProperty(xcb_connection_t *connection,
                                            xcb_window_t window,
                                            xcb_atom_t property)
{
    QVector<xcb_atom_t> atoms;
    if (!connection || property == XCB_ATOM_NONE) {
        return atoms;
    }

    xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, 0, window, property, XCB_ATOM_ATOM, 0, 4096);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply) {
        return atoms;
    }

    if (reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const int count = xcb_get_property_value_length(reply) / static_cast<int>(sizeof(xcb_atom_t));
        const xcb_atom_t *values =
            static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        atoms.reserve(count);
        for (int i = 0; i < count; ++i) {
            atoms.append(values[i]);
        }
    }

    std::free(reply);
    return atoms;
}

QVector<std::uint32_t> readX11CardinalListProperty(xcb_connection_t *connection,
                                                   xcb_window_t window,
                                                   xcb_atom_t property,
                                                   uint32_t maxValues)
{
    QVector<std::uint32_t> values;
    if (!connection || property == XCB_ATOM_NONE || maxValues == 0) {
        return values;
    }

    xcb_get_property_cookie_t cookie =
        xcb_get_property(connection, 0, window, property, XCB_ATOM_CARDINAL, 0, maxValues);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply) {
        return values;
    }

    if (reply->format == 32 && reply->type == XCB_ATOM_CARDINAL) {
        const int count = xcb_get_property_value_length(reply) / static_cast<int>(sizeof(std::uint32_t));
        const auto *raw = static_cast<const std::uint32_t *>(xcb_get_property_value(reply));
        values.reserve(count);
        for (int i = 0; i < count; ++i) {
            values.append(raw[i]);
        }
    }

    std::free(reply);
    return values;
}

bool x11WindowHasState(xcb_connection_t *connection,
                       xcb_window_t window,
                       const X11WindowAtoms &atoms,
                       xcb_atom_t state)
{
    if (state == XCB_ATOM_NONE) {
        return false;
    }
    const QVector<xcb_atom_t> states =
        readX11AtomListProperty(connection, window, atoms.netWmState);
    return states.contains(state);
}

bool x11WindowIsIconic(xcb_connection_t *connection,
                       xcb_window_t window,
                       const X11WindowAtoms &atoms)
{
    const QVector<std::uint32_t> values =
        readX11CardinalListProperty(connection, window, atoms.wmState, 2);
    return !values.isEmpty() && values.first() == kWmStateIconic;
}

bool x11WindowIsHiddenOrIconic(xcb_connection_t *connection,
                               xcb_window_t window,
                               const X11WindowAtoms &atoms)
{
    return x11WindowHasState(connection, window, atoms, atoms.netWmStateHidden)
        || x11WindowIsIconic(connection, window, atoms);
}

std::optional<QRect> x11WindowFrameGeometry(xcb_connection_t *connection,
                                            xcb_window_t root,
                                            xcb_window_t window,
                                            const X11WindowAtoms &atoms)
{
    if (!connection || window == XCB_WINDOW_NONE || x11WindowIsHiddenOrIconic(connection, window, atoms)) {
        return std::nullopt;
    }

    xcb_get_window_attributes_cookie_t attrCookie = xcb_get_window_attributes(connection, window);
    xcb_get_window_attributes_reply_t *attrReply =
        xcb_get_window_attributes_reply(connection, attrCookie, nullptr);
    if (!attrReply) {
        return std::nullopt;
    }
    const bool isViewable = attrReply->map_state == XCB_MAP_STATE_VIEWABLE;
    const bool isOverrideRedirect = attrReply->override_redirect != 0;
    std::free(attrReply);
    if (!isViewable || isOverrideRedirect) {
        return std::nullopt;
    }

    xcb_get_geometry_cookie_t geoCookie = xcb_get_geometry(connection, window);
    xcb_get_geometry_reply_t *geoReply = xcb_get_geometry_reply(connection, geoCookie, nullptr);
    if (!geoReply) {
        return std::nullopt;
    }

    xcb_translate_coordinates_cookie_t transCookie =
        xcb_translate_coordinates(connection, window, root, 0, 0);
    xcb_translate_coordinates_reply_t *transReply =
        xcb_translate_coordinates_reply(connection, transCookie, nullptr);
    if (!transReply) {
        std::free(geoReply);
        return std::nullopt;
    }

    QRect rect(transReply->dst_x, transReply->dst_y, geoReply->width, geoReply->height);
    std::free(transReply);
    std::free(geoReply);

    const QVector<std::uint32_t> extents =
        readX11CardinalListProperty(connection, window, atoms.netFrameExtents, 4);
    if (extents.size() >= 4) {
        const int left = static_cast<int>(std::min<std::uint32_t>(extents.at(0), 16384));
        const int right = static_cast<int>(std::min<std::uint32_t>(extents.at(1), 16384));
        const int top = static_cast<int>(std::min<std::uint32_t>(extents.at(2), 16384));
        const int bottom = static_cast<int>(std::min<std::uint32_t>(extents.at(3), 16384));
        rect.adjust(-left, -top, right, bottom);
    }

    return rect.normalized();
}

void appendUniqueWindowRect(QVector<QRect> *results, const QRect &screenRect, QRect rect)
{
    if (!results) {
        return;
    }
    rect = rect.normalized();
    if (rect.width() <= 1 || rect.height() <= 1 || !rect.intersects(screenRect)) {
        return;
    }
    if (!results->contains(rect)) {
        results->append(rect);
    }
}

#endif


#include "screen_capture_pipewire_libportal.h"

#ifdef HAVE_LIBPORTAL

QString glibErrorText(const GError *error)
{
    return error && error->message
        ? QString::fromUtf8(error->message)
        : QStringLiteral("unknown libportal error");
}

QVariant gVariantToQVariant(GVariant *value)
{
    if (!value) {
        return {};
    }

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT)) {
        GVariant *nested = g_variant_get_variant(value);
        const QVariant converted = gVariantToQVariant(nested);
        g_variant_unref(nested);
        return converted;
    }

    switch (g_variant_classify(value)) {
    case G_VARIANT_CLASS_BOOLEAN:
        return static_cast<bool>(g_variant_get_boolean(value));
    case G_VARIANT_CLASS_BYTE:
        return static_cast<int>(g_variant_get_byte(value));
    case G_VARIANT_CLASS_INT16:
        return static_cast<int>(g_variant_get_int16(value));
    case G_VARIANT_CLASS_UINT16:
        return static_cast<uint>(g_variant_get_uint16(value));
    case G_VARIANT_CLASS_INT32:
        return g_variant_get_int32(value);
    case G_VARIANT_CLASS_UINT32:
        return g_variant_get_uint32(value);
    case G_VARIANT_CLASS_INT64:
        return static_cast<qlonglong>(g_variant_get_int64(value));
    case G_VARIANT_CLASS_UINT64:
        return static_cast<qulonglong>(g_variant_get_uint64(value));
    case G_VARIANT_CLASS_HANDLE:
        return g_variant_get_handle(value);
    case G_VARIANT_CLASS_DOUBLE:
        return g_variant_get_double(value);
    case G_VARIANT_CLASS_STRING:
    case G_VARIANT_CLASS_OBJECT_PATH:
    case G_VARIANT_CLASS_SIGNATURE:
        return QString::fromUtf8(g_variant_get_string(value, nullptr));
    case G_VARIANT_CLASS_ARRAY:
    case G_VARIANT_CLASS_TUPLE: {
        QVariantList list;
        const gsize childCount = g_variant_n_children(value);
        list.reserve(static_cast<int>(childCount));
        for (gsize i = 0; i < childCount; ++i) {
            GVariant *child = g_variant_get_child_value(value, i);
            list.push_back(gVariantToQVariant(child));
            g_variant_unref(child);
        }
        return list;
    }
    default:
        break;
    }

    gchar *printed = g_variant_print(value, TRUE);
    const QString fallback = printed
        ? QString::fromUtf8(printed)
        : QStringLiteral("<unprintable>");
    g_free(printed);
    return fallback;
}

QVariantMap gVariantDictionaryToVariantMap(GVariant *dictionary)
{
    QVariantMap map;
    if (!dictionary) {
        return map;
    }

    const gsize childCount = g_variant_n_children(dictionary);
    for (gsize i = 0; i < childCount; ++i) {
        GVariant *entry = g_variant_get_child_value(dictionary, i);
        if (!entry || g_variant_n_children(entry) < 2) {
            if (entry) {
                g_variant_unref(entry);
            }
            continue;
        }

        GVariant *key = g_variant_get_child_value(entry, 0);
        GVariant *value = g_variant_get_child_value(entry, 1);
        if (key && g_variant_is_of_type(key, G_VARIANT_TYPE_STRING)) {
            map.insert(QString::fromUtf8(g_variant_get_string(key, nullptr)),
                       gVariantToQVariant(value));
        }
        if (value) {
            g_variant_unref(value);
        }
        if (key) {
            g_variant_unref(key);
        }
        g_variant_unref(entry);
    }
    return map;
}

bool readLibportalStream(GVariant *streams,
                         uint *nodeId,
                         QVariantMap *properties,
                         QString *error)
{
    if (!streams || g_variant_n_children(streams) == 0) {
        if (error) {
            *error = QStringLiteral("libportal ScreenCast Start returned no PipeWire stream");
        }
        return false;
    }

    GVariant *stream = g_variant_get_child_value(streams, 0);
    if (!stream || g_variant_n_children(stream) < 2) {
        if (stream) {
            g_variant_unref(stream);
        }
        if (error) {
            *error = QStringLiteral("libportal returned an invalid stream descriptor");
        }
        return false;
    }

    GVariant *nodeValue = g_variant_get_child_value(stream, 0);
    GVariant *propertiesValue = g_variant_get_child_value(stream, 1);
    const bool nodeOk = nodeValue && g_variant_is_of_type(nodeValue, G_VARIANT_TYPE_UINT32);
    if (nodeOk && nodeId) {
        *nodeId = g_variant_get_uint32(nodeValue);
    }
    if (propertiesValue && properties) {
        *properties = gVariantDictionaryToVariantMap(propertiesValue);
    }

    if (propertiesValue) {
        g_variant_unref(propertiesValue);
    }
    if (nodeValue) {
        g_variant_unref(nodeValue);
    }
    g_variant_unref(stream);

    if (!nodeOk || !nodeId || *nodeId == 0) {
        if (error) {
            *error = QStringLiteral("libportal returned a PipeWire stream without a node id");
        }
        return false;
    }
    return true;
}

LibportalOperation::LibportalOperation(GMainContext *context, GCancellable *cancellable)
    : m_loop(g_main_loop_new(context, FALSE))
    , m_cancellable(cancellable)
{
    m_timeoutSource = g_timeout_source_new_seconds(120);
    g_source_set_callback(m_timeoutSource, &LibportalOperation::timeoutCallback, this, nullptr);
    g_source_attach(m_timeoutSource, context);
}

LibportalOperation::~LibportalOperation()
{
    if (m_timeoutSource) {
        if (!g_source_is_destroyed(m_timeoutSource)) {
            g_source_destroy(m_timeoutSource);
        }
        g_source_unref(m_timeoutSource);
    }
    if (m_loop) {
        g_main_loop_unref(m_loop);
    }
}

GMainLoop *LibportalOperation::loop() const
{
    return m_loop;
}

void LibportalOperation::finish()
{
    m_done = true;
    if (m_loop) {
        g_main_loop_quit(m_loop);
    }
}

bool LibportalOperation::wait(const QString &operation, QString *error)
{
    if (!m_done && m_loop) {
        g_main_loop_run(m_loop);
    }
    if (m_timedOut) {
        if (error) {
            *error = QStringLiteral("%1 timed out").arg(operation);
        }
        return false;
    }
    return true;
}

gboolean LibportalOperation::timeoutCallback(gpointer data)
{
    auto *self = static_cast<LibportalOperation *>(data);
    if (!self) {
        return G_SOURCE_REMOVE;
    }
    self->m_timedOut = true;
    if (self->m_cancellable) {
        g_cancellable_cancel(self->m_cancellable);
    }
    return G_SOURCE_REMOVE;
}

void onLibportalCreateScreencastFinished(GObject *object,
                                         GAsyncResult *result,
                                         gpointer data)
{
    auto *call = static_cast<LibportalCreateCall *>(data);
    if (!call || !call->operation) {
        return;
    }
    call->session = xdp_portal_create_screencast_session_finish(XDP_PORTAL(object),
                                                                result,
                                                                &call->error);
    call->operation->finish();
}

void onLibportalSessionStartFinished(GObject *object,
                                     GAsyncResult *result,
                                     gpointer data)
{
    auto *call = static_cast<LibportalStartCall *>(data);
    if (!call || !call->operation) {
        return;
    }
    call->ok = xdp_session_start_finish(XDP_SESSION(object), result, &call->error);
    call->operation->finish();
}

#endif  // HAVE_LIBPORTAL

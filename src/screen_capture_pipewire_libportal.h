#pragma once

#include "screen_capture_internal.h"

#if defined(HAVE_PIPEWIRE) && defined(HAVE_LIBPORTAL)

QString glibErrorText(const GError *error);
QVariant gVariantToQVariant(GVariant *value);
QVariantMap gVariantDictionaryToVariantMap(GVariant *dictionary);
bool readLibportalStream(GVariant *streams,
                         uint *nodeId,
                         QVariantMap *properties,
                         QString *error);

class LibportalOperation final {
public:
    LibportalOperation(GMainContext *context, GCancellable *cancellable);
    ~LibportalOperation();

    GMainLoop *loop() const;
    void finish();
    bool wait(const QString &operation, QString *error);

private:
    static gboolean timeoutCallback(gpointer data);

    GMainLoop *m_loop = nullptr;
    GCancellable *m_cancellable = nullptr;
    GSource *m_timeoutSource = nullptr;
    bool m_timedOut = false;
    bool m_done = false;
};

struct LibportalCreateCall {
    LibportalOperation *operation = nullptr;
    XdpSession *session = nullptr;
    GError *error = nullptr;
};

void onLibportalCreateScreencastFinished(GObject *object,
                                         GAsyncResult *result,
                                         gpointer data);

struct LibportalStartCall {
    LibportalOperation *operation = nullptr;
    gboolean ok = FALSE;
    GError *error = nullptr;
};

void onLibportalSessionStartFinished(GObject *object,
                                     GAsyncResult *result,
                                     gpointer data);

#endif

#include "screen_capture_internal.h"

#ifdef MARK_SHOT_WITH_DBUS

QDBusArgument &operator<<(QDBusArgument &argument, const PortalStream &stream)
{
    argument.beginStructure();
    argument << stream.nodeId << stream.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalStream &stream)
{
    argument.beginStructure();
    argument >> stream.nodeId >> stream.properties;
    argument.endStructure();
    return argument;
}

#endif

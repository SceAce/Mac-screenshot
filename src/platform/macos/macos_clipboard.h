#pragma once

class QByteArray;

#ifdef __OBJC__
@class NSPasteboard;
#endif

namespace markshot::macos {

bool copyPngToClipboard(const QByteArray &png);

#ifdef __OBJC__
// Also accepts a named pasteboard so integration tests can avoid the user's clipboard.
bool writePngToPasteboard(NSPasteboard *pasteboard, const QByteArray &png);
#endif

} // namespace markshot::macos

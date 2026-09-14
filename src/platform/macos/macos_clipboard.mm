#import <AppKit/AppKit.h>

#include "macos_clipboard.h"

#include <QByteArray>

namespace markshot::macos {

bool writePngToPasteboard(NSPasteboard *pasteboard, const QByteArray &png)
{
    @autoreleasepool {
        if (!pasteboard || png.isEmpty()) {
            return false;
        }

        NSData *pngData = [NSData dataWithBytes:png.constData() length:png.size()];
        NSBitmapImageRep *bitmap = [NSBitmapImageRep imageRepWithData:pngData];
        if (!bitmap) {
            return false;
        }

        // QClipboard::setImage publishes only uncompressed TIFF on Cocoa, which
        // becomes very large for scrolling captures. A Qt image/png MIME entry
        // also maps to a private Qt type, rather than the native public.png UTI.
        // Publish PNG first and a lossless compressed TIFF for older consumers.
        NSPasteboardItem *item = [[NSPasteboardItem alloc] init];
        if (![item setData:pngData forType:NSPasteboardTypePNG]) {
            return false;
        }
        NSData *tiff = [bitmap representationUsingType:NSBitmapImageFileTypeTIFF
                                          properties:@{NSImageCompressionMethod: @(NSTIFFCompressionLZW)}];
        if (tiff.length) {
            [item setData:tiff forType:NSPasteboardTypeTIFF];
        }

        // Materialize the data before closing the capture window or exiting.
        // Pasteboard services own both representations after writeObjects returns.
        [pasteboard clearContents];
        return [pasteboard writeObjects:@[item]];
    }
}

bool copyPngToClipboard(const QByteArray &png)
{
    return writePngToPasteboard(NSPasteboard.generalPasteboard, png);
}

} // namespace markshot::macos

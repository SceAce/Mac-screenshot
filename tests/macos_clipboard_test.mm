#import <AppKit/AppKit.h>

#include "platform/macos/macos_clipboard.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QImage>
#include <QProcess>
#include <QScopeGuard>
#include <QtTest>

namespace {

QImage fixture(QSize size)
{
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    for (int y = 0; y < size.height(); ++y) {
        image.setPixelColor(size.width() / 2, y, QColor(y % 256, (y / 256) % 256, 127));
    }
    image.setPixelColor(0, 0, Qt::red);
    image.setPixelColor(size.width() - 1, size.height() - 1, Qt::blue);
    return image;
}

QByteArray encodePng(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    return image.save(&buffer, "PNG") ? bytes : QByteArray();
}

QByteArray bytes(NSData *data)
{
    return QByteArray(static_cast<const char *>(data.bytes), data.length);
}

} // namespace

class MacosClipboardTest : public QObject {
    Q_OBJECT

private slots:
    void nativeImagesSurviveWriterExit_data()
    {
        QTest::addColumn<QSize>("size");
        QTest::newRow("ordinary") << QSize(800, 600);
        QTest::newRow("several-retina-screens") << QSize(1200, 18000);
        QTest::newRow("very-tall") << QSize(256, 80000);
        QTest::newRow("horizontal") << QSize(40000, 256);
    }

    void nativeImagesSurviveWriterExit()
    {
        QFETCH(QSize, size);
        @autoreleasepool {
            NSPasteboard *board = [NSPasteboard pasteboardWithUniqueName];
            if (!board) {
                QSKIP("macOS pasteboard service is unavailable in this test environment");
            }
            const auto cleanup = qScopeGuard([&] { [board releaseGlobally]; });
            QProcess writer;
            writer.start(QCoreApplication::applicationFilePath(),
                         {QStringLiteral("--write-pasteboard"), QString::fromNSString(board.name),
                          QString::number(size.width()), QString::number(size.height())});
            QVERIFY(writer.waitForFinished(30000));
            QCOMPARE(writer.exitStatus(), QProcess::NormalExit);
            QCOMPARE(writer.exitCode(), 0);

            QVERIFY([board.types containsObject:NSPasteboardTypePNG]);
            QVERIFY([board.types containsObject:NSPasteboardTypeTIFF]);
            const QImage expected = fixture(size);
            const QByteArray png = bytes([board dataForType:NSPasteboardTypePNG]);
            QCOMPARE(png, encodePng(expected));
            const QImage decoded = QImage::fromData(png, "PNG");
            QCOMPARE(decoded.size(), size);
            QCOMPARE(decoded.pixelColor(0, 0), QColor(Qt::red));
            QCOMPARE(decoded.pixelColor(size.width() - 1, size.height() - 1), QColor(Qt::blue));

            NSData *tiff = [board dataForType:NSPasteboardTypeTIFF];
            QVERIFY(tiff.length > 0);
            // This mostly flat fixture must not regress to a raw 80+ MB TIFF.
            QVERIFY(tiff.length < static_cast<NSUInteger>(expected.sizeInBytes() / 4));
            NSBitmapImageRep *native = [NSBitmapImageRep imageRepWithData:tiff];
            QCOMPARE(native.pixelsWide, size.width());
            QCOMPARE(native.pixelsHigh, size.height());
            QCOMPARE(native.bitsPerSample, 8);
            QVERIFY(native.samplesPerPixel >= 3 && native.samplesPerPixel <= 4);
            NSUInteger lastPixel[4] = {};
            [native getPixel:lastPixel atX:size.width() - 1 y:size.height() - 1];
            QCOMPARE(lastPixel[0], NSUInteger(0));
            QCOMPARE(lastPixel[1], NSUInteger(0));
            QCOMPARE(lastPixel[2], NSUInteger(255));
            NSImage *pasted = [[NSImage alloc] initWithPasteboard:board];
            QVERIFY(pasted && pasted.valid);
        }
    }

    void invalidInputPreservesPasteboard()
    {
        @autoreleasepool {
            NSPasteboard *board = [NSPasteboard pasteboardWithUniqueName];
            if (!board) {
                QSKIP("macOS pasteboard service is unavailable in this test environment");
            }
            const auto cleanup = qScopeGuard([&] { [board releaseGlobally]; });
            [board setString:@"keep existing contents" forType:NSPasteboardTypeString];
            const NSInteger count = board.changeCount;
            QVERIFY(!markshot::macos::writePngToPasteboard(board, {}));
            QVERIFY(!markshot::macos::writePngToPasteboard(board, QByteArray("invalid image")));
            QCOMPARE(board.changeCount, count);
            QVERIFY([[board stringForType:NSPasteboardTypeString] isEqualToString:@"keep existing contents"]);
        }
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() == 5 && args.at(1) == QStringLiteral("--write-pasteboard")) {
        @autoreleasepool {
            NSPasteboard *board = [NSPasteboard pasteboardWithName:args.at(2).toNSString()];
            const QByteArray png = encodePng(fixture(QSize(args.at(3).toInt(), args.at(4).toInt())));
            return markshot::macos::writePngToPasteboard(board, png) ? 0 : 1;
        }
    }
    MacosClipboardTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "macos_clipboard_test.moc"

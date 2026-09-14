#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include "macos_capture.h"
#include "ui/i18n.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace markshot::macos {

bool screenCapturePermissionGranted()
{
    return CGPreflightScreenCaptureAccess();
}

void openScreenCaptureSettings()
{
    [[NSWorkspace sharedWorkspace] openURL:
        [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
}

bool ensureScreenCapturePermission(QWidget *parent)
{
    if (screenCapturePermissionGranted()) {
        return true;
    }
    QDialog dialog(parent);
    dialog.setWindowTitle(MS_TR("Screen Recording Permission"));
    dialog.setMinimumWidth(480);
    auto *layout = new QVBoxLayout(&dialog);
    auto *description = new QLabel(MS_TR(
        "Mark Shot needs Screen Recording permission to capture your screen.\n\n"
        "1. Click Request Permission.\n"
        "2. Enable Mark Shot in System Settings > Privacy & Security > Screen & System Audio Recording.\n"
        "3. Return here to continue. If macOS asks you to quit and reopen Mark Shot, do so.\n\n"
        "Screenshots stay on your Mac unless you choose to upload them."), &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);
    auto *status = new QLabel(MS_TR("Screen Recording: not authorized"), &dialog);
    layout->addWidget(status);
    auto *buttons = new QDialogButtonBox(&dialog);
    auto *request = buttons->addButton(MS_TR("Request Permission"), QDialogButtonBox::ActionRole);
    auto *settings = buttons->addButton(MS_TR("Open System Settings"), QDialogButtonBox::ActionRole);
    auto *retry = buttons->addButton(MS_TR("Check Again"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    auto check = [&dialog, status] {
        if (screenCapturePermissionGranted()) {
            dialog.accept();
        } else {
            status->setText(MS_TR("Not authorized yet. Enable Mark Shot in System Settings; you may need to reopen the app."));
        }
    };
    QObject::connect(request, &QPushButton::clicked, &dialog, [request, check] {
        request->setEnabled(false);
        CGRequestScreenCaptureAccess();
        check();
        request->setEnabled(true);
    });
    QObject::connect(settings, &QPushButton::clicked, &dialog, [] { openScreenCaptureSettings(); });
    QObject::connect(retry, &QPushButton::clicked, &dialog, check);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QTimer timer;
    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, &dialog, [&dialog] {
        if (screenCapturePermissionGranted()) {
            dialog.accept();
        }
    });
    timer.start();
    [NSApp activateIgnoringOtherApps:YES];
    return dialog.exec() == QDialog::Accepted && screenCapturePermissionGranted();
}

} // namespace markshot::macos

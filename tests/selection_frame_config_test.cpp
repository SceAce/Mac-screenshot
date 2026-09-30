#include "selection_frame_config.h"

#include <QJsonObject>
#include <QtTest/QtTest>

class SelectionFrameConfigTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultsToCurrentAccent()
    {
        QCOMPARE(markshot::defaultSelectionFrameColor(), QColor(94, 234, 212));
        QCOMPARE(markshot::selectionFrameColorFromConfigRoot(QJsonObject()),
                 QColor(94, 234, 212));
    }

    void readsConfiguredColor()
    {
        QJsonObject capture;
        capture.insert(QStringLiteral("selectionColor"), QStringLiteral("#F43F5E"));
        QJsonObject root;
        root.insert(QStringLiteral("capture"), capture);

        QCOMPARE(markshot::selectionFrameColorFromConfigRoot(root), QColor(244, 63, 94));
    }

    void rejectsInvalidColor()
    {
        QJsonObject capture;
        capture.insert(QStringLiteral("selectionColor"), QStringLiteral("not-a-color"));
        QJsonObject root;
        root.insert(QStringLiteral("capture"), capture);

        QCOMPARE(markshot::selectionFrameColorFromConfigRoot(root),
                 markshot::defaultSelectionFrameColor());
    }
};

QTEST_APPLESS_MAIN(SelectionFrameConfigTest)

#include "selection_frame_config_test.moc"

// SPDX-FileCopyrightText: 2026 Rotation Suggestions contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rotationmapping.h"

#include <QTest>

class RotationMappingTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void targetRotation_data();
    void targetRotation();
    void direction_data();
    void direction();
};

void RotationMappingTest::targetRotation_data()
{
    QTest::addColumn<QString>("panelMode");
    QTest::addColumn<QString>("orientation");
    QTest::addColumn<QString>("expected");

    QTest::newRow("landscape normal") << "1280x720" << "normal" << "normal";
    QTest::newRow("landscape left-up") << "1280x720" << "left-up" << "left";
    QTest::newRow("landscape right-up") << "1280x720" << "right-up" << "right";
    QTest::newRow("landscape bottom-up") << "1280x720" << "bottom-up" << "inverted";
    QTest::newRow("portrait normal") << "720x1280" << "normal" << "left";
    QTest::newRow("portrait left-up") << "720x1280" << "left-up" << "inverted";
    QTest::newRow("portrait right-up") << "720x1280" << "right-up" << "normal";
    QTest::newRow("portrait bottom-up") << "720x1280" << "bottom-up" << "right";
    QTest::newRow("unknown orientation") << "1280x720" << "undefined" << QString();
    QTest::newRow("malformed mode") << "1280" << "normal" << QString();
}
void RotationMappingTest::targetRotation()
{
    QFETCH(QString, panelMode);
    QFETCH(QString, orientation);
    QFETCH(QString, expected);

    QCOMPARE(RotationMapping::targetRotation(panelMode, orientation), expected);
}

void RotationMappingTest::direction_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("current");
    QTest::addColumn<QString>("expected");

    QTest::newRow("same") << "normal" << "normal" << QString();
    QTest::newRow("clockwise") << "right" << "normal" << "right";
    QTest::newRow("counter-clockwise") << "left" << "normal" << "left";
    QTest::newRow("upside down") << "inverted" << "normal" << "flip";
    QTest::newRow("wrap to right") << "normal" << "left" << "right";
    QTest::newRow("wrap to left") << "normal" << "right" << "left";
    QTest::newRow("opposite") << "right" << "left" << "flip";
    QTest::newRow("unknown target") << QString() << "normal" << QString();
    QTest::newRow("unknown current") << "normal" << QString() << QString();
}
void RotationMappingTest::direction()
{
    QFETCH(QString, target);
    QFETCH(QString, current);
    QFETCH(QString, expected);

    QCOMPARE(RotationMapping::direction(target, current), expected);
}

QTEST_GUILESS_MAIN(RotationMappingTest)
#include "rotationmappingtest.moc"

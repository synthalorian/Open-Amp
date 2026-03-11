#pragma once

#include <QColor>
#include <QString>

namespace openamp {

struct Theme {
    // Background colors
    QColor background = QColor(30, 30, 35);
    QColor surface = QColor(40, 40, 48);
    QColor surfaceVariant = QColor(50, 50, 60);

    // Text colors
    QColor textPrimary = QColor(240, 240, 245);
    QColor textSecondary = QColor(160, 160, 170);
    QColor textMuted = QColor(100, 100, 110);

    // Accent colors
    QColor accent = QColor(255, 140, 0);  // Orange
    QColor accentLight = QColor(255, 170, 50);
    QColor accentDark = QColor(200, 100, 0);

    // Status colors
    QColor enabled = QColor(0, 200, 100);
    QColor disabled = QColor(80, 80, 90);
    QColor warning = QColor(255, 180, 0);
    QColor error = QColor(255, 80, 80);

    // Knob colors
    QColor knobRing = QColor(60, 60, 70);
    QColor knobValue = QColor(255, 140, 0);
    QColor knobCenter = QColor(50, 50, 58);

    // Fonts
    QString fontFamily = "Inter";
    int fontSizeSmall = 11;
    int fontSizeNormal = 13;
    int fontSizeLarge = 16;
    int fontSizeTitle = 20;

    static Theme dark() {
        return Theme();
    }

    static Theme light() {
        Theme t;
        t.background = QColor(245, 245, 250);
        t.surface = QColor(255, 255, 255);
        t.surfaceVariant = QColor(235, 235, 240);
        t.textPrimary = QColor(30, 30, 35);
        t.textSecondary = QColor(80, 80, 90);
        t.textMuted = QColor(140, 140, 150);
        t.knobRing = QColor(200, 200, 210);
        t.knobCenter = QColor(240, 240, 245);
        return t;
    }
};

} // namespace openamp

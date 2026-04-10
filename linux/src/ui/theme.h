#pragma once

#include <QColor>
#include <QString>

namespace openamp {

struct Theme {
    // Background colors
    QColor background = QColor(10, 10, 15);      // Deeper black
    QColor surface = QColor(25, 15, 35);         // Dark purple
    QColor surfaceVariant = QColor(40, 20, 50);  // Lighter purple

    // Text colors
    QColor textPrimary = QColor(255, 255, 255);
    QColor textSecondary = QColor(0, 255, 255);  // Cyan
    QColor textMuted = QColor(255, 0, 255);      // Magenta

    // Accent colors
    QColor accent = QColor(255, 0, 127);         // Neon Pink
    QColor accentLight = QColor(255, 100, 180);
    QColor accentDark = QColor(150, 0, 75);

    // Status colors
    QColor enabled = QColor(0, 255, 100);        // Neon Green
    QColor disabled = QColor(50, 50, 60);
    QColor warning = QColor(255, 255, 0);        // Yellow
    QColor error = QColor(255, 0, 0);            // Red

    // Knob colors
    QColor knobRing = QColor(0, 255, 255);       // Cyan Ring
    QColor knobValue = QColor(255, 0, 255);      // Magenta Value
    QColor knobCenter = QColor(20, 10, 30);      // Dark Center

    // Fonts
    QString fontFamily = "Inter";
    int fontSizeSmall = 10;
    int fontSizeNormal = 12;
    int fontSizeLarge = 14;
    int fontSizeTitle = 18;

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

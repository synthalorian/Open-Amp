#pragma once

#include <QColor>
#include <QString>

namespace openamp {

struct Theme {
    // Background colors (Blackshield)
    QColor background = QColor(0x10, 0x10, 0x14);      // #101014 iron
    QColor surface = QColor(0x16, 0x16, 0x1C);         // #16161C steel
    QColor surfaceVariant = QColor(0x1A, 0x1A, 0x20);  // #1A1A20 steel-light

    // Text colors
    QColor textPrimary = QColor(0xD8, 0xD3, 0xC8);     // #D8D3C8 bone
    QColor textSecondary = QColor(0x7B, 0x9D, 0xC4);   // #7B9DC4 steel-blue bright
    QColor textMuted = QColor(0x8A, 0x8F, 0x98);       // #8A8F98 ash

    // Accent colors
    QColor accent = QColor(0xC1, 0x12, 0x1F);          // #C1121F blood
    QColor accentLight = QColor(0xFF, 0x6B, 0x72);     // #FF6B72 blood-bright
    QColor accentDark = QColor(0x6E, 0x0A, 0x11);      // dark blood

    // Status colors
    QColor enabled = QColor(0x6A, 0x99, 0x4E);         // #6A994E field-green
    QColor disabled = QColor(0x50, 0x50, 0x58);
    QColor warning = QColor(0xC9, 0xA2, 0x27);         // #C9A227 war-gold
    QColor error = QColor(0xC1, 0x12, 0x1F);           // #C1121F blood

    // Knob colors
    QColor knobRing = QColor(0x8A, 0x8F, 0x98);        // #8A8F98 ash ring
    QColor knobValue = QColor(0xC1, 0x12, 0x1F);       // #C1121F blood value
    QColor knobCenter = QColor(0x0D, 0x0D, 0x11);      // #0D0D11 void

    // Fonts
    QString fontFamily = "Inter";
    int fontSizeSmall = 10;
    int fontSizeNormal = 12;
    int fontSizeLarge = 14;
    int fontSizeTitle = 18;

    // Default theme: Blackshield
    static Theme dark() {
        return Theme();
    }

    // Legacy neon synthwave palette (previous default), kept selectable
    static Theme neon() {
        Theme t;
        t.background = QColor(10, 10, 15);
        t.surface = QColor(25, 15, 35);
        t.surfaceVariant = QColor(40, 20, 50);
        t.textPrimary = QColor(255, 255, 255);
        t.textSecondary = QColor(0, 255, 255);
        t.textMuted = QColor(255, 0, 255);
        t.accent = QColor(255, 0, 127);
        t.accentLight = QColor(255, 100, 180);
        t.accentDark = QColor(150, 0, 75);
        t.enabled = QColor(0, 255, 100);
        t.disabled = QColor(50, 50, 60);
        t.warning = QColor(255, 255, 0);
        t.error = QColor(255, 0, 0);
        t.knobRing = QColor(0, 255, 255);
        t.knobValue = QColor(255, 0, 255);
        t.knobCenter = QColor(20, 10, 30);
        return t;
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

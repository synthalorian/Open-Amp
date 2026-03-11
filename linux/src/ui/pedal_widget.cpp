#include "pedal_widget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

namespace openamp {

PedalWidget::PedalWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(80, 100);
    setCursor(Qt::PointingHandCursor);
}

QSize PedalWidget::sizeHint() const {
    return QSize(80, 100);
}

QSize PedalWidget::minimumSizeHint() const {
    return QSize(70, 90);
}

void PedalWidget::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        emit toggled(enabled);
        update();
    }
}

void PedalWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int pedalW = w - 10;
    int pedalH = h - 25;
    int pedalX = 5;
    int pedalY = 5;

    // Pedal body
    QColor bodyColor = enabled_ ? color_ : theme_.surfaceVariant;
    if (hovered_ && !enabled_) {
        bodyColor = theme_.surfaceVariant.lighter(120);
    }

    painter.setBrush(bodyColor);
    painter.setPen(QPen(theme_.textMuted, 2));

    // Rounded rectangle for pedal
    QPainterPath path;
    path.addRoundedRect(pedalX, pedalY, pedalW, pedalH, 8, 8);
    painter.drawPath(path);

    // LED indicator
    int ledSize = 8;
    int ledX = pedalX + pedalW - 15;
    int ledY = pedalY + 10;
    painter.setBrush(enabled_ ? theme_.enabled : theme_.disabled);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(ledX, ledY, ledSize, ledSize);

    // LED glow when enabled
    if (enabled_) {
        QRadialGradient gradient(ledX + ledSize/2, ledY + ledSize/2, ledSize);
        gradient.setColorAt(0, QColor(theme_.enabled.red(),
                                       theme_.enabled.green(),
                                       theme_.enabled.blue(), 150));
        gradient.setColorAt(1, QColor(theme_.enabled.red(),
                                       theme_.enabled.green(),
                                       theme_.enabled.blue(), 0));
        painter.setBrush(gradient);
        painter.drawEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8);
    }

    // Pedal name
    painter.setFont(QFont(theme_.fontFamily, theme_.fontSizeNormal, QFont::Bold));
    painter.setPen(enabled_ ? theme_.textPrimary : theme_.textSecondary);
    QFontMetrics fm(painter.font());
    QRect textRect = fm.boundingRect(name_);
    painter.drawText(pedalX + (pedalW - textRect.width()) / 2,
                     pedalY + pedalH / 2 + fm.ascent() / 2,
                     name_);

    // Label
    painter.setFont(QFont(theme_.fontFamily, theme_.fontSizeSmall));
    painter.setPen(theme_.textMuted);
    QRect labelRect = fm.boundingRect(name_);
    painter.drawText((w - labelRect.width()) / 2, h - 8, name_);
}

void PedalWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setEnabled(!enabled_);
        event->accept();
    }
}

void PedalWidget::enterEvent(QEnterEvent*) {
    hovered_ = true;
    update();
}

void PedalWidget::leaveEvent(QEvent*) {
    hovered_ = false;
    update();
}

} // namespace openamp

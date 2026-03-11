#include "knob_widget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

namespace openamp {

KnobWidget::KnobWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(70, 90);
    setCursor(Qt::SizeVerCursor);
}

QSize KnobWidget::sizeHint() const {
    return QSize(70, 90);
}

QSize KnobWidget::minimumSizeHint() const {
    return QSize(60, 80);
}

float KnobWidget::normalizedValue() const {
    if (maxValue_ == minValue_) return 0.0f;
    return (value_ - minValue_) / (maxValue_ - minValue_);
}

void KnobWidget::setNormalizedValue(float norm) {
    norm = std::clamp(norm, 0.0f, 1.0f);
    setValue(minValue_ + norm * (maxValue_ - minValue_));
}

void KnobWidget::setValue(float value) {
    value = std::clamp(value, minValue_, maxValue_);
    if (std::abs(value_ - value) > 0.0001f) {
        value_ = value;
        emit valueChanged(value_);
        update();
    }
}

void KnobWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int knobSize = std::min(w - 10, h - 30);
    int knobX = (w - knobSize) / 2;
    int knobY = 5;

    // Draw label
    painter.setFont(QFont(theme_.fontFamily, theme_.fontSizeSmall));
    painter.setPen(theme_.textSecondary);
    QFontMetrics fm(painter.font());
    QRect labelRect = fm.boundingRect(label_);
    painter.drawText((w - labelRect.width()) / 2, knobY + knobSize + 18, label_);

    // Draw value
    QString valueText = QString::number(value_, 'f', 1) + valueSuffix_;
    QRect valueRect = fm.boundingRect(valueText);
    painter.drawText((w - valueRect.width()) / 2, knobY + knobSize + 32, valueText);

    // Knob center
    painter.setBrush(theme_.knobCenter);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(knobX, knobY, knobSize, knobSize);

    // Knob ring background
    painter.setBrush(Qt::NoBrush);
    QPen ringPen(theme_.knobRing, knobSize / 10);
    painter.setPen(ringPen);
    int ringOffset = knobSize / 20;
    painter.drawEllipse(knobX + ringOffset, knobY + ringOffset,
                        knobSize - ringOffset * 2, knobSize - ringOffset * 2);

    // Knob value arc
    float norm = normalizedValue();
    float startAngle = 225 * 16;  // Start at 225 degrees (7:30 position)
    float spanAngle = -270 * 16 * norm;  // Sweep to 315 degrees (4:30 position)

    QPen valuePen(hovered_ ? theme_.accentLight : theme_.knobValue, knobSize / 10);
    painter.setPen(valuePen);
    painter.drawArc(knobX + ringOffset, knobY + ringOffset,
                    knobSize - ringOffset * 2, knobSize - ringOffset * 2,
                    startAngle, spanAngle);

    // Draw pointer
    float angle = 225 - 270 * norm;  // Degrees
    float rad = angle * M_PI / 180.0;
    int pointerLength = knobSize / 3;
    int centerX = knobX + knobSize / 2;
    int centerY = knobY + knobSize / 2;

    painter.setPen(QPen(theme_.textPrimary, 2));
    painter.drawLine(centerX, centerY,
                     centerX + static_cast<int>(pointerLength * std::cos(rad)),
                     centerY - static_cast<int>(pointerLength * std::sin(rad)));

    // Center dot
    painter.setBrush(theme_.textPrimary);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(centerX - 3, centerY - 3, 6, 6);
}

void KnobWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        lastY_ = static_cast<int>(event->position().y());
        event->accept();
    }
}

void KnobWidget::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_) {
        int delta = lastY_ - static_cast<int>(event->position().y());
        float sensitivity = 0.01f;
        if (event->modifiers() & Qt::ShiftModifier) {
            sensitivity = 0.002f;  // Fine control
        }
        setNormalizedValue(normalizedValue() + delta * sensitivity);
        lastY_ = static_cast<int>(event->position().y());
        event->accept();
    }
}

void KnobWidget::mouseReleaseEvent(QMouseEvent*) {
    dragging_ = false;
}

void KnobWidget::wheelEvent(QWheelEvent* event) {
    float delta = event->angleDelta().y() / 1200.0f;
    if (event->modifiers() & Qt::ShiftModifier) {
        delta /= 5.0f;  // Fine control
    }
    setNormalizedValue(normalizedValue() + delta);
    event->accept();
}

void KnobWidget::enterEvent(QEnterEvent*) {
    hovered_ = true;
    update();
}

void KnobWidget::leaveEvent(QEvent*) {
    hovered_ = false;
    update();
}

} // namespace openamp

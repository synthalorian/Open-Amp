#include "meter_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace openamp {

// ============== LevelMeter ==============

LevelMeter::LevelMeter(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(30, 150);
}

QSize LevelMeter::sizeHint() const {
    return QSize(stereo_ ? 50 : 30, 200);
}

QSize LevelMeter::minimumSizeHint() const {
    return QSize(stereo_ ? 40 : 20, 100);
}

float LevelMeter::dbToY(float db, int height) const {
    float normalized = (db - kMinDb) / (kMaxDb - kMinDb);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    return height * (1.0f - normalized);
}

void LevelMeter::setLevel(float left, float right) {
    // Convert linear to dB
    auto toDb = [](float linear) {
        if (linear <= 0.0f) return kMinDb;
        return std::max(kMinDb, 20.0f * std::log10(linear));
    };
    
    leftLevel_ = toDb(left);
    rightLevel_ = toDb(right);
    
    // Update peaks (hold for a bit)
    if (leftLevel_ > leftPeak_) leftPeak_ = leftLevel_;
    if (rightLevel_ > rightPeak_) rightPeak_ = rightLevel_;
    
    update();
}

void LevelMeter::setLevel(float mono) {
    setLevel(mono, mono);
    stereo_ = false;
}

void LevelMeter::setPeak(float leftPeak, float rightPeak) {
    leftPeak_ = leftPeak;
    rightPeak_ = rightPeak;
    update();
}

void LevelMeter::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int meterWidth = stereo_ ? (w - 15) / 2 : w - 10;
    int meterHeight = h - 20;

    // Background
    painter.fillRect(0, 0, w, h, theme_.surface);

    // Draw meters
    auto drawMeter = [&](int x, float level, float peak) {
        // Meter background
        painter.fillRect(x, 10, meterWidth, meterHeight, theme_.surfaceVariant);

        // Calculate level height
        float levelY = dbToY(level, meterHeight);
        float levelHeight = meterHeight - levelY;

        // Draw gradient level
        if (levelHeight > 0) {
            QLinearGradient gradient(x, 10 + levelY, x, 10 + meterHeight);
            gradient.setColorAt(0.0, QColor(0, 200, 0));      // Green at bottom
            gradient.setColorAt(0.6, QColor(200, 200, 0));    // Yellow in middle
            gradient.setColorAt(0.85, QColor(255, 100, 0));   // Orange
            gradient.setColorAt(1.0, QColor(255, 0, 0));      // Red at top
            
            painter.fillRect(x, 10 + levelY, meterWidth, levelHeight, gradient);
        }

        // Peak indicator
        float peakY = dbToY(peak, meterHeight);
        painter.setPen(QPen(QColor(255, 255, 255), 2));
        painter.drawLine(x, 10 + peakY, x + meterWidth, 10 + peakY);

        // Scale markers
        painter.setPen(QPen(theme_.textMuted, 1));
        for (float db : {0.0f, -6.0f, -12.0f, -24.0f, -48.0f}) {
            float y = dbToY(db, meterHeight);
            painter.drawLine(x, 10 + y, x + 3, 10 + y);
        }
    };

    if (stereo_) {
        drawMeter(5, leftLevel_, leftPeak_);
        drawMeter(10 + meterWidth, rightLevel_, rightPeak_);
        
        // Labels
        painter.setFont(QFont(theme_.fontFamily, theme_.fontSizeSmall - 1));
        painter.setPen(theme_.textMuted);
        painter.drawText(5, h - 3, "L");
        painter.drawText(10 + meterWidth, h - 3, "R");
    } else {
        drawMeter(5, leftLevel_, leftPeak_);
    }

    // Decay peaks slowly
    leftPeak_ -= 0.5f;
    rightPeak_ -= 0.5f;
    if (leftPeak_ < kMinDb) leftPeak_ = kMinDb;
    if (rightPeak_ < kMinDb) rightPeak_ = kMinDb;
}

// ============== WaveformDisplay ==============

WaveformDisplay::WaveformDisplay(QWidget* parent)
    : QWidget(parent)
    , buffer_(bufferSize_, 0.0f)
{
    setMinimumSize(200, 80);
}

QSize WaveformDisplay::sizeHint() const {
    return QSize(400, 100);
}

QSize WaveformDisplay::minimumSizeHint() const {
    return QSize(100, 60);
}

void WaveformDisplay::pushSamples(const float* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        buffer_[writePos_] = samples[i];
        writePos_ = (writePos_ + 1) % bufferSize_;
    }
    update();
}

void WaveformDisplay::clear() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_ = 0;
    update();
}

void WaveformDisplay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int centerY = h / 2;

    // Background
    painter.fillRect(0, 0, w, h, theme_.surface);

    // Center line
    painter.setPen(QPen(theme_.textMuted, 1, Qt::DashLine));
    painter.drawLine(0, centerY, w, centerY);

    // Draw waveform
    if (buffer_.empty()) return;

    painter.setPen(QPen(theme_.accent, 1.5f));

    QPainterPath path;
    bool first = true;
    
    for (int x = 0; x < w; ++x) {
        size_t idx = (writePos_ + x * bufferSize_ / w) % bufferSize_;
        float sample = buffer_[idx];
        int y = centerY - static_cast<int>(sample * centerY * 0.9f);
        
        if (first) {
            path.moveTo(x, y);
            first = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    painter.drawPath(path);
}

// ============== TunerDisplay ==============

TunerDisplay::TunerDisplay(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 100);
}

QSize TunerDisplay::sizeHint() const {
    return QSize(250, 120);
}

QSize TunerDisplay::minimumSizeHint() const {
    return QSize(150, 80);
}

void TunerDisplay::setFrequency(float hz) {
    frequency_ = hz;
    update();
}

void TunerDisplay::setNote(const QString& note, int octave, float cents) {
    note_ = note;
    octave_ = octave;
    cents_ = cents;
    update();
}

void TunerDisplay::setActive(bool active) {
    active_ = active;
    update();
}

void TunerDisplay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    painter.fillRect(0, 0, w, h, theme_.surface);

    // Note display
    QFont noteFont(theme_.fontFamily, 36, QFont::Bold);
    painter.setFont(noteFont);
    painter.setPen(active_ ? theme_.accent : theme_.textMuted);
    
    QString noteText = note_;
    if (octave_ > 0) {
        noteText += QString::number(octave_);
    }
    
    QFontMetrics fm(noteFont);
    QRect noteRect = fm.boundingRect(noteText);
    painter.drawText((w - noteRect.width()) / 2, 50, noteText);

    // Frequency display
    QFont freqFont(theme_.fontFamily, theme_.fontSizeNormal);
    painter.setFont(freqFont);
    painter.setPen(theme_.textSecondary);
    
    QString freqText = QString("%1 Hz").arg(frequency_, 0, 'f', 1);
    painter.drawText((w - fm.boundingRect(freqText).width()) / 2, 70, freqText);

    // Cents indicator
    int centerY = h - 25;
    int indicatorWidth = w - 40;
    int indicatorX = 20;

    // Draw cents scale
    painter.setPen(QPen(theme_.textMuted, 1));
    painter.drawLine(indicatorX, centerY, indicatorX + indicatorWidth, centerY);

    // Center mark
    painter.setPen(QPen(theme_.accent, 2));
    int centerX = indicatorX + indicatorWidth / 2;
    painter.drawLine(centerX, centerY - 8, centerX, centerY + 8);

    // Cents position (-50 to +50)
    float normalizedCents = cents_ / 50.0f;  // -1 to +1
    normalizedCents = std::clamp(normalizedCents, -1.0f, 1.0f);
    int centsX = centerX + static_cast<int>(normalizedCents * (indicatorWidth / 2));

    // Draw cents indicator
    QColor centsColor = std::abs(cents_) < 5 ? theme_.enabled :
                         std::abs(cents_) < 15 ? theme_.warning : theme_.error;
    painter.setBrush(centsColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(centsX - 5, centerY - 5, 10, 10);

    // Scale marks
    painter.setPen(QPen(theme_.textMuted, 1));
    for (int i = -2; i <= 2; ++i) {
        int x = centerX + i * (indicatorWidth / 4);
        painter.drawLine(x, centerY - 4, x, centerY + 4);
    }
}

} // namespace openamp

#pragma once

#include <QWidget>
#include <QTimer>
#include <vector>
#include "theme.h"

namespace openamp {

class LevelMeter : public QWidget {
    Q_OBJECT

public:
    explicit LevelMeter(QWidget* parent = nullptr);
    
    void setLevel(float left, float right);
    void setLevel(float mono);
    void setPeak(float leftPeak, float rightPeak);
    void setStereo(bool stereo) { stereo_ = stereo; update(); }
    void setTheme(const Theme& theme) { theme_ = theme; update(); }
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float leftLevel_ = -60.0f;   // dB
    float rightLevel_ = -60.0f;
    float leftPeak_ = -60.0f;
    float rightPeak_ = -60.0f;
    bool stereo_ = true;
    Theme theme_;
    
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb = 6.0f;
    
    float dbToY(float db, int height) const;
};

class WaveformDisplay : public QWidget {
    Q_OBJECT

public:
    explicit WaveformDisplay(QWidget* parent = nullptr);
    
    void pushSamples(const float* samples, size_t count);
    void clear();
    void setTheme(const Theme& theme) { theme_ = theme; update(); }
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<float> buffer_;
    size_t writePos_ = 0;
    size_t bufferSize_ = 2048;
    Theme theme_;
};

class TunerDisplay : public QWidget {
    Q_OBJECT

public:
    explicit TunerDisplay(QWidget* parent = nullptr);
    
    void setFrequency(float hz);
    void setNote(const QString& note, int octave, float cents);
    void setActive(bool active);
    void setTheme(const Theme& theme) { theme_ = theme; update(); }
    
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float frequency_ = 0.0f;
    QString note_ = "--";
    int octave_ = 0;
    float cents_ = 0.0f;  // -50 to +50
    bool active_ = false;
    Theme theme_;
};

} // namespace openamp

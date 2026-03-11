#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <memory>
#include "theme.h"

namespace openamp {

class DSPEngine;

class LatencyDisplayWidget : public QWidget {
    Q_OBJECT

public:
    explicit LatencyDisplayWidget(DSPEngine* engine, QWidget* parent = nullptr);

    void setTheme(const Theme& theme);
    void startMonitoring(int updateIntervalMs = 100);  // Default 10Hz
    void stopMonitoring();

private:
    void setupUI();
    void updateDisplay();

    DSPEngine* engine_;
    Theme theme_;

    QLabel* latencyValueLabel_ = nullptr;
    QLabel* bufferSizeLabel_ = nullptr;
    QLabel* sampleRateLabel_ = nullptr;
    QLabel* theoreticalLabel_ = nullptr;

    QTimer* updateTimer_ = nullptr;
};

} // namespace openamp

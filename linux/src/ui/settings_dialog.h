#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include "theme.h"
#include "audio/audio_backend.h"

namespace openamp {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(AudioBackend* backend, QWidget* parent = nullptr);

    AudioConfig getAudioConfig() const;
    void setAudioConfig(const AudioConfig& config);

signals:
    void settingsChanged(const AudioConfig& config);

private:
    void setupUI();
    void populateDevices();
    void applyTheme();
    void calculateLatency();

    AudioBackend* backend_;
    Theme theme_;

    QComboBox* inputDeviceCombo_ = nullptr;
    QComboBox* outputDeviceCombo_ = nullptr;
    QSpinBox* sampleRateSpin_ = nullptr;
    QSpinBox* bufferSizeSpin_ = nullptr;
    QLabel* latencyLabel_ = nullptr;
    QLabel* backendLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

} // namespace openamp

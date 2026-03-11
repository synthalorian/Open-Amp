#pragma once

#include <QMainWindow>
#include <QLabel>
#include <memory>
#include "theme.h"
#include "openamp/dsp_engine.h"
#include "audio/audio_backend.h"
#include "audio/midi_handler.h"

namespace openamp {

class AmpPanel;
class EffectsPanel;
class PresetBrowser;
class LevelMeter;
class TunerDisplay;
class IRPanelWidget;
class LatencyDisplayWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void connectSignals();

    void startAudio();
    void stopAudio();
    void toggleAudio();

    void showSettings();
    void showAbout();

    void loadPreset(const QString& path);
    void savePreset(const QString& path);

    void updateLatencyDisplay();
    void updatePresetDisplay(const QString& name);
    void updateMeters();
    void toggleTuner(bool enabled);

    // Core components
    std::unique_ptr<DSPEngine> engine_;
    std::unique_ptr<AudioBackend> audio_;
    std::unique_ptr<MidiHandler> midi_;

    // UI components
    Theme theme_;

    QWidget* centralWidget_ = nullptr;
    AmpPanel* ampPanel_ = nullptr;
    EffectsPanel* effectsPanel_ = nullptr;
    PresetBrowser* presetBrowser_ = nullptr;
    IRPanelWidget* irPanel_ = nullptr;
    LatencyDisplayWidget* latencyDisplay_ = nullptr;
    
    // Meters and visualizations
    LevelMeter* inputMeter_ = nullptr;
    LevelMeter* outputMeter_ = nullptr;
    TunerDisplay* tunerDisplay_ = nullptr;
    QTimer* meterTimer_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QLabel* latencyLabel_ = nullptr;
    QLabel* presetLabel_ = nullptr;

    QAction* audioToggleAction_ = nullptr;
    QAction* tunerToggleAction_ = nullptr;
    bool audioRunning_ = false;
    bool tunerEnabled_ = false;
};

} // namespace openamp

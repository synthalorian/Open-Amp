#pragma once

#include <QWidget>
#include <memory>
#include "theme.h"
#include "openamp/dsp_engine.h"

namespace openamp {

class KnobWidget;
class PedalWidget;

class EffectsPanel : public QWidget {
    Q_OBJECT

public:
    explicit EffectsPanel(DSPEngine* engine, QWidget* parent = nullptr);

    void setTheme(const Theme& theme);
    void syncFromEngine();

signals:
    void settingsChanged();

private:
    void setupUI();
    void connectSignals();

    DSPEngine* engine_;
    Theme theme_;

    // Distortion
    PedalWidget* distortionPedal_ = nullptr;
    KnobWidget* distortionDrive_ = nullptr;
    KnobWidget* distortionTone_ = nullptr;
    KnobWidget* distortionLevel_ = nullptr;

    // Delay
    PedalWidget* delayPedal_ = nullptr;
    KnobWidget* delayTime_ = nullptr;
    KnobWidget* delayFeedback_ = nullptr;
    KnobWidget* delayMix_ = nullptr;

    // Reverb
    PedalWidget* reverbPedal_ = nullptr;
    KnobWidget* reverbRoom_ = nullptr;
    KnobWidget* reverbDamp_ = nullptr;
    KnobWidget* reverbMix_ = nullptr;
};

} // namespace openamp

#pragma once

#include <QWidget>
#include <memory>
#include "theme.h"
#include "openamp/dsp_engine.h"

namespace openamp {

class KnobWidget;

class AmpPanel : public QWidget {
    Q_OBJECT

public:
    explicit AmpPanel(DSPEngine* engine, QWidget* parent = nullptr);

    void setTheme(const Theme& theme);
    void syncFromEngine();

signals:
    void settingsChanged();

private:
    void setupUI();
    void connectSignals();

    DSPEngine* engine_;
    Theme theme_;

    KnobWidget* gainKnob_ = nullptr;
    KnobWidget* driveKnob_ = nullptr;
    KnobWidget* bassKnob_ = nullptr;
    KnobWidget* midKnob_ = nullptr;
    KnobWidget* trebleKnob_ = nullptr;
    KnobWidget* presenceKnob_ = nullptr;
    KnobWidget* masterKnob_ = nullptr;
};

} // namespace openamp

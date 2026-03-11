#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <memory>
#include "theme.h"

namespace openamp {

class KnobWidget;
class DSPEngine;

class IRPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit IRPanelWidget(DSPEngine* engine, QWidget* parent = nullptr);

    void setTheme(const Theme& theme);
    void syncFromEngine();

signals:
    void settingsChanged();
    void irLoaded(const QString& irName);

private:
    void setupUI();
    void connectSignals();
    void browseIR();
    void updateIRName();

    DSPEngine* engine_;
    Theme theme_;

    // IR file display
    QLabel* irNameLabel_ = nullptr;
    QPushButton* browseButton_ = nullptr;

    // Enable toggle
    QCheckBox* enableCheckBox_ = nullptr;

    // Controls
    KnobWidget* mixKnob_ = nullptr;
    KnobWidget* highCutKnob_ = nullptr;
    KnobWidget* lowCutKnob_ = nullptr;
};

} // namespace openamp

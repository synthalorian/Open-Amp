#include "ir_panel_widget.h"
#include "knob_widget.h"
#include "openamp/dsp_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QFrame>
#include <QPainter>
#include <QFileInfo>

namespace openamp {

IRPanelWidget::IRPanelWidget(DSPEngine* engine, QWidget* parent)
    : QWidget(parent)
    , engine_(engine)
    , theme_(Theme::dark())
{
    setupUI();
    connectSignals();
}

void IRPanelWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title
    auto* titleLabel = new QLabel("IR Loader");
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
    ).arg(theme_.textPrimary.name()));
    mainLayout->addWidget(titleLabel);

    // Separator line
    auto* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QString(
        "QFrame {"
        "  background-color: %1;"
        "  border: none;"
        "  max-height: 1px;"
        "}"
    ).arg(theme_.surfaceVariant.name()));
    mainLayout->addWidget(separator);

    // IR file selection row
    auto* fileLayout = new QHBoxLayout;
    fileLayout->setSpacing(8);

    irNameLabel_ = new QLabel("No IR loaded");
    irNameLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  background-color: %2;"
        "  padding: 6px 10px;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "}"
    ).arg(theme_.textSecondary.name(),
          theme_.surface.name()));
    irNameLabel_->setMinimumWidth(150);
    fileLayout->addWidget(irNameLabel_, 1);

    browseButton_ = new QPushButton("Browse...");
    browseButton_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %3;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %4;"
        "}"
    ).arg(theme_.accent.name(),
          theme_.textPrimary.name(),
          theme_.accentLight.name(),
          theme_.accentDark.name()));
    fileLayout->addWidget(browseButton_);

    mainLayout->addLayout(fileLayout);

    // Enable checkbox
    enableCheckBox_ = new QCheckBox("Enable IR");
    enableCheckBox_->setStyleSheet(QString(
        "QCheckBox {"
        "  color: %1;"
        "  font-size: 13px;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 3px;"
        "  border: 2px solid %2;"
        "  background-color: %3;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %4;"
        "  border-color: %4;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: %5;"
        "}"
    ).arg(theme_.textPrimary.name(),
          theme_.surfaceVariant.name(),
          theme_.surface.name(),
          theme_.enabled.name(),
          theme_.accent.name()));
    mainLayout->addWidget(enableCheckBox_);

    // Separator
    auto* separator2 = new QFrame;
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet(QString(
        "QFrame {"
        "  background-color: %1;"
        "  border: none;"
        "  max-height: 1px;"
        "}"
    ).arg(theme_.surfaceVariant.name()));
    mainLayout->addWidget(separator2);

    // Knobs row
    auto* knobsLayout = new QHBoxLayout;
    knobsLayout->setSpacing(15);

    // Mix knob (0-100%)
    mixKnob_ = new KnobWidget;
    mixKnob_->setLabel("Mix");
    mixKnob_->setMinValue(0.0f);
    mixKnob_->setMaxValue(100.0f);
    mixKnob_->setValue(100.0f);
    mixKnob_->setValueSuffix("%");
    mixKnob_->setTheme(theme_);
    knobsLayout->addWidget(mixKnob_);

    // High-cut knob (500-16000 Hz)
    highCutKnob_ = new KnobWidget;
    highCutKnob_->setLabel("High Cut");
    highCutKnob_->setMinValue(500.0f);
    highCutKnob_->setMaxValue(16000.0f);
    highCutKnob_->setValue(16000.0f);
    highCutKnob_->setValueSuffix(" Hz");
    highCutKnob_->setTheme(theme_);
    knobsLayout->addWidget(highCutKnob_);

    // Low-cut knob (80-500 Hz)
    lowCutKnob_ = new KnobWidget;
    lowCutKnob_->setLabel("Low Cut");
    lowCutKnob_->setMinValue(80.0f);
    lowCutKnob_->setMaxValue(500.0f);
    lowCutKnob_->setValue(80.0f);
    lowCutKnob_->setValueSuffix(" Hz");
    lowCutKnob_->setTheme(theme_);
    knobsLayout->addWidget(lowCutKnob_);

    mainLayout->addLayout(knobsLayout);

    // Spacer to push everything up
    mainLayout->addStretch();

    // Set panel background
    setStyleSheet(QString(
        "IRPanelWidget {"
        "  background-color: %1;"
        "  border-radius: 8px;"
        "}"
    ).arg(theme_.surface.name()));
}

void IRPanelWidget::connectSignals() {
    connect(browseButton_, &QPushButton::clicked, this, &IRPanelWidget::browseIR);
    
    connect(enableCheckBox_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (engine_) {
            engine_->setIREnabled(enabled);
            emit settingsChanged();
        }
    });

    connect(mixKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) {
            engine_->setIRMix(value / 100.0f);  // Convert to 0-1 range
            emit settingsChanged();
        }
    });

    connect(highCutKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) {
            engine_->setIRHighCut(value);
            emit settingsChanged();
        }
    });

    connect(lowCutKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) {
            engine_->setIRLowCut(value);
            emit settingsChanged();
        }
    });
}

void IRPanelWidget::browseIR() {
    QString path = QFileDialog::getOpenFileName(this, "Load Impulse Response",
                                                 QString(),
                                                 "Audio Files (*.wav *.aiff *.flac);;All Files (*)");
    if (!path.isEmpty() && engine_) {
        std::string error;
        if (engine_->loadIR(path.toStdString(), error)) {
            updateIRName();
            emit irLoaded(QFileInfo(path).fileName());
            emit settingsChanged();
        } else {
            // Show error in label
            irNameLabel_->setText(QString("Error: %1").arg(QString::fromStdString(error)));
            irNameLabel_->setStyleSheet(QString(
                "QLabel {"
                "  color: %1;"
                "  background-color: %2;"
                "  padding: 6px 10px;"
                "  border-radius: 4px;"
                "  font-size: 12px;"
                "}"
            ).arg(theme_.error.name(),
                  theme_.surface.name()));
        }
    }
}

void IRPanelWidget::updateIRName() {
    if (engine_) {
        std::string name = engine_->getIRName();
        if (!name.empty()) {
            irNameLabel_->setText(QString::fromStdString(name));
            irNameLabel_->setStyleSheet(QString(
                "QLabel {"
                "  color: %1;"
                "  background-color: %2;"
                "  padding: 6px 10px;"
                "  border-radius: 4px;"
                "  font-size: 12px;"
                "}"
            ).arg(theme_.textPrimary.name(),
                  theme_.surface.name()));
        } else {
            irNameLabel_->setText("No IR loaded");
            irNameLabel_->setStyleSheet(QString(
                "QLabel {"
                "  color: %1;"
                "  background-color: %2;"
                "  padding: 6px 10px;"
                "  border-radius: 4px;"
                "  font-size: 12px;"
                "}"
            ).arg(theme_.textSecondary.name(),
                  theme_.surface.name()));
        }
    }
}

void IRPanelWidget::setTheme(const Theme& theme) {
    theme_ = theme;
    // Rebuild UI with new theme
    // For simplicity, we just update the stylesheet
    setStyleSheet(QString(
        "IRPanelWidget {"
        "  background-color: %1;"
        "  border-radius: 8px;"
        "}"
    ).arg(theme_.surface.name()));
    
    if (mixKnob_) mixKnob_->setTheme(theme);
    if (highCutKnob_) highCutKnob_->setTheme(theme);
    if (lowCutKnob_) lowCutKnob_->setTheme(theme);
    
    update();
}

void IRPanelWidget::syncFromEngine() {
    if (!engine_) return;
    
    // Sync enable state
    // Note: DSPEngine doesn't have getIREnabled(), so we'd need to track it locally
    // For now, we just sync the IR name
    updateIRName();
    
    // Note: DSPEngine doesn't have getters for IR parameters
    // In a full implementation, we'd add those and sync here
}

} // namespace openamp

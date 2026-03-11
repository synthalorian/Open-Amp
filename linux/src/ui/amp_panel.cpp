#include "amp_panel.h"
#include "knob_widget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace openamp {

AmpPanel::AmpPanel(DSPEngine* engine, QWidget* parent)
    : QWidget(parent)
    , engine_(engine)
{
    setupUI();
    connectSignals();
}

void AmpPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title
    auto* titleLabel = new QLabel("Amplifier");
    titleLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeTitle, QFont::Bold));
    titleLabel->setStyleSheet(QString("color: %1;").arg(theme_.textPrimary.name()));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Separator
    auto* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QString("background-color: %1; max-height: 1px;")
                             .arg(theme_.surfaceVariant.name()));
    mainLayout->addWidget(separator);

    // Knobs layout
    auto* knobsLayout = new QHBoxLayout;
    knobsLayout->setSpacing(15);

    // Gain knob
    gainKnob_ = new KnobWidget;
    gainKnob_->setLabel("Gain");
    gainKnob_->setValueSuffix(" dB");
    gainKnob_->setMinValue(-20);
    gainKnob_->setMaxValue(20);
    gainKnob_->setTheme(theme_);
    knobsLayout->addWidget(gainKnob_);

    // Drive knob
    driveKnob_ = new KnobWidget;
    driveKnob_->setLabel("Drive");
    driveKnob_->setValueSuffix("%");
    driveKnob_->setMinValue(0);
    driveKnob_->setMaxValue(100);
    driveKnob_->setTheme(theme_);
    knobsLayout->addWidget(driveKnob_);

    knobsLayout->addStretch();

    mainLayout->addLayout(knobsLayout);

    // EQ section
    auto* eqLabel = new QLabel("EQ");
    eqLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    eqLabel->setStyleSheet(QString("color: %1;").arg(theme_.textSecondary.name()));
    eqLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(eqLabel);

    // EQ knobs
    auto* eqLayout = new QHBoxLayout;
    eqLayout->setSpacing(15);

    bassKnob_ = new KnobWidget;
    bassKnob_->setLabel("Bass");
    bassKnob_->setValueSuffix(" dB");
    bassKnob_->setMinValue(-12);
    bassKnob_->setMaxValue(12);
    bassKnob_->setTheme(theme_);
    eqLayout->addWidget(bassKnob_);

    midKnob_ = new KnobWidget;
    midKnob_->setLabel("Mid");
    midKnob_->setValueSuffix(" dB");
    midKnob_->setMinValue(-12);
    midKnob_->setMaxValue(12);
    midKnob_->setTheme(theme_);
    eqLayout->addWidget(midKnob_);

    trebleKnob_ = new KnobWidget;
    trebleKnob_->setLabel("Treble");
    trebleKnob_->setValueSuffix(" dB");
    trebleKnob_->setMinValue(-12);
    trebleKnob_->setMaxValue(12);
    trebleKnob_->setTheme(theme_);
    eqLayout->addWidget(trebleKnob_);

    presenceKnob_ = new KnobWidget;
    presenceKnob_->setLabel("Presence");
    presenceKnob_->setValueSuffix(" dB");
    presenceKnob_->setMinValue(-12);
    presenceKnob_->setMaxValue(12);
    presenceKnob_->setTheme(theme_);
    eqLayout->addWidget(presenceKnob_);

    eqLayout->addStretch();

    mainLayout->addLayout(eqLayout);

    // Master knob
    auto* masterLayout = new QHBoxLayout;
    masterLayout->addStretch();

    masterKnob_ = new KnobWidget;
    masterKnob_->setLabel("Master");
    masterKnob_->setValueSuffix(" dB");
    masterKnob_->setMinValue(-40);
    masterKnob_->setMaxValue(10);
    masterKnob_->setTheme(theme_);
    masterLayout->addWidget(masterKnob_);

    masterLayout->addStretch();
    mainLayout->addLayout(masterLayout);

    mainLayout->addStretch();

    setStyleSheet(QString("background-color: %1; border-radius: 10px;")
                  .arg(theme_.surface.name()));
}

void AmpPanel::connectSignals() {
    connect(gainKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setGain(value);
        emit settingsChanged();
    });

    connect(driveKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDrive(value / 100.0f);
        emit settingsChanged();
    });

    connect(bassKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setBass(value);
        emit settingsChanged();
    });

    connect(midKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setMid(value);
        emit settingsChanged();
    });

    connect(trebleKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setTreble(value);
        emit settingsChanged();
    });

    connect(presenceKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setPresence(value);
        emit settingsChanged();
    });

    connect(masterKnob_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setMaster(value);
        emit settingsChanged();
    });
}

void AmpPanel::setTheme(const Theme& theme) {
    theme_ = theme;
    gainKnob_->setTheme(theme);
    driveKnob_->setTheme(theme);
    bassKnob_->setTheme(theme);
    midKnob_->setTheme(theme);
    trebleKnob_->setTheme(theme);
    presenceKnob_->setTheme(theme);
    masterKnob_->setTheme(theme);
    setStyleSheet(QString("background-color: %1; border-radius: 10px;")
                  .arg(theme_.surface.name()));
}

void AmpPanel::syncFromEngine() {
    if (!engine_) return;

    const auto& preset = engine_->getCurrentPreset();
    gainKnob_->setValue(preset.ampGainDb);
    driveKnob_->setValue(preset.ampDrive * 100.0f);
    bassKnob_->setValue(preset.ampBassDb);
    midKnob_->setValue(preset.ampMidDb);
    trebleKnob_->setValue(preset.ampTrebleDb);
    presenceKnob_->setValue(preset.ampPresenceDb);
    masterKnob_->setValue(preset.ampMasterDb);
}

} // namespace openamp

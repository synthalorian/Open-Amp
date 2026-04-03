#include "effects_panel.h"
#include "knob_widget.h"
#include "pedal_widget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

namespace openamp {

EffectsPanel::EffectsPanel(DSPEngine* engine, QWidget* parent)
    : QWidget(parent)
    , engine_(engine)
    , looper_(std::make_unique<Looper>())
    , metronome_(std::make_unique<Metronome>())
{
    // Initialize looper and metronome with engine's sample rate
    if (engine_) {
        looper_->prepare(engine_->getSampleRate(), engine_->getBlockSize());
        metronome_->prepare(engine_->getSampleRate(), engine_->getBlockSize());
    }
    setupUI();
    connectSignals();
}

void EffectsPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Title
    auto* titleLabel = new QLabel("Effects");
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

    // Distortion section
    auto* distortionLayout = new QVBoxLayout;

    auto* distortionLabel = new QLabel("Distortion");
    distortionLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    distortionLabel->setStyleSheet(QString("color: %1;").arg(theme_.accent.name()));
    distortionLayout->addWidget(distortionLabel, 0, Qt::AlignCenter);

    // Distortion pedal and knobs row
    auto* distortionRow = new QHBoxLayout;

    distortionPedal_ = new PedalWidget;
    distortionPedal_->setName("DIST");
    distortionPedal_->setColor(QColor(200, 50, 50));
    distortionPedal_->setTheme(theme_);
    distortionRow->addWidget(distortionPedal_);

    distortionDrive_ = new KnobWidget;
    distortionDrive_->setLabel("Drive");
    distortionDrive_->setValueSuffix("%");
    distortionDrive_->setMinValue(0);
    distortionDrive_->setMaxValue(100);
    distortionDrive_->setTheme(theme_);
    distortionRow->addWidget(distortionDrive_);

    distortionTone_ = new KnobWidget;
    distortionTone_->setLabel("Tone");
    distortionTone_->setValueSuffix("%");
    distortionTone_->setMinValue(0);
    distortionTone_->setMaxValue(100);
    distortionTone_->setTheme(theme_);
    distortionRow->addWidget(distortionTone_);

    distortionLevel_ = new KnobWidget;
    distortionLevel_->setLabel("Level");
    distortionLevel_->setValueSuffix("%");
    distortionLevel_->setMinValue(0);
    distortionLevel_->setMaxValue(100);
    distortionLevel_->setTheme(theme_);
    distortionRow->addWidget(distortionLevel_);

    distortionRow->addStretch();
    distortionLayout->addLayout(distortionRow);

    mainLayout->addLayout(distortionLayout);

    // Delay section
    auto* delayLayout = new QVBoxLayout;

    auto* delayLabel = new QLabel("Delay");
    delayLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    delayLabel->setStyleSheet(QString("color: %1;").arg(theme_.accent.name()));
    delayLayout->addWidget(delayLabel, 0, Qt::AlignCenter);

    auto* delayRow = new QHBoxLayout;

    delayPedal_ = new PedalWidget;
    delayPedal_->setName("DLY");
    delayPedal_->setColor(QColor(50, 150, 200));
    delayPedal_->setTheme(theme_);
    delayRow->addWidget(delayPedal_);

    delayTime_ = new KnobWidget;
    delayTime_->setLabel("Time");
    delayTime_->setValueSuffix("ms");
    delayTime_->setMinValue(10);
    delayTime_->setMaxValue(1000);
    delayTime_->setTheme(theme_);
    delayRow->addWidget(delayTime_);

    delayFeedback_ = new KnobWidget;
    delayFeedback_->setLabel("Fdbk");
    delayFeedback_->setValueSuffix("%");
    delayFeedback_->setMinValue(0);
    delayFeedback_->setMaxValue(90);
    delayFeedback_->setTheme(theme_);
    delayRow->addWidget(delayFeedback_);

    delayMix_ = new KnobWidget;
    delayMix_->setLabel("Mix");
    delayMix_->setValueSuffix("%");
    delayMix_->setMinValue(0);
    delayMix_->setMaxValue(100);
    delayMix_->setTheme(theme_);
    delayRow->addWidget(delayMix_);

    delayRow->addStretch();
    delayLayout->addLayout(delayRow);

    mainLayout->addLayout(delayLayout);

    // Reverb section
    auto* reverbLayout = new QVBoxLayout;

    auto* reverbLabel = new QLabel("Reverb");
    reverbLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    reverbLabel->setStyleSheet(QString("color: %1;").arg(theme_.accent.name()));
    reverbLayout->addWidget(reverbLabel, 0, Qt::AlignCenter);

    auto* reverbRow = new QHBoxLayout;

    reverbPedal_ = new PedalWidget;
    reverbPedal_->setName("REV");
    reverbPedal_->setColor(QColor(100, 180, 100));
    reverbPedal_->setTheme(theme_);
    reverbRow->addWidget(reverbPedal_);

    reverbRoom_ = new KnobWidget;
    reverbRoom_->setLabel("Room");
    reverbRoom_->setValueSuffix("%");
    reverbRoom_->setMinValue(0);
    reverbRoom_->setMaxValue(100);
    reverbRoom_->setTheme(theme_);
    reverbRow->addWidget(reverbRoom_);

    reverbDamp_ = new KnobWidget;
    reverbDamp_->setLabel("Damp");
    reverbDamp_->setValueSuffix("%");
    reverbDamp_->setMinValue(0);
    reverbDamp_->setMaxValue(100);
    reverbDamp_->setTheme(theme_);
    reverbRow->addWidget(reverbDamp_);

    reverbMix_ = new KnobWidget;
    reverbMix_->setLabel("Mix");
    reverbMix_->setValueSuffix("%");
    reverbMix_->setMinValue(0);
    reverbMix_->setMaxValue(100);
    reverbMix_->setTheme(theme_);
    reverbRow->addWidget(reverbMix_);

    reverbRow->addStretch();
    reverbLayout->addLayout(reverbRow);

    mainLayout->addLayout(reverbLayout);

    // Noise Gate section
    auto* gateLayout = new QVBoxLayout;
    auto* gateLabel = new QLabel("Noise Gate");
    gateLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    gateLabel->setStyleSheet(QString("color: %1;").arg(theme_.accent.name()));
    gateLayout->addWidget(gateLabel, 0, Qt::AlignCenter);

    auto* gateRow = new QHBoxLayout;
    noiseGatePedal_ = new PedalWidget;
    noiseGatePedal_->setName("GATE");
    noiseGatePedal_->setColor(QColor(150, 150, 150));
    noiseGatePedal_->setTheme(theme_);
    gateRow->addWidget(noiseGatePedal_);

    noiseGateThreshold_ = new KnobWidget;
    noiseGateThreshold_->setLabel("Thresh");
    noiseGateThreshold_->setValueSuffix("dB");
    noiseGateThreshold_->setMinValue(-80);
    noiseGateThreshold_->setMaxValue(0);
    noiseGateThreshold_->setTheme(theme_);
    gateRow->addWidget(noiseGateThreshold_);

    noiseGateAttack_ = new KnobWidget;
    noiseGateAttack_->setLabel("Attack");
    noiseGateAttack_->setValueSuffix("ms");
    noiseGateAttack_->setMinValue(1);
    noiseGateAttack_->setMaxValue(50);
    noiseGateAttack_->setTheme(theme_);
    gateRow->addWidget(noiseGateAttack_);

    noiseGateRelease_ = new KnobWidget;
    noiseGateRelease_->setLabel("Rel");
    noiseGateRelease_->setValueSuffix("ms");
    noiseGateRelease_->setMinValue(10);
    noiseGateRelease_->setMaxValue(1000);
    noiseGateRelease_->setTheme(theme_);
    gateRow->addWidget(noiseGateRelease_);

    gateRow->addStretch();
    gateLayout->addLayout(gateRow);
    mainLayout->addLayout(gateLayout);

    // Tools Section (Looper & Metronome)
    auto* toolsLayout = new QVBoxLayout;
    auto* toolsLabel = new QLabel("Looper & Metronome");
    toolsLabel->setFont(QFont(theme_.fontFamily, theme_.fontSizeLarge, QFont::Bold));
    toolsLabel->setStyleSheet(QString("color: %1;").arg(theme_.accent.name()));
    toolsLayout->addWidget(toolsLabel, 0, Qt::AlignCenter);

    auto* toolsRow = new QHBoxLayout;
    
    // Looper
    looperRecPedal_ = new PedalWidget;
    looperRecPedal_->setName("REC");
    looperRecPedal_->setColor(QColor(255, 0, 0));
    looperRecPedal_->setTheme(theme_);
    toolsRow->addWidget(looperRecPedal_);

    looperPlayPedal_ = new PedalWidget;
    looperPlayPedal_->setName("PLAY");
    looperPlayPedal_->setColor(QColor(0, 255, 0));
    looperPlayPedal_->setTheme(theme_);
    toolsRow->addWidget(looperPlayPedal_);

    looperClearPedal_ = new PedalWidget;
    looperClearPedal_->setName("CLR");
    looperClearPedal_->setColor(QColor(100, 100, 100));
    looperClearPedal_->setTheme(theme_);
    toolsRow->addWidget(looperClearPedal_);

    looperMix_ = new KnobWidget;
    looperMix_->setLabel("Loop Mix");
    looperMix_->setValueSuffix("%");
    looperMix_->setMinValue(0);
    looperMix_->setMaxValue(100);
    looperMix_->setTheme(theme_);
    toolsRow->addWidget(looperMix_);

    // Metronome
    metronomePedal_ = new PedalWidget;
    metronomePedal_->setName("METRO");
    metronomePedal_->setColor(QColor(200, 200, 0));
    metronomePedal_->setTheme(theme_);
    toolsRow->addWidget(metronomePedal_);

    metronomeTempo_ = new KnobWidget;
    metronomeTempo_->setLabel("BPM");
    metronomeTempo_->setMinValue(40);
    metronomeTempo_->setMaxValue(240);
    metronomeTempo_->setTheme(theme_);
    toolsRow->addWidget(metronomeTempo_);

    metronomeVol_ = new KnobWidget;
    metronomeVol_->setLabel("Metro Vol");
    metronomeVol_->setValueSuffix("%");
    metronomeVol_->setMinValue(0);
    metronomeVol_->setMaxValue(100);
    metronomeVol_->setTheme(theme_);
    toolsRow->addWidget(metronomeVol_);

    toolsRow->addStretch();
    toolsLayout->addLayout(toolsRow);
    mainLayout->addLayout(toolsLayout);

    mainLayout->addStretch();

    setStyleSheet(QString("background-color: %1; border-radius: 10px;")
                  .arg(theme_.surface.name()));
}

void EffectsPanel::connectSignals() {
    // Distortion
    connect(distortionPedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (engine_) engine_->setDistortionEnabled(enabled);
        emit settingsChanged();
    });

    connect(distortionDrive_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDistortionDrive(value / 100.0f);
        emit settingsChanged();
    });

    connect(distortionTone_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDistortionTone(value / 100.0f);
        emit settingsChanged();
    });

    connect(distortionLevel_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDistortionLevel(value / 100.0f);
        emit settingsChanged();
    });

    // Delay
    connect(delayPedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (engine_) engine_->setDelayEnabled(enabled);
        emit settingsChanged();
    });

    connect(delayTime_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDelayTime(value);
        emit settingsChanged();
    });

    connect(delayFeedback_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDelayFeedback(value / 100.0f);
        emit settingsChanged();
    });

    connect(delayMix_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setDelayMix(value / 100.0f);
        emit settingsChanged();
    });

    // Reverb
    connect(reverbPedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (engine_) engine_->setReverbEnabled(enabled);
        emit settingsChanged();
    });

    connect(reverbRoom_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setReverbRoom(value / 100.0f);
        emit settingsChanged();
    });

    connect(reverbDamp_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setReverbDamp(value / 100.0f);
        emit settingsChanged();
    });

    connect(reverbMix_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setReverbMix(value / 100.0f);
        emit settingsChanged();
    });

    // Noise Gate
    connect(noiseGatePedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (engine_) engine_->setNoiseGateEnabled(enabled);
        emit settingsChanged();
    });
    connect(noiseGateThreshold_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setNoiseGateThreshold(value);
        emit settingsChanged();
    });
    connect(noiseGateAttack_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setNoiseGateAttack(value);
        emit settingsChanged();
    });
    connect(noiseGateRelease_, &KnobWidget::valueChanged, this, [this](float value) {
        if (engine_) engine_->setNoiseGateRelease(value);
        emit settingsChanged();
    });

    // Looper
    connect(looperRecPedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (!looper_) return;
        if (enabled) {
            if (looper_->getState() == Looper::State::Stopped) {
                looper_->record();
            } else {
                looper_->overdub();
            }
        } else {
            if (looper_->getState() == Looper::State::Recording ||
                looper_->getState() == Looper::State::Overdubbing) {
                looper_->play();
            }
        }
    });
    connect(looperPlayPedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (!looper_) return;
        if (enabled) {
            looper_->play();
        } else {
            looper_->pause();
        }
    });
    connect(looperClearPedal_, &PedalWidget::toggled, this, [this](bool /*enabled*/) {
        if (looper_) looper_->clear();
    });
    connect(looperMix_, &KnobWidget::valueChanged, this, [this](float value) {
        if (looper_) looper_->setMix(value / 100.0f);
    });

    // Metronome
    connect(metronomePedal_, &PedalWidget::toggled, this, [this](bool enabled) {
        if (!metronome_) return;
        if (enabled) metronome_->start();
        else metronome_->stop();
    });
    connect(metronomeTempo_, &KnobWidget::valueChanged, this, [this](float value) {
        if (metronome_) metronome_->setTempo(value);
    });
    connect(metronomeVol_, &KnobWidget::valueChanged, this, [this](float value) {
        if (metronome_) metronome_->setClickLevel(value / 100.0f);
    });
}

void EffectsPanel::setTheme(const Theme& theme) {
    theme_ = theme;

    distortionPedal_->setTheme(theme);
    distortionDrive_->setTheme(theme);
    distortionTone_->setTheme(theme);
    distortionLevel_->setTheme(theme);

    delayPedal_->setTheme(theme);
    delayTime_->setTheme(theme);
    delayFeedback_->setTheme(theme);
    delayMix_->setTheme(theme);

    reverbPedal_->setTheme(theme);
    reverbRoom_->setTheme(theme);
    reverbDamp_->setTheme(theme);
    reverbMix_->setTheme(theme);

    noiseGatePedal_->setTheme(theme);
    noiseGateThreshold_->setTheme(theme);
    noiseGateAttack_->setTheme(theme);
    noiseGateRelease_->setTheme(theme);

    looperRecPedal_->setTheme(theme);
    looperPlayPedal_->setTheme(theme);
    looperClearPedal_->setTheme(theme);
    looperMix_->setTheme(theme);

    metronomePedal_->setTheme(theme);
    metronomeTempo_->setTheme(theme);
    metronomeVol_->setTheme(theme);

    setStyleSheet(QString("background-color: %1; border-radius: 10px;")
                  .arg(theme_.surface.name()));
}

void EffectsPanel::syncFromEngine() {
    if (!engine_) return;

    const auto& preset = engine_->getCurrentPreset();

    // Distortion
    distortionPedal_->setEnabled(preset.distortionEnabled);
    distortionDrive_->setValue(preset.distortionDrive * 100.0f);
    distortionTone_->setValue(preset.distortionTone * 100.0f);
    distortionLevel_->setValue(preset.distortionLevel * 100.0f);

    // Delay
    delayPedal_->setEnabled(preset.delayEnabled);
    delayTime_->setValue(preset.delayTimeMs);
    delayFeedback_->setValue(preset.delayFeedback * 100.0f);
    delayMix_->setValue(preset.delayMix * 100.0f);

    // Reverb
    reverbPedal_->setEnabled(preset.reverbEnabled);
    reverbRoom_->setValue(preset.reverbRoom * 100.0f);
    reverbDamp_->setValue(preset.reverbDamp * 100.0f);
    reverbMix_->setValue(preset.reverbMix * 100.0f);
}

} // namespace openamp

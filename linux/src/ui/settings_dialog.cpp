#include "settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QFrame>

namespace openamp {

SettingsDialog::SettingsDialog(AudioBackend* backend, QWidget* parent)
    : QDialog(parent)
    , backend_(backend)
{
    setupUI();
    populateDevices();
    applyTheme();
}

void SettingsDialog::setupUI() {
    setWindowTitle("Audio Settings");
    setMinimumWidth(450);
    setMinimumHeight(400);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Backend info header
    auto* headerLayout = new QHBoxLayout;
    
    backendLabel_ = new QLabel;
    backendLabel_->setFont(QFont(theme_.fontFamily, theme_.fontSizeNormal, QFont::Bold));
    headerLayout->addWidget(backendLabel_);
    
    headerLayout->addStretch();
    
    statusLabel_ = new QLabel;
    statusLabel_->setFont(QFont(theme_.fontFamily, theme_.fontSizeSmall));
    headerLayout->addWidget(statusLabel_);
    
    mainLayout->addLayout(headerLayout);

    // Separator
    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet(QString("background-color: %1; max-height: 1px;").arg(theme_.surfaceVariant.name()));
    mainLayout->addWidget(sep1);

    // Audio devices group
    auto* devicesGroup = new QGroupBox("Audio Devices");
    devicesGroup->setFont(QFont(theme_.fontFamily, theme_.fontSizeNormal, QFont::Bold));

    auto* devicesLayout = new QFormLayout(devicesGroup);
    devicesLayout->setSpacing(12);
    devicesLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    devicesLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    inputDeviceCombo_ = new QComboBox;
    inputDeviceCombo_->setMinimumWidth(280);
    devicesLayout->addRow("Input:", inputDeviceCombo_);

    outputDeviceCombo_ = new QComboBox;
    outputDeviceCombo_->setMinimumWidth(280);
    devicesLayout->addRow("Output:", outputDeviceCombo_);

    mainLayout->addWidget(devicesGroup);

    // Audio settings group
    auto* settingsGroup = new QGroupBox("Buffer Settings");
    settingsGroup->setFont(QFont(theme_.fontFamily, theme_.fontSizeNormal, QFont::Bold));

    auto* settingsLayout = new QFormLayout(settingsGroup);
    settingsLayout->setSpacing(12);
    settingsLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    settingsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sampleRateSpin_ = new QSpinBox;
    sampleRateSpin_->setRange(22050, 192000);
    sampleRateSpin_->setValue(48000);
    sampleRateSpin_->setSingleStep(1000);
    sampleRateSpin_->setSuffix(" Hz");
    sampleRateSpin_->setMinimumWidth(120);
    settingsLayout->addRow("Sample Rate:", sampleRateSpin_);

    bufferSizeSpin_ = new QSpinBox;
    bufferSizeSpin_->setRange(64, 4096);
    bufferSizeSpin_->setValue(256);
    bufferSizeSpin_->setSingleStep(64);
    bufferSizeSpin_->setSuffix(" samples");
    bufferSizeSpin_->setMinimumWidth(120);
    settingsLayout->addRow("Buffer Size:", bufferSizeSpin_);

    // Latency display
    latencyLabel_ = new QLabel;
    latencyLabel_->setFont(QFont(theme_.fontFamily, theme_.fontSizeSmall));
    latencyLabel_->setStyleSheet(QString("color: %1;").arg(theme_.textSecondary.name()));
    settingsLayout->addRow("Latency:", latencyLabel_);

    mainLayout->addWidget(settingsGroup);

    // Connect latency calculation
    connect(sampleRateSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::calculateLatency);
    connect(bufferSizeSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::calculateLatency);

    // Stretch to push buttons down
    mainLayout->addStretch();

    // Dialog buttons
    auto* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    auto* cancelButton = new QPushButton("Cancel");
    cancelButton->setMinimumWidth(100);
    buttonLayout->addWidget(cancelButton);

    auto* applyButton = new QPushButton("Apply");
    applyButton->setMinimumWidth(100);
    applyButton->setDefault(true);
    buttonLayout->addWidget(applyButton);

    mainLayout->addLayout(buttonLayout);

    // Connect buttons
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, [this]() {
        emit settingsChanged(getAudioConfig());
        accept();
    });

    // Initial latency calculation
    calculateLatency();
}

void SettingsDialog::populateDevices() {
    if (!backend_) {
        backendLabel_->setText("No Audio Backend");
        statusLabel_->setText("⚠ Not available");
        statusLabel_->setStyleSheet(QString("color: %1;").arg(theme_.error.name()));
        return;
    }

    backendLabel_->setText(QString("%1 %2")
        .arg(QString::fromStdString(backend_->getName()),
             QString::fromStdString(backend_->getVersion())));
    
    if (backend_->isInitialized()) {
        statusLabel_->setText("✓ Ready");
        statusLabel_->setStyleSheet(QString("color: %1;").arg(theme_.enabled.name()));
    } else {
        statusLabel_->setText("⚠ Not initialized");
        statusLabel_->setStyleSheet(QString("color: %1;").arg(theme_.warning.name()));
    }

    // Populate input devices
    inputDeviceCombo_->clear();
    auto inputDevices = backend_->getInputDevices();
    for (const auto& dev : inputDevices) {
        QString name = QString::fromStdString(dev.name);
        if (dev.isDefault) {
            name += " (Default)";
        }
        inputDeviceCombo_->addItem(name, QString::fromStdString(dev.id));
    }

    // Populate output devices
    outputDeviceCombo_->clear();
    auto outputDevices = backend_->getOutputDevices();
    for (const auto& dev : outputDevices) {
        QString name = QString::fromStdString(dev.name);
        if (dev.isDefault) {
            name += " (Default)";
        }
        outputDeviceCombo_->addItem(name, QString::fromStdString(dev.id));
    }

    // Select default devices
    auto defaultInput = backend_->getDefaultInputDevice();
    auto defaultOutput = backend_->getDefaultOutputDevice();
    
    int inputIndex = inputDeviceCombo_->findData(QString::fromStdString(defaultInput.id));
    if (inputIndex >= 0) inputDeviceCombo_->setCurrentIndex(inputIndex);
    
    int outputIndex = outputDeviceCombo_->findData(QString::fromStdString(defaultOutput.id));
    if (outputIndex >= 0) outputDeviceCombo_->setCurrentIndex(outputIndex);
}

void SettingsDialog::calculateLatency() {
    int sampleRate = sampleRateSpin_->value();
    int bufferSize = bufferSizeSpin_->value();
    double latencyMs = (static_cast<double>(bufferSize) / sampleRate) * 1000.0;
    
    QString quality;
    if (latencyMs < 5.0) {
        quality = "Ultra Low";
        latencyLabel_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme_.enabled.name()));
    } else if (latencyMs < 10.0) {
        quality = "Low";
        latencyLabel_->setStyleSheet(QString("color: %1;").arg(theme_.enabled.name()));
    } else if (latencyMs < 20.0) {
        quality = "Normal";
        latencyLabel_->setStyleSheet(QString("color: %1;").arg(theme_.textSecondary.name()));
    } else {
        quality = "High";
        latencyLabel_->setStyleSheet(QString("color: %1;").arg(theme_.warning.name()));
    }
    
    latencyLabel_->setText(QString("%1 ms (%2)").arg(latencyMs, 0, 'f', 1).arg(quality));
}

AudioConfig SettingsDialog::getAudioConfig() const {
    AudioConfig config;
    config.inputDeviceId = inputDeviceCombo_->currentData().toString().toStdString();
    config.outputDeviceId = outputDeviceCombo_->currentData().toString().toStdString();
    config.sampleRate = sampleRateSpin_->value();
    config.bufferSize = bufferSizeSpin_->value();
    config.inputChannels = 1;
    config.outputChannels = 2;
    return config;
}

void SettingsDialog::setAudioConfig(const AudioConfig& config) {
    int inputIndex = inputDeviceCombo_->findData(QString::fromStdString(config.inputDeviceId));
    if (inputIndex >= 0) inputDeviceCombo_->setCurrentIndex(inputIndex);

    int outputIndex = outputDeviceCombo_->findData(QString::fromStdString(config.outputDeviceId));
    if (outputIndex >= 0) outputDeviceCombo_->setCurrentIndex(outputIndex);

    sampleRateSpin_->setValue(config.sampleRate);
    bufferSizeSpin_->setValue(config.bufferSize);
}

void SettingsDialog::applyTheme() {
    QString baseStyle = QString(
        "QDialog {"
        "  background-color: %1;"
        "}"
        "QLabel {"
        "  color: %2;"
        "  font-family: %3;"
        "}"
        "QGroupBox {"
        "  color: %2;"
        "  font-family: %3;"
        "  font-weight: bold;"
        "  border: 2px solid %4;"
        "  border-radius: 8px;"
        "  margin-top: 12px;"
        "  padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 8px;"
        "}"
    ).arg(theme_.background.name(),
          theme_.textPrimary.name(),
          theme_.fontFamily,
          theme_.surfaceVariant.name());

    QString inputStyle = QString(
        "QComboBox, QSpinBox {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 2px solid %3;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-family: %4;"
        "  font-size: %5px;"
        "}"
        "QComboBox:hover, QSpinBox:hover {"
        "  border-color: %6;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 8px solid %2;"
        "  margin-right: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 2px solid %3;"
        "  selection-background-color: %6;"
        "  padding: 4px;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  background-color: %3;"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QSpinBox::up-arrow, QSpinBox::down-arrow {"
        "  width: 12px;"
        "  height: 12px;"
        "}"
    ).arg(theme_.surface.name(),
          theme_.textPrimary.name(),
          theme_.surfaceVariant.name(),
          theme_.fontFamily,
          QString::number(theme_.fontSizeNormal),
          theme_.accent.name());

    QString buttonStyle = QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 10px 24px;"
        "  font-family: %3;"
        "  font-size: %4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: %5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %6;"
        "}"
        "QPushButton#cancel {"
        "  background-color: %7;"
        "}"
        "QPushButton#cancel:hover {"
        "  background-color: %8;"
        "}"
    ).arg(theme_.accent.name(),
          theme_.textPrimary.name(),
          theme_.fontFamily,
          QString::number(theme_.fontSizeNormal),
          theme_.accentLight.name(),
          theme_.accentDark.name(),
          theme_.surfaceVariant.name(),
          theme_.surface.name());

    setStyleSheet(baseStyle + inputStyle + buttonStyle);
    
    // Apply button-specific styles
    for (auto* btn : findChildren<QPushButton*>()) {
        if (btn->text() == "Cancel") {
            btn->setObjectName("cancel");
        }
        btn->setStyleSheet(buttonStyle);
    }
}

} // namespace openamp

#include "main_window.h"
#include "amp_panel.h"
#include "effects_panel.h"
#include "preset_browser.h"
#include "settings_dialog.h"
#include "meter_widget.h"
#include "ir_panel_widget.h"
#include "latency_display_widget.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>

namespace openamp {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , engine_(std::make_unique<DSPEngine>())
    , audio_(createBestBackend())
    , midi_(std::make_unique<MidiHandler>())
    , theme_(Theme::dark())
{
    setWindowTitle("Guitar Amp");
    setMinimumSize(1200, 700);
    resize(1400, 800);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();

    // Initialize audio backend
    if (audio_ && audio_->initialize()) {
        AudioConfig config;
        config.sampleRate = 48000;
        config.bufferSize = 256;
        audio_->setConfig(config);
    }

    // Initialize MIDI
    if (midi_) {
        midi_->initialize();
    }

    // Load last settings
    QSettings settings("Synthalorian", "OpenAmp");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Load last preset
    QString lastPreset = settings.value("lastPreset").toString();
    if (!lastPreset.isEmpty()) {
        loadPreset(lastPreset);
    }
}

MainWindow::~MainWindow() {
    stopAudio();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings("Synthalorian", "OpenAmp");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastPreset", presetLabel_->text());

    stopAudio();
    event->accept();
}

void MainWindow::setupUI() {
    centralWidget_ = new QWidget;
    setCentralWidget(centralWidget_);

    auto* mainLayout = new QHBoxLayout(centralWidget_);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Create splitter for resizable panels
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left panel - Preset browser + Input meter
    auto* leftPanel = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(5);
    
    presetBrowser_ = new PresetBrowser(engine_.get());
    presetBrowser_->setMinimumWidth(200);
    presetBrowser_->setMaximumWidth(300);
    leftLayout->addWidget(presetBrowser_, 1);
    
    // Input meter
    auto* inputMeterLayout = new QVBoxLayout;
    auto* inputLabel = new QLabel("Input");
    inputLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
                              .arg(theme_.textSecondary.name()));
    inputLabel->setAlignment(Qt::AlignCenter);
    inputMeterLayout->addWidget(inputLabel);
    
    inputMeter_ = new LevelMeter;
    inputMeter_->setStereo(false);
    inputMeter_->setTheme(theme_);
    inputMeterLayout->addWidget(inputMeter_);
    leftLayout->addLayout(inputMeterLayout);
    
    leftPanel->setMinimumWidth(200);
    leftPanel->setMaximumWidth(300);
    splitter->addWidget(leftPanel);

    // Center panel - Amp controls + Output meter
    auto* centerPanel = new QWidget;
    auto* centerLayout = new QHBoxLayout(centerPanel);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(5);
    
    ampPanel_ = new AmpPanel(engine_.get());
    ampPanel_->setMinimumWidth(350);
    centerLayout->addWidget(ampPanel_, 1);
    
    // Output meter
    auto* outputMeterLayout = new QVBoxLayout;
    auto* outputLabel = new QLabel("Output");
    outputLabel->setStyleSheet(QString("color: %1; font-weight: bold;")
                               .arg(theme_.textSecondary.name()));
    outputLabel->setAlignment(Qt::AlignCenter);
    outputMeterLayout->addWidget(outputLabel);
    
    outputMeter_ = new LevelMeter;
    outputMeter_->setStereo(true);
    outputMeter_->setTheme(theme_);
    outputMeterLayout->addWidget(outputMeter_);
    centerLayout->addLayout(outputMeterLayout);
    
    splitter->addWidget(centerPanel);

    // Right panel - Effects + IR Loader + Tuner + Latency
    auto* rightPanel = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(5);
    
    // Tuner display
    tunerDisplay_ = new TunerDisplay;
    tunerDisplay_->setTheme(theme_);
    tunerDisplay_->setMaximumHeight(120);
    tunerDisplay_->hide();  // Hidden by default
    rightLayout->addWidget(tunerDisplay_);
    
    effectsPanel_ = new EffectsPanel(engine_.get());
    effectsPanel_->setMinimumWidth(350);
    rightLayout->addWidget(effectsPanel_, 1);
    
    // IR Loader panel
    irPanel_ = new IRPanelWidget(engine_.get());
    irPanel_->setTheme(theme_);
    irPanel_->setMinimumWidth(350);
    rightLayout->addWidget(irPanel_);
    
    // Latency display
    latencyDisplay_ = new LatencyDisplayWidget(engine_.get());
    latencyDisplay_->setTheme(theme_);
    rightLayout->addWidget(latencyDisplay_);
    
    splitter->addWidget(rightPanel);

    // Set initial sizes
    splitter->setSizes({250, 500, 500});

    mainLayout->addWidget(splitter);

    // Meter update timer
    meterTimer_ = new QTimer(this);
    connect(meterTimer_, &QTimer::timeout, this, &MainWindow::updateMeters);
    meterTimer_->start(33);  // ~30 FPS

    // Set dark theme
    setStyleSheet(QString(
        "QMainWindow {"
        "  background-color: %1;"
        "}"
        "QSplitter::handle {"
        "  background-color: %2;"
        "}"
        "QSplitter::handle:horizontal {"
        "  width: 2px;"
        "}"
    ).arg(theme_.background.name(),
          theme_.surfaceVariant.name()));
}

void MainWindow::setupMenuBar() {
    // File menu
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* openAction = fileMenu->addAction("&Open Preset...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] {
        QString path = QFileDialog::getOpenFileName(this, "Open Preset",
                                                     QString(), "Preset Files (*.preset)");
        if (!path.isEmpty()) {
            loadPreset(path);
        }
    });

    auto* saveAction = fileMenu->addAction("&Save Preset...");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] {
        QString path = QFileDialog::getSaveFileName(this, "Save Preset",
                                                     QString(), "Preset Files (*.preset)");
        if (!path.isEmpty()) {
            savePreset(path);
        }
    });

    fileMenu->addSeparator();

    auto* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    auto* editMenu = menuBar()->addMenu("&Edit");

    auto* settingsAction = editMenu->addAction("&Audio Settings...");
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);

    // View menu
    auto* viewMenu = menuBar()->addMenu("&View");

    auto* darkThemeAction = viewMenu->addAction("&Dark Theme");
    darkThemeAction->setCheckable(true);
    darkThemeAction->setChecked(true);
    connect(darkThemeAction, &QAction::triggered, this, [this](bool checked) {
        theme_ = checked ? Theme::dark() : Theme::light();
        ampPanel_->setTheme(theme_);
        effectsPanel_->setTheme(theme_);
    });

    // Help menu
    auto* helpMenu = menuBar()->addMenu("&Help");

    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    auto* aboutQtAction = helpMenu->addAction("About &Qt");
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);

    helpMenu->addSeparator();

    auto* supportAction = helpMenu->addAction("☕ Buy Me a Coffee");
    connect(supportAction, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl("https://buymeacoffee.com/synthalorian"));
    });
}

void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setStyleSheet(QString(
        "QToolBar {"
        "  background-color: %1;"
        "  border: none;"
        "  spacing: 10px;"
        "  padding: 5px;"
        "}"
        "QToolButton {"
        "  background-color: %2;"
        "  color: %3;"
        "  border: none;"
        "  border-radius: 5px;"
        "  padding: 8px 15px;"
        "}"
        "QToolButton:hover {"
        "  background-color: %4;"
        "}"
    ).arg(theme_.surface.name(),
          theme_.accent.name(),
          theme_.textPrimary.name(),
          theme_.accentLight.name()));

    // Audio toggle button
    audioToggleAction_ = toolbar->addAction("▶ Start Audio");
    connect(audioToggleAction_, &QAction::triggered, this, &MainWindow::toggleAudio);

    toolbar->addSeparator();

    // Preset navigation
    auto* prevPresetAction = toolbar->addAction("◄ Prev");
    auto* nextPresetAction = toolbar->addAction("Next ►");

    toolbar->addSeparator();

    // Panic button
    auto* panicAction = toolbar->addAction("⚠ Panic");
    connect(panicAction, &QAction::triggered, this, [this] {
        stopAudio();
        if (engine_) engine_->reset();
    });
    
    toolbar->addSeparator();
    
    // Tuner toggle
    tunerToggleAction_ = toolbar->addAction("🎸 Tuner");
    tunerToggleAction_->setCheckable(true);
    connect(tunerToggleAction_, &QAction::toggled, this, &MainWindow::toggleTuner);
}

void MainWindow::setupStatusBar() {
    auto* statusBar = this->statusBar();
    statusBar->setStyleSheet(QString(
        "QStatusBar {"
        "  background-color: %1;"
        "  color: %2;"
        "}"
    ).arg(theme_.surface.name(),
          theme_.textSecondary.name()));

    statusLabel_ = new QLabel("Ready");
    statusBar->addWidget(statusLabel_);

    latencyLabel_ = new QLabel("Latency: -- ms");
    statusBar->addPermanentWidget(latencyLabel_);

    presetLabel_ = new QLabel("No preset loaded");
    statusBar->addPermanentWidget(presetLabel_);
}

void MainWindow::connectSignals() {
    connect(presetBrowser_, &PresetBrowser::presetLoaded,
            this, &MainWindow::updatePresetDisplay);

    connect(ampPanel_, &AmpPanel::settingsChanged, this, [this] {
        presetLabel_->setText("* Modified");
    });

    connect(effectsPanel_, &EffectsPanel::settingsChanged, this, [this] {
        presetLabel_->setText("* Modified");
    });

    connect(irPanel_, &IRPanelWidget::settingsChanged, this, [this] {
        presetLabel_->setText("* Modified");
    });

    connect(irPanel_, &IRPanelWidget::irLoaded, this, [this](const QString& name) {
        statusLabel_->setText(QString("Loaded IR: %1").arg(name));
    });
}

void MainWindow::startAudio() {
    if (!audio_ || audioRunning_) return;

    engine_->prepare(audio_->getConfig().sampleRate, audio_->getConfig().bufferSize);

    auto callback = [this](float* input, float* output, uint32_t frames) {
        // Process input in-place through DSP chain
        engine_->process(input, frames);

        // Convert mono processed input to stereo output
        // Output buffer is interleaved: [L0, R0, L1, R1, L2, R2, ...]
        for (uint32_t i = 0; i < frames; ++i) {
            float sample = input[i];
            output[i * 2] = sample;      // Left channel
            output[i * 2 + 1] = sample;  // Right channel
        }
    };

    if (audio_->start(callback)) {
        audioRunning_ = true;
        audioToggleAction_->setText("⏹ Stop Audio");
        statusLabel_->setText("Audio running");
        updateLatencyDisplay();
        
        // Start latency monitoring (10Hz update)
        if (latencyDisplay_) {
            latencyDisplay_->startMonitoring(100);
        }
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to start audio: %1")
                .arg(QString::fromStdString(audio_->getLastError())));
    }
}

void MainWindow::stopAudio() {
    if (!audio_ || !audioRunning_) return;

    audio_->stop();
    audioRunning_ = false;
    audioToggleAction_->setText("▶ Start Audio");
    statusLabel_->setText("Audio stopped");
    
    // Stop latency monitoring
    if (latencyDisplay_) {
        latencyDisplay_->stopMonitoring();
    }
}

void MainWindow::toggleAudio() {
    if (audioRunning_) {
        stopAudio();
    } else {
        startAudio();
    }
}

void MainWindow::showSettings() {
    SettingsDialog dialog(audio_.get(), this);

    if (audio_) {
        dialog.setAudioConfig(audio_->getConfig());
    }

    connect(&dialog, &SettingsDialog::settingsChanged, this, [this](const AudioConfig& config) {
        if (audio_) {
            bool wasRunning = audioRunning_;
            if (wasRunning) stopAudio();
            audio_->setConfig(config);
            if (wasRunning) startAudio();
            updateLatencyDisplay();
        }
    });

    dialog.exec();
}

void MainWindow::showAbout() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("About Guitar Amp");
    aboutBox.setText(
        "<h2>Guitar Amp</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A professional guitar amplifier and effects processor.</p>"
        "<p>Audio Backend: " + QString::fromStdString(audio_ ? audio_->getName() : "None") + "</p>"
        "<p>© 2024 Synthalorian</p>"
        "<p>Built with Qt 6 and " + QString::fromStdString(audio_ ? audio_->getVersion() : "") + "</p>"
    );
    
    // Add support button
    auto* supportButton = aboutBox.addButton("☕ Support Development", QMessageBox::ActionRole);
    connect(supportButton, &QPushButton::clicked, [] {
        QDesktopServices::openUrl(QUrl("https://buymeacoffee.com/synthalorian"));
    });
    
    aboutBox.addButton(QMessageBox::Ok);
    aboutBox.setDefaultButton(QMessageBox::Ok);
    
    aboutBox.exec();
}

void MainWindow::loadPreset(const QString& path) {
    if (!engine_) return;

    std::string error;
    if (engine_->loadPreset(path.toStdString(), error)) {
        ampPanel_->syncFromEngine();
        effectsPanel_->syncFromEngine();
        if (irPanel_) irPanel_->syncFromEngine();
        updatePresetDisplay(QFileInfo(path).baseName());
    } else {
        QMessageBox::warning(this, "Error", QString::fromStdString(error));
    }
}

void MainWindow::savePreset(const QString& path) {
    if (!engine_) return;

    std::string error;
    if (!engine_->savePreset(path.toStdString(), error)) {
        QMessageBox::warning(this, "Error", QString::fromStdString(error));
    }
}

void MainWindow::updateLatencyDisplay() {
    if (!audio_) return;

    double inputLatency = audio_->getInputLatency();
    double outputLatency = audio_->getOutputLatency();
    double totalLatency = inputLatency + outputLatency;

    latencyLabel_->setText(QString("Latency: %1 ms (In: %2, Out: %3)")
        .arg(totalLatency, 0, 'f', 1)
        .arg(inputLatency, 0, 'f', 1)
        .arg(outputLatency, 0, 'f', 1));
}

void MainWindow::updatePresetDisplay(const QString& name) {
    presetLabel_->setText(name);
}

void MainWindow::updateMeters() {
    if (!audio_) return;
    
    // Get real meter levels from audio backend
    auto inputLevels = audio_->getInputLevels();
    auto outputLevels = audio_->getOutputLevels();
    
    if (inputMeter_) {
        inputMeter_->setLevel(inputLevels.leftDb);
        inputMeter_->setPeak(inputLevels.leftPeakDb, inputLevels.rightPeakDb);
    }
    if (outputMeter_) {
        outputMeter_->setLevel(outputLevels.leftDb, outputLevels.rightDb);
        outputMeter_->setPeak(outputLevels.leftPeakDb, outputLevels.rightPeakDb);
    }
}

void MainWindow::toggleTuner(bool enabled) {
    tunerEnabled_ = enabled;
    if (tunerDisplay_) {
        tunerDisplay_->setVisible(enabled);
        if (enabled) {
            tunerDisplay_->setActive(false);
            tunerDisplay_->setNote("--", 0, 0.0f);
        }
    }
}

} // namespace openamp

#include "latency_display_widget.h"
#include "openamp/dsp_engine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

namespace openamp {

LatencyDisplayWidget::LatencyDisplayWidget(DSPEngine* engine, QWidget* parent)
    : QWidget(parent)
    , engine_(engine)
    , theme_(Theme::dark())
{
    setupUI();
}

void LatencyDisplayWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Title
    auto* titleLabel = new QLabel("Latency Monitor");
    titleLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
    ).arg(theme_.textPrimary.name()));
    mainLayout->addWidget(titleLabel);

    // Separator
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

    // Latency value (main display)
    auto* latencyLayout = new QHBoxLayout;
    
    auto* latencyLabel = new QLabel("Latency:");
    latencyLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 13px;"
        "}"
    ).arg(theme_.textSecondary.name()));
    latencyLayout->addWidget(latencyLabel);
    
    latencyValueLabel_ = new QLabel("-- ms");
    latencyValueLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "}"
    ).arg(theme_.accent.name()));
    latencyValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    latencyLayout->addWidget(latencyValueLabel_, 1);
    
    mainLayout->addLayout(latencyLayout);

    // Theoretical latency
    auto* theoreticalLayout = new QHBoxLayout;
    
    auto* theoreticalLabelText = new QLabel("Theoretical:");
    theoreticalLabelText->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 11px;"
        "}"
    ).arg(theme_.textMuted.name()));
    theoreticalLayout->addWidget(theoreticalLabelText);
    
    theoreticalLabel_ = new QLabel("-- ms");
    theoreticalLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 11px;"
        "}"
    ).arg(theme_.textMuted.name()));
    theoreticalLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    theoreticalLayout->addWidget(theoreticalLabel_, 1);
    
    mainLayout->addLayout(theoreticalLayout);

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

    // Buffer size
    auto* bufferLayout = new QHBoxLayout;
    
    auto* bufferLabel = new QLabel("Buffer Size:");
    bufferLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 12px;"
        "}"
    ).arg(theme_.textSecondary.name()));
    bufferLayout->addWidget(bufferLabel);
    
    bufferSizeLabel_ = new QLabel("-- samples");
    bufferSizeLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 12px;"
        "}"
    ).arg(theme_.textPrimary.name()));
    bufferSizeLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bufferLayout->addWidget(bufferSizeLabel_, 1);
    
    mainLayout->addLayout(bufferLayout);

    // Sample rate
    auto* sampleRateLayout = new QHBoxLayout;
    
    auto* sampleRateLabel = new QLabel("Sample Rate:");
    sampleRateLabel->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 12px;"
        "}"
    ).arg(theme_.textSecondary.name()));
    sampleRateLayout->addWidget(sampleRateLabel);
    
    sampleRateLabel_ = new QLabel("-- kHz");
    sampleRateLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 12px;"
        "}"
    ).arg(theme_.textPrimary.name()));
    sampleRateLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sampleRateLayout->addWidget(sampleRateLabel_, 1);
    
    mainLayout->addLayout(sampleRateLayout);

    // Set background style
    setStyleSheet(QString(
        "LatencyDisplayWidget {"
        "  background-color: %1;"
        "  border-radius: 8px;"
        "}"
    ).arg(theme_.surface.name()));

    // Set minimum size
    setMinimumWidth(180);
    setMaximumHeight(180);
}

void LatencyDisplayWidget::startMonitoring(int updateIntervalMs) {
    if (!updateTimer_) {
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, &LatencyDisplayWidget::updateDisplay);
    }
    updateTimer_->start(updateIntervalMs);
    updateDisplay();  // Initial update
}

void LatencyDisplayWidget::stopMonitoring() {
    if (updateTimer_) {
        updateTimer_->stop();
    }
}

void LatencyDisplayWidget::updateDisplay() {
    if (!engine_) return;

    // Get latency values
    float latencyMs = engine_->getLatencyMs();
    float theoreticalMs = engine_->getTheoreticalLatencyMs();
    uint32_t bufferSize = engine_->getBufferSize();
    double sampleRate = engine_->getSampleRate();

    // Update labels
    latencyValueLabel_->setText(QString("%1 ms").arg(latencyMs, 0, 'f', 1));
    theoreticalLabel_->setText(QString("%1 ms").arg(theoreticalMs, 0, 'f', 2));
    bufferSizeLabel_->setText(QString("%1 samples").arg(bufferSize));
    sampleRateLabel_->setText(QString("%1 kHz").arg(sampleRate / 1000.0, 0, 'f', 1));

    // Color-code latency based on severity
    QString latencyColor;
    if (latencyMs < 10.0f) {
        latencyColor = theme_.enabled.name();  // Green - good
    } else if (latencyMs < 20.0f) {
        latencyColor = theme_.warning.name();  // Yellow/Orange - acceptable
    } else {
        latencyColor = theme_.error.name();    // Red - high latency
    }

    latencyValueLabel_->setStyleSheet(QString(
        "QLabel {"
        "  color: %1;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "}"
    ).arg(latencyColor));
}

void LatencyDisplayWidget::setTheme(const Theme& theme) {
    theme_ = theme;
    setStyleSheet(QString(
        "LatencyDisplayWidget {"
        "  background-color: %1;"
        "  border-radius: 8px;"
        "}"
    ).arg(theme_.surface.name()));
    update();
}

} // namespace openamp

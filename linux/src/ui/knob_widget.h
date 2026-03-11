#pragma once

#include <QWidget>
#include <QString>
#include "theme.h"

namespace openamp {

class KnobWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(float minValue READ minValue WRITE setMinValue)
    Q_PROPERTY(float maxValue READ maxValue WRITE setMaxValue)
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(QString valueSuffix READ valueSuffix WRITE setValueSuffix)

public:
    explicit KnobWidget(QWidget* parent = nullptr);

    float value() const { return value_; }
    void setValue(float value);

    float minValue() const { return minValue_; }
    void setMinValue(float min) { minValue_ = min; update(); }

    float maxValue() const { return maxValue_; }
    void setMaxValue(float max) { maxValue_ = max; update(); }

    QString label() const { return label_; }
    void setLabel(const QString& label) { label_ = label; update(); }

    QString valueSuffix() const { return valueSuffix_; }
    void setValueSuffix(const QString& suffix) { valueSuffix_ = suffix; update(); }

    void setTheme(const Theme& theme) { theme_ = theme; update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(float value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    float value_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    QString label_;
    QString valueSuffix_;
    Theme theme_;

    bool dragging_ = false;
    int lastY_ = 0;
    bool hovered_ = false;

    float normalizedValue() const;
    void setNormalizedValue(float norm);
};

} // namespace openamp

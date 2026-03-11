#pragma once

#include <QWidget>
#include <QString>
#include "theme.h"

namespace openamp {

class PedalWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY toggled)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    explicit PedalWidget(QWidget* parent = nullptr);

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled);

    QString name() const { return name_; }
    void setName(const QString& name) { name_ = name; update(); }

    QColor color() const { return color_; }
    void setColor(const QColor& color) { color_ = color; update(); }

    void setTheme(const Theme& theme) { theme_ = theme; update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void toggled(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    bool enabled_ = false;
    QString name_;
    QColor color_ = QColor(255, 140, 0);
    Theme theme_;
    bool hovered_ = false;
};

} // namespace openamp

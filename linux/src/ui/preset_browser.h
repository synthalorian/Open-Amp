#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <memory>
#include "theme.h"
#include "openamp/dsp_engine.h"

namespace openamp {

class PresetBrowser : public QWidget {
    Q_OBJECT

public:
    explicit PresetBrowser(DSPEngine* engine, QWidget* parent = nullptr);

    void setTheme(const Theme& theme);
    void loadFactoryPresets();
    void loadUserPresets();

signals:
    void presetLoaded(const QString& name);

private:
    void setupUI();
    void connectSignals();
    void populatePresetList();
    QString getPresetPath(const QString& name, bool factory = true);

    DSPEngine* engine_;
    Theme theme_;

    QLineEdit* searchBox_ = nullptr;
    QListWidget* presetList_ = nullptr;
    QPushButton* saveButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;

    QString factoryPresetsPath_;
    QString userPresetsPath_;
};

} // namespace openamp

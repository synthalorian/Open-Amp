#include "preset_browser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardPaths>

namespace openamp {

PresetBrowser::PresetBrowser(DSPEngine* engine, QWidget* parent)
    : QWidget(parent)
    , engine_(engine)
{
    // Set up preset paths
    factoryPresetsPath_ = QDir::currentPath() + "/../presets/factory";
    userPresetsPath_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/presets";

    // Ensure user presets directory exists
    QDir().mkpath(userPresetsPath_);

    setupUI();
    connectSignals();
    loadFactoryPresets();
}

void PresetBrowser::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title
    auto* titleLabel = new QLabel("Presets");
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

    // Search box
    searchBox_ = new QLineEdit;
    searchBox_->setPlaceholderText("Search presets...");
    searchBox_->setStyleSheet(QString(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "}"
    ).arg(theme_.surfaceVariant.name(),
          theme_.textPrimary.name(),
          theme_.textMuted.name()));
    mainLayout->addWidget(searchBox_);

    // Preset list
    presetList_ = new QListWidget;
    presetList_->setStyleSheet(QString(
        "QListWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 5px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid %4;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: %5;"
        "  color: %6;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: %7;"
        "}"
    ).arg(theme_.surfaceVariant.name(),
          theme_.textPrimary.name(),
          theme_.textMuted.name(),
          theme_.surface.name(),
          theme_.accent.name(),
          theme_.textPrimary.name(),
          theme_.accentDark.name()));
    mainLayout->addWidget(presetList_);

    // Buttons
    auto* buttonLayout = new QHBoxLayout;

    saveButton_ = new QPushButton("Save As...");
    saveButton_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 5px;"
        "  padding: 10px 20px;"
        "  font-weight: bold;"
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

    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: 5px;"
        "  padding: 10px 20px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %3;"
        "}"
    ).arg(theme_.surface.name(),
          theme_.textSecondary.name(),
          theme_.error.name()));

    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(deleteButton_);
    mainLayout->addLayout(buttonLayout);

    setStyleSheet(QString("background-color: %1; border-radius: 10px;")
                  .arg(theme_.surface.name()));
}

void PresetBrowser::connectSignals() {
    connect(searchBox_, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < presetList_->count(); ++i) {
            QListWidgetItem* item = presetList_->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });

    connect(presetList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString name = item->text();
        QString path = getPresetPath(name);
        if (path.isEmpty()) {
            path = getPresetPath(name, false);
        }

        if (!path.isEmpty() && engine_) {
            std::string error;
            if (engine_->loadPreset(path.toStdString(), error)) {
                emit presetLoaded(name);
            } else {
                QMessageBox::warning(this, "Error", QString::fromStdString(error));
            }
        }
    });

    connect(saveButton_, &QPushButton::clicked, this, [this] {
        bool ok;
        QString name = QInputDialog::getText(this, "Save Preset",
                                             "Preset name:", QLineEdit::Normal,
                                             "", &ok);
        if (ok && !name.isEmpty() && engine_) {
            QString path = userPresetsPath_ + "/" + name + ".preset";
            std::string error;
            if (engine_->savePreset(path.toStdString(), error)) {
                loadUserPresets();
            } else {
                QMessageBox::warning(this, "Error", QString::fromStdString(error));
            }
        }
    });

    connect(deleteButton_, &QPushButton::clicked, this, [this] {
        QListWidgetItem* item = presetList_->currentItem();
        if (!item) return;

        QString name = item->text();
        QString path = getPresetPath(name, false);
        if (path.isEmpty()) {
            QMessageBox::information(this, "Info", "Cannot delete factory presets");
            return;
        }

        auto reply = QMessageBox::question(this, "Confirm Delete",
                                          QString("Delete preset '%1'?").arg(name),
                                          QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QFile::remove(path);
            loadUserPresets();
        }
    });
}

void PresetBrowser::loadFactoryPresets() {
    QDir dir(factoryPresetsPath_);
    if (!dir.exists()) return;

    QStringList filters;
    filters << "*.preset";
    dir.setNameFilters(filters);

    presetList_->clear();
    for (const QString& file : dir.entryList()) {
        QString name = file;
        name.chop(7);  // Remove ".preset"
        auto* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, factoryPresetsPath_ + "/" + file);
        presetList_->addItem(item);
    }
}

void PresetBrowser::loadUserPresets() {
    loadFactoryPresets();

    QDir dir(userPresetsPath_);
    if (!dir.exists()) return;

    QStringList filters;
    filters << "*.preset";
    dir.setNameFilters(filters);

    for (const QString& file : dir.entryList()) {
        QString name = file;
        name.chop(7);
        auto* item = new QListWidgetItem(name + " (User)");
        item->setData(Qt::UserRole, userPresetsPath_ + "/" + file);
        presetList_->addItem(item);
    }
}

void PresetBrowser::setTheme(const Theme& theme) {
    theme_ = theme;
    // Reapply stylesheet with new theme colors
    setupUI();
}

QString PresetBrowser::getPresetPath(const QString& name, bool factory) {
    QString baseName = name;
    baseName.remove(" (User)");

    if (factory) {
        QString path = factoryPresetsPath_ + "/" + baseName + ".preset";
        if (QFile::exists(path)) return path;
    } else {
        QString path = userPresetsPath_ + "/" + baseName + ".preset";
        if (QFile::exists(path)) return path;
    }

    return QString();
}

} // namespace openamp

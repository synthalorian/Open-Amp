#include <QApplication>
#include <QIcon>
#include <QFontDatabase>
#include <QFile>
#include "ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("OpenAmp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Synthalorian");

    // Set application style
    app.setStyle("Fusion");

    // Load custom font if available
    QFontDatabase fontDb;
    if (fontDb.families().contains("Inter")) {
        QFont font("Inter", 13);
        app.setFont(font);
    } else {
        QFont font = app.font();
        font.setFamily("sans-serif");
        app.setFont(font);
    }

    // Create and show main window
    openamp::MainWindow window;
    window.show();

    return app.exec();
}

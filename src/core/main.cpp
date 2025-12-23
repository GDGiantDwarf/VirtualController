#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application info
    QApplication::setApplicationName("Game Library Launcher");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Student Project");
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
#include <QApplication>
#include <QStringList>
#include <QLockFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <cstdlib>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(true);
    
    // When quit is requested, forcefully exit after brief cleanup
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        std::exit(0);
    });
    
    // Default server target
    QString serverHost = "127.0.0.1";
    int serverPort = 8765;
    
    // Parse optional CLI args: <host> <port>
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        serverHost = args[1];
    }
    if (args.size() > 2) {
        bool ok = false;
        int parsedPort = args[2].toInt(&ok);
        if (ok && parsedPort > 0) {
            serverPort = parsedPort;
        }
    }
    
    // Single-instance guard
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + "/GameLibraryLauncher.lock";
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock(100)) {
        QMessageBox::warning(nullptr, "Already Running",
                             "Game Library Launcher is already running.");
        return 1;
    }
    
    // Set application info
    QApplication::setApplicationName("Game Library Launcher");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Student Project");
    
    // Create and show main window
    MainWindow window(serverHost, serverPort);
    window.show();
    
    int rc = app.exec();
    lockFile.unlock();
    // Force process termination to ensure no lingering processes
    std::exit(0);
}
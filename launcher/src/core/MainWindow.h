#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include "GameLibraryTab.h"
#include "ControllerTab.h"
#include "WebSocketTab.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(const QString& serverHost = "127.0.0.1", int serverPort = 8765, QWidget* parent = nullptr);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private:
    void setupUI();
    void createMenuBar();
    
    QTabWidget* tabWidget;
    GameLibraryTab* libraryTab;
    ControllerTab* controllerTab;
    QString serverHost;
    int serverPort;
    WebSocketTab* webSocketTab;
};

#endif // MAINWINDOW_H
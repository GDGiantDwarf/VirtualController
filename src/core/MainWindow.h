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
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private:
    void setupUI();
    void createMenuBar();
    
    QTabWidget* tabWidget;
    GameLibraryTab* libraryTab;
    ControllerTab* controllerTab;
    WebSocketTab* webSocketTab;
};

#endif // MAINWINDOW_H
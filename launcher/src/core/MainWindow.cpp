#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMetaObject>
#include <QMessageBox>
#include <cstdlib>

MainWindow::MainWindow(const QString& serverHostIn, int serverPortIn, QWidget* parent)
    : QMainWindow(parent)
    , serverHost(serverHostIn)
    , serverPort(serverPortIn)
{
    setAttribute(Qt::WA_QuitOnClose, true);
    setupUI();
    createMenuBar();
    
    setWindowTitle("Game Library Launcher");
    resize(800, 600);
}

MainWindow::~MainWindow() {
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Accept the close event and let Qt's aboutToQuit signal handler exit
    if (event) event->accept();
}

void MainWindow::setupUI() {
    // Create tab widget
    tabWidget = new QTabWidget(this);
    
    // Create and add tabs
    controllerTab = new ControllerTab(this);
    libraryTab = new GameLibraryTab(this, serverHost, serverPort);
    
    tabWidget->addTab(controllerTab, "Connect a Controller");
    tabWidget->addTab(libraryTab, "Game Library");
    
    // Set as central widget
    setCentralWidget(tabWidget);
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* refreshAction = new QAction("&Refresh Games", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, [this]() {
        // Switch to library tab and trigger refresh
        tabWidget->setCurrentWidget(libraryTab);
    });
    fileMenu->addAction(refreshAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About Game Library Launcher",
            "Game Library Launcher v1.0\n\n"
            "A simple game launcher with virtual controller support.\n"
            "Created for educational purposes.");
    });
    helpMenu->addAction(aboutAction);
}
#include "GameLibraryTab.h"
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>  // ← AJOUTEZ CETTE LIGNE

GameLibraryTab::GameLibraryTab(QWidget* parent, const QString& serverHostIn, int serverPortIn)
    : QWidget(parent)
    , scanner(new GameScanner(this))
    , serverHost(serverHostIn)
    , serverPort(serverPortIn)
{
    setupUI();
    loadGames();
}

void GameLibraryTab::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Refresh button
    refreshButton = new QPushButton("Refresh Game Library", this);
    connect(refreshButton, &QPushButton::clicked, this, &GameLibraryTab::onRefreshClicked);
    layout->addWidget(refreshButton);
    
    // Games list
    gamesList = new QListWidget(this);
    gamesList->setIconSize(QSize(48, 48));
    gamesList->setSpacing(5);
    connect(gamesList, &QListWidget::itemClicked, this, &GameLibraryTab::onGameClicked);
    layout->addWidget(gamesList);
    
    setLayout(layout);
}

void GameLibraryTab::loadGames() {
    gamesList->clear();
    games = scanner->scanGames("./games");
    
    if (games.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No games found in ./games/ directory");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        gamesList->addItem(item);
        return;
    }
    
    for (const GameInfo& game : games) {
        QListWidgetItem* item = new QListWidgetItem(game.icon, game.name);
        item->setData(Qt::UserRole, QVariant::fromValue(game));
        gamesList->addItem(item);
    }
}

void GameLibraryTab::onGameClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QVariant data = item->data(Qt::UserRole);
    if (!data.canConvert<GameInfo>()) return;
    
    GameInfo game = data.value<GameInfo>();
    launchGame(game);
}

void GameLibraryTab::onRefreshClicked() {
    loadGames();
}

void GameLibraryTab::launchGame(const GameInfo& game) {
    qDebug() << "Launching game:" << game.name << "at" << game.executablePath;
    
    try {
        // Set working directory to game folder
        QFileInfo exeInfo(game.executablePath);
        QString workingDir = exeInfo.absolutePath();

        if (!exeInfo.exists() || !exeInfo.isFile()) {
            QMessageBox::critical(this, "Launch Error",
                QString("Executable not found for '%1'.\nExpected: %2")
                    .arg(game.name, game.executablePath));
            return;
        }
        
        qDebug() << "Working directory:" << workingDir;
        qDebug() << "Executable:" << game.executablePath;
        
        // Start the game with server host/port arguments
        QStringList args;
        args << serverHost << QString::number(serverPort);
        
        qDebug() << "Starting process with args:" << args;
        
        // Launch detached so the game runs independently of the launcher
        qint64 pid;
        bool success = QProcess::startDetached(game.executablePath, args, workingDir, &pid);
        
        qDebug() << "startDetached returned:" << success << "PID:" << pid;
        
        if (!success) {
            QMessageBox::critical(this, "Launch Error", 
                QString("Failed to start '%1'.\nExecutable: %2\nWorking dir: %3")
                    .arg(game.name, game.executablePath, workingDir));
        } else {
            qDebug() << "Game" << game.name << "started with PID" << pid;
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception during launch:" << e.what();
        QMessageBox::critical(this, "Launch Error", 
            QString("Exception: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "Unknown exception during launch";
        QMessageBox::critical(this, "Launch Error", "Unknown exception occurred");
    }
}
#include "GameLibraryTab.h"
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>  // ← AJOUTEZ CETTE LIGNE

GameLibraryTab::GameLibraryTab(QWidget* parent)
    : QWidget(parent)
    , scanner(new GameScanner(this))
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
    
    QProcess* process = new QProcess(this);
    
    // Set working directory to game folder
    QFileInfo exeInfo(game.executablePath);
    process->setWorkingDirectory(exeInfo.absolutePath());
    
    // Connect to handle process finish
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, game, process](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::CrashExit) {
                    QMessageBox::warning(this, "Game Crashed", 
                        QString("Game '%1' crashed!").arg(game.name));
                }
                qDebug() << "Game" << game.name << "exited with code" << exitCode;
                process->deleteLater();
            });
    
    // Connect error handler
    connect(process, &QProcess::errorOccurred,
            [this, game, process](QProcess::ProcessError error) {
                QString errorMsg;
                switch (error) {
                    case QProcess::FailedToStart:
                        errorMsg = "Failed to start. Check if the executable exists.";
                        break;
                    case QProcess::Crashed:
                        errorMsg = "Game crashed during execution.";
                        break;
                    default:
                        errorMsg = "An error occurred.";
                        break;
                }
                QMessageBox::critical(this, "Launch Error", 
                    QString("Failed to launch '%1': %2").arg(game.name, errorMsg));
                process->deleteLater();
            });
    
    // Start the game
    process->start(game.executablePath);
    
    if (!process->waitForStarted(3000)) {
        QMessageBox::critical(this, "Launch Error", 
            QString("Failed to start game '%1'").arg(game.name));
        process->deleteLater();
    }
}
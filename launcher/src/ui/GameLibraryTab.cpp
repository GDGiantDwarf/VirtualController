#include "GameLibraryTab.h"
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCoreApplication>

GameLibraryTab::GameLibraryTab(QWidget* parent, const QString& serverHostIn, int serverPortIn, bool useCliArgsIn)
    : QWidget(parent)
    , scanner(new GameScanner(this))
    , serverHost(serverHostIn)
    , serverPort(serverPortIn)
    , useCliArgs(useCliArgsIn)
{
    // If no CLI args provided, try to load per-game config
    if (!useCliArgs) {
        loadGameConfigs();
    }
    
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

void GameLibraryTab::loadGameConfigs() {
    // Look for games_config.json in the same directory as the launcher
    QDir launcherDir(QCoreApplication::applicationDirPath());
    QString configPath = launcherDir.filePath("games_config.json");
    
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open config file:" << configPath;
        return;
    }
    
    QByteArray configData = configFile.readAll();
    configFile.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(configData);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in config file";
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonObject gamesObj = root.value("games").toObject();
    
    for (const QString& gameName : gamesObj.keys()) {
        QJsonObject gameObj = gamesObj.value(gameName).toObject();
        GameConfig config;
        config.host = gameObj.value("host").toString("127.0.0.1");
        config.port = gameObj.value("port").toInt(8765);
        gameConfigs[gameName] = config;
        
        qDebug() << "Loaded config for" << gameName << ":" << config.host << ":" << config.port;
    }
}

GameConfig GameLibraryTab::getGameConfig(const QString& gameName) const {
    if (gameConfigs.contains(gameName)) {
        return gameConfigs[gameName];
    }
    // Return defaults if not found
    return GameConfig{"127.0.0.1", 8765};
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
        
        // Determine which host/port to use
        QString launchHost = serverHost;
        int launchPort = serverPort;
        
        if (!useCliArgs && gameConfigs.contains(game.name)) {
            // Use per-game config
            GameConfig config = getGameConfig(game.name);
            launchHost = config.host;
            launchPort = config.port;
            qDebug() << "Using game-specific config:" << launchHost << launchPort;
        } else if (useCliArgs) {
            qDebug() << "Using CLI args:" << launchHost << launchPort;
        } else {
            qDebug() << "Using defaults:" << launchHost << launchPort;
        }
        
        // Start the game with server host/port arguments
        QStringList args;
        args << launchHost << QString::number(launchPort);
        
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

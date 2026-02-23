#include "GameScanner.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

GameScanner::GameScanner(QObject* parent)
    : QObject(parent)
{}

QVector<GameInfo> GameScanner::scanGames(const QString& gamesDirectory) {
    QVector<GameInfo> games;
    
    // Get all .exe files from the build output directory (except the launcher itself)
    QDir buildDir(QCoreApplication::applicationDirPath());
    QStringList exeFiles = buildDir.entryList(QStringList("*.exe"), QDir::Files);
    
    for (const QString& exeFile : exeFiles) {
        QString gameName = exeFile.left(exeFile.length() - 4); // Remove .exe extension
        
        // Skip the launcher itself and all servers
        if (gameName == "GameLibraryLauncher" || gameName == "GameServer" || gameName.endsWith("GameServer")) {
            continue;
        }
        
        // Validate that at least the executable exists
        QString exePath = buildDir.filePath(exeFile);
        QFileInfo exeInfo(exePath);
        
        if (exeInfo.exists() && exeInfo.isFile()) {
            // Try to load icon from build directory, but don't require it
            QString icoPath = buildDir.filePath(gameName + ".ico");
            
            // Create a dummy games folder path (not used anymore, but kept for compatibility)
            GameInfo game(gameName, gamesDirectory + "/" + gameName, exePath, icoPath);
            games.append(game);
            qDebug() << "Found game:" << game.name << "at" << game.executablePath;
        }
    }
    
    return games;
}

bool GameScanner::isValidGameFolder(const QString& folderPath, const QString& folderName) {
    // This function is now deprecated - we scan the build directory directly
    // Kept for backward compatibility if needed
    return true;
}

GameInfo GameScanner::createGameInfo(const QString& folderPath, const QString& folderName) {
    // Use executable from root build directory (unified build)
    QDir rootBuildDir(QCoreApplication::applicationDirPath());
    QString exePath = rootBuildDir.filePath(folderName + ".exe");
    
    // Icon is also in the build output directory now (copied at build time)
    QString icoPath = rootBuildDir.filePath(folderName + ".ico");
    
    return GameInfo(folderName, folderPath, exePath, icoPath);
}
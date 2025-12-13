#include "GameScanner.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

GameScanner::GameScanner(QObject* parent)
    : QObject(parent)
{}

QVector<GameInfo> GameScanner::scanGames(const QString& gamesDirectory) {
    QVector<GameInfo> games;
    
    QDir gamesDir(gamesDirectory);
    if (!gamesDir.exists()) {
        qWarning() << "Games directory does not exist:" << gamesDirectory;
        return games;
    }
    
    // Get all subdirectories
    QFileInfoList folders = gamesDir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, 
        QDir::Name
    );
    
    for (const QFileInfo& folderInfo : folders) {
        QString folderName = folderInfo.fileName();
        QString folderPath = folderInfo.absoluteFilePath();
        
        if (isValidGameFolder(folderPath, folderName)) {
            GameInfo game = createGameInfo(folderPath, folderName);
            if (game.isValid()) {
                games.append(game);
                qDebug() << "Found game:" << game.name;
            }
        }
    }
    
    return games;
}

bool GameScanner::isValidGameFolder(const QString& folderPath, const QString& folderName) {
    QDir folder(folderPath);
    
    // Check for .exe file with same name as folder
    QString exeName = folderName + ".exe";
    QString exePath = folder.filePath(exeName);
    
    // Check for .ico file with same name as folder
    QString icoName = folderName + ".ico";
    QString icoPath = folder.filePath(icoName);
    
    QFileInfo exeInfo(exePath);
    QFileInfo icoInfo(icoPath);
    
    bool valid = exeInfo.exists() && exeInfo.isFile() && 
                 icoInfo.exists() && icoInfo.isFile();
    
    if (!valid) {
        qDebug() << "Invalid game folder:" << folderName 
                 << "- exe exists:" << exeInfo.exists()
                 << "- ico exists:" << icoInfo.exists();
    }
    
    return valid;
}

GameInfo GameScanner::createGameInfo(const QString& folderPath, const QString& folderName) {
    QDir folder(folderPath);
    
    QString exePath = folder.filePath(folderName + ".exe");
    QString icoPath = folder.filePath(folderName + ".ico");
    
    return GameInfo(folderName, folderPath, exePath, icoPath);
}
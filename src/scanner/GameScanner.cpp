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
            games.append(game);
            qDebug() << "Found game:" << game.name << "at" << game.executablePath;
        }
    }
    
    return games;
}

bool GameScanner::isValidGameFolder(const QString& folderPath, const QString& folderName) {
    QDir folder(folderPath);
    
    // Check for .exe file with same name as folder
    QString exeName = folderName + ".exe";
    QString exePath = folder.filePath(exeName);
    
    QFileInfo exeInfo(exePath);
    
    if (!exeInfo.exists() || !exeInfo.isFile()) {
        qDebug() << "Game folder" << folderName << "missing executable:" << exeName;
        return false;
    }
    
    return true;
}

GameInfo GameScanner::createGameInfo(const QString& folderPath, const QString& folderName) {
    QDir folder(folderPath);
    
    QString exePath = folder.filePath(folderName + ".exe");
    QString icoPath = folder.filePath(folderName + ".ico");
    
    // Icon is optional, exe is mandatory
    return GameInfo(folderName, folderPath, exePath, icoPath);
}
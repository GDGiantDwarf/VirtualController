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
    // Check if executable exists in the root build directory (unified build)
    // Root build structure: VirtualController/build/bin/Release/<name>.exe
    QDir rootBuildDir(QCoreApplication::applicationDirPath());
    QString rootExePath = rootBuildDir.filePath(folderName + ".exe");
    
    QFileInfo rootExe(rootExePath);
    if (rootExe.exists() && rootExe.isFile()) {
        return true;
    }

    qDebug() << "Game folder" << folderName << "missing executable in root build:" << rootExePath;
    return false;
}

GameInfo GameScanner::createGameInfo(const QString& folderPath, const QString& folderName) {
    // Use executable from root build directory (unified build)
    QDir rootBuildDir(QCoreApplication::applicationDirPath());
    QString exePath = rootBuildDir.filePath(folderName + ".exe");
    
    // Icon is in the game's source folder
    QDir folder(folderPath);
    QString icoPath = folder.filePath(folderName + ".ico");
    
    return GameInfo(folderName, folderPath, exePath, icoPath);
}
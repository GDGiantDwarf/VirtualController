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

    // Prefer build/bin/Release/<name>.exe if present, fall back to <name>.exe
    const QString releaseExePath = folder.filePath("build/bin/Release/" + folderName + ".exe");
    const QString flatExePath = folder.filePath(folderName + ".exe");

    QFileInfo releaseExe(releaseExePath);
    QFileInfo flatExe(flatExePath);

    if (releaseExe.exists() && releaseExe.isFile()) {
        return true;
    }

    if (flatExe.exists() && flatExe.isFile()) {
        return true;
    }

    qDebug() << "Game folder" << folderName << "missing executable in either" << releaseExePath
             << "or" << flatExePath;
    return false;
}

GameInfo GameScanner::createGameInfo(const QString& folderPath, const QString& folderName) {
    QDir folder(folderPath);

    // Pick build/bin/Release executable if available; otherwise use flat exe
    QString exePath = folder.filePath("build/bin/Release/" + folderName + ".exe");
    QFileInfo exeInfo(exePath);
    if (!exeInfo.exists() || !exeInfo.isFile()) {
        exePath = folder.filePath(folderName + ".exe");
    }

    QString icoPath = folder.filePath(folderName + ".ico");
    
    // Icon is optional, exe is mandatory
    return GameInfo(folderName, folderPath, exePath, icoPath);
}
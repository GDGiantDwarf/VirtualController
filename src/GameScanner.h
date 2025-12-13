#ifndef GAMESCANNER_H
#define GAMESCANNER_H

#include <QObject>
#include <QVector>
#include <QString>
#include "GameInfo.h"

class GameScanner : public QObject {
    Q_OBJECT
    
public:
    explicit GameScanner(QObject* parent = nullptr);
    
    // Scan the ./games/ directory for valid games
    QVector<GameInfo> scanGames(const QString& gamesDirectory = "./games");
    
private:
    bool isValidGameFolder(const QString& folderPath, const QString& folderName);
    GameInfo createGameInfo(const QString& folderPath, const QString& folderName);
};

#endif // GAMESCANNER_H
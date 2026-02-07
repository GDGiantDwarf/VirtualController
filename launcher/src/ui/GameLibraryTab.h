#ifndef GAMELIBRARYTAB_H
#define GAMELIBRARYTAB_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProcess>
#include "GameInfo.h"
#include "GameScanner.h"

class GameLibraryTab : public QWidget {
    Q_OBJECT
    
public:
    explicit GameLibraryTab(QWidget* parent = nullptr, const QString& serverHost = "127.0.0.1", int serverPort = 8765);
    
private slots:
    void onGameClicked(QListWidgetItem* item);
    void onRefreshClicked();
    
private:
    void setupUI();
    void loadGames();
    void launchGame(const GameInfo& game);
    
    QListWidget* gamesList;
    QPushButton* refreshButton;
    GameScanner* scanner;
    QVector<GameInfo> games;
    QString serverHost;
    int serverPort;
};

#endif // GAMELIBRARYTAB_H
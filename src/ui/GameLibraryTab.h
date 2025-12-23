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
    explicit GameLibraryTab(QWidget* parent = nullptr);
    
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
};

#endif // GAMELIBRARYTAB_H
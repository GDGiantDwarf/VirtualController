#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <QString>
#include <QIcon>
#include <QMetaType>

struct GameInfo {
    QString name;
    QString folderPath;
    QString executablePath;
    QString iconPath;
    QIcon icon;
    
    GameInfo() = default;
    
    GameInfo(const QString& gameName, 
             const QString& folder, 
             const QString& exe, 
             const QString& ico)
        : name(gameName)
        , folderPath(folder)
        , executablePath(exe)
        , iconPath(ico)
        , icon(ico)
    {}
    
    bool isValid() const {
        return !name.isEmpty() && 
               !executablePath.isEmpty() && 
               !iconPath.isEmpty();
    }
};

Q_DECLARE_METATYPE(GameInfo)

#endif // GAMEINFO_H
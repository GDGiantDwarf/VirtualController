#ifndef GAMEINFO_H
#define GAMEINFO_H

#include <QString>
#include <QIcon>
#include <QMetaType>
#include <QFileInfo>

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
    {
        // Load icon only if it exists (optional)
        QFileInfo icoFile(ico);
        if (icoFile.exists() && icoFile.isFile()) {
            icon = QIcon(ico);
        }
    }
    
    bool isValid() const {
        // Only exe is mandatory, icon is optional
        return !name.isEmpty() && !executablePath.isEmpty();
    }
};

Q_DECLARE_METATYPE(GameInfo)

#endif // GAMEINFO_H